#include "catch2/catch.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <future>
#include <atomic>
#include <unordered_map>
#include <any>
#include <typeindex>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <string>
#include <any>
#include <random>

////////////////////////////////////////////////////////////////////
// 数据上下文，支持具体的算法类进行数据存取
struct DataContext {
    template<typename T>
    T* Fetch() {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = dataRepo_.find(std::type_index(typeid(T)));
        if (it != dataRepo_.end()) {
            return std::any_cast<std::shared_ptr<T>>(it->second).get();
        }
        return nullptr;
    }

    template<typename T, typename ...Args>
    T* Create(Args&& ...args) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = dataRepo_.find(typeid(T));
        if (it != dataRepo_.end()) {
            return nullptr;
        }
        auto ptr = std::shared_ptr<T>(new T{std::forward<Args>(args)...});
        dataRepo_[std::type_index(typeid(T))] = ptr;
        return ptr.get();
    }

    template<typename T, typename VISITOR>
    std::size_t Travel(VISITOR&& visitor) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::size_t count = 0;
        for (auto& [type, item] : dataRepo_) {
            auto dataPtr = std::any_cast<std::shared_ptr<T>>(item).get();
            std::forward<VISITOR>(visitor)(type, *dataPtr);
            count++;
        }
        return count;
    }

private:
    std::unordered_map<std::type_index, std::any> dataRepo_;
    mutable std::shared_mutex mutex_;
};

// 业务格式各样的算法类满足如下原型
// struct Algorithm {
//     void Init() {
//     }
//     void Execute(DataContext& context) {
//     }
// };

////////////////////////////////////////////////////////////////////
struct ProcessorId {
    static constexpr uint32_t MAX_DEPTH = 8;       // 最多支持8层嵌套
    static constexpr uint32_t BITS_PER_LEVEL = 8;  // 每层使用8位，每层最多支持255个处理器
    static constexpr uint64_t LEVEL_MASK = 0xFF;   // 每层的掩码
    
    ProcessorId() : value_(0) {}
    
    explicit ProcessorId(uint64_t value) : value_(value) {}
    
    static ProcessorId CreateChild(const ProcessorId& parent, uint32_t childIndex) {
        uint64_t parentValue = parent.value_;
        uint32_t depth = GetDepthFromValue(parentValue);
        
        if (depth >= MAX_DEPTH) {
            return parent;
        }
        
        uint32_t newDepth = depth + 1;
        uint64_t shiftAmount = (newDepth - 1) * BITS_PER_LEVEL;
        uint64_t newValue = parentValue | ((childIndex & LEVEL_MASK) << shiftAmount);
        
        newValue = (newValue & ~(LEVEL_MASK << 56)) | (static_cast<uint64_t>(newDepth) << 56);
        return ProcessorId(newValue);
    }
    
    static ProcessorId Root() {
        // Root 节点的深度为 1，ID值为 1
        return ProcessorId((static_cast<uint64_t>(1) << 56) | 1);
    }
    
    ProcessorId GetParent() const {
        uint32_t depth = GetDepth();
        if (depth <= 1) {
             // 根ID没有父ID
            return ProcessorId();
        }
        
        uint64_t parentValue = value_;
        uint64_t clearMask = ~(LEVEL_MASK << ((depth - 1) * BITS_PER_LEVEL));
        parentValue &= clearMask;
        
        parentValue = (parentValue & ~(LEVEL_MASK << 56)) | (static_cast<uint64_t>(depth - 1) << 56);
        return ProcessorId(parentValue);
    }
    
    uint32_t GetDepth() const {
        return GetDepthFromValue(value_);
    }
    
    uint64_t GetValue() const {
        return value_;
    }
    
    uint8_t GetLevelValue(uint32_t level) const {
        if (level >= GetDepth()) {
            return 0;
        }
        
        uint32_t shift = level * BITS_PER_LEVEL;
        return (value_ >> shift) & LEVEL_MASK;
    }
    
    std::string ToString() const {
        uint32_t depth = GetDepth();
        if (depth == 0) return "null";
        
        std::ostringstream oss;
        oss << static_cast<int>(GetLevelValue(0));
        for (uint32_t i = 1; i < depth; ++i) {
            oss << "." << static_cast<int>(GetLevelValue(i));
        }
        return oss.str();
    }
    
    bool operator==(const ProcessorId& other) const {
        return value_ == other.value_;
    }
    
    bool operator!=(const ProcessorId& other) const {
        return value_ != other.value_;
    }
    
private:
    static uint32_t GetDepthFromValue(uint64_t value) {
        return (value >> 56) & LEVEL_MASK;
    }
    
private:
    // 单一的64位值存储整个ID
    uint64_t value_;
};

namespace std {
    template<>
    struct hash<ProcessorId> {
        size_t operator()(const ProcessorId& id) const {
            return std::hash<uint64_t>{}(id.GetValue());
        }
    };
}

struct ProcessorInfo {
    ProcessorInfo(const std::string& name, ProcessorId id)
    : name{name}, id{id} {}

    std::string ToString() const {
        return name + " [" + id.ToString() + "]";
    }

    const std::string name;
    const ProcessorId id;
};

std::ostream& operator<<(std::ostream& os, const ProcessorInfo& processInfo) {
    os << processInfo.ToString();
    return os;
}

////////////////////////////////////////////////////////////////////

// 执行器返回值
enum class Status {
    OK,
    CANCELLED,
    ERROR
};

std::string ToString(Status status) {
    switch (status) {
    case Status::OK:
        return "OK";
    case Status::CANCELLED:
        return "CANCELLED";
    case Status::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::ostream& operator<<(std::ostream& os, Status status) {
    os << ToString(status);
    return os;
}

////////////////////////////////////////////////////////////////////
struct ProcessTracker {
    virtual void TrackEnter(const ProcessorInfo&) = 0;
    virtual void TrackExit(const ProcessorInfo&, Status) = 0;
    virtual void Dump() const {}
    virtual ~ProcessTracker() = default;
};

struct ConsoleTracker : ProcessTracker {
    void TrackEnter(const ProcessorInfo& info) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (info.id == ProcessorId::Root()) {
            std::cout << "\n============== Schedule Start =============\n";
        } 
        std::cout << info << " enter..." << "\n";
    }

    void TrackExit(const ProcessorInfo& info, Status status) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << info << " exit with status: " << status << "!" << "\n";
    }

private:
    std::mutex mutex_;
};

struct TimingTracker : ProcessTracker {
    void TrackEnter(const ProcessorInfo& info) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::high_resolution_clock::now();
        timingData_[info.id] = std::make_pair(now, std::chrono::nanoseconds(0));
    }

    void TrackExit(const ProcessorInfo& info, Status status) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::high_resolution_clock::now();
        auto it = timingData_.find(info.id);
        if (it != timingData_.end()) {
            auto duration = now - it->second.first;
            it->second.second = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        }
    }

    void Dump() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\n======= Processor Timing Statistics =======\n";
        
        ProcessorId rootId = ProcessorId::Root();
        if (timingData_.find(rootId) == timingData_.end()) {
            std::cout << "No timing data available!\n";
            return;
        } 
        DumpProcessor(rootId, 0);
    }

private:
    void DumpProcessor(const ProcessorId& id, int level) const {
        auto timingIt = timingData_.find(id);

        if (timingIt == timingData_.end()) {
            std::cout << "No timing data find for id << " << id.ToString() << " in level " << level << "!\n";
            return;
        }

        double ms = timingIt->second.second.count() / 1'000'000.0;
        std::string indent(level * 2, ' ');
        std::cout << indent << "[" << id.ToString() << "]: " << ms << " ms\n";
        
        std::vector<ProcessorId> children;
        for (const auto& [childId, _] : timingData_) {
            if (childId.GetParent() == id) {
                children.push_back(childId);
            }
        }

        uint32_t childLevel = id.GetDepth();
        std::sort(children.begin(), children.end(), [childLevel](const ProcessorId& a, const ProcessorId& b) {
            return a.GetLevelValue(childLevel) < b.GetLevelValue(childLevel);
        });
        
        for (const auto& childId : children) {
            DumpProcessor(childId, level + 1);
        }
    }

private:
    mutable std::mutex mutex_;
    using TimingData = std::pair<std::chrono::high_resolution_clock::time_point, std::chrono::nanoseconds>;
    std::unordered_map<ProcessorId, TimingData> timingData_;
};

////////////////////////////////////////////////////////////////////

// 执行器的上下文
struct ProcessContext {
    static ProcessContext CreateSubContext(ProcessContext& parentCtx) {
        return ProcessContext(parentCtx.GetDataContext(), &parentCtx.stopFlag_, parentCtx.tracker_);
    }

    ProcessContext(DataContext& dataCtx)
    : dataCtx_(dataCtx) {}

    void Stop() {
        stopFlag_.store(true);
    }

    void Resume() {
        stopFlag_.store(false);
    }

    bool TryStop() {
        bool expected = false;
        return stopFlag_.compare_exchange_strong(expected, true);
    }

    bool IsStopped() const {
        if (parentStopFlag_ && parentStopFlag_->load()) {
            return true;
        }
        return stopFlag_.load();
    }

    void SetTracker(ProcessTracker* tracker) {
        tracker_ = tracker;
    }

    void EnterProcess(const ProcessorInfo& info) {
        if (tracker_) {
            tracker_->TrackEnter(info);
        }
    }

    void ExitProcess(const ProcessorInfo& info, Status status) {
        if (tracker_) {
            tracker_->TrackExit(info, status);
        }
    }

    DataContext& GetDataContext() {
        return dataCtx_;
    }

private:
    ProcessContext(DataContext& dataCtx, const std::atomic<bool>* parentStopFlag, ProcessTracker* tracker)
    : dataCtx_(dataCtx), parentStopFlag_(parentStopFlag), tracker_(tracker) {}

private:
    DataContext& dataCtx_;
    std::atomic<bool> stopFlag_{false};
    const std::atomic<bool>* parentStopFlag_{nullptr};
    ProcessTracker* tracker_{nullptr};
};

////////////////////////////////////////////////////////////////////

// 执行器接口
struct Processor {
    Processor(const std::string& name)
    : name_(name) {}

    const std::string& GetName() const {
        return name_;
    }

    ProcessorId GetId() const {
        return id_;
    }

    Status Process(ProcessContext& ctx) {
        ProcessorInfo info{name_, id_};

        ctx.EnterProcess(info);

        if (ctx.IsStopped()) {
            ctx.ExitProcess(info, Status::CANCELLED);
            return Status::CANCELLED;
        }
        auto status = Execute(ctx);
        ctx.ExitProcess(info, status);
        return status;
    }
    
    virtual void Init(const ProcessorInfo& parentInfo, uint32_t childIndex) {
        if (parentInfo.id == ProcessorId::Root() && childIndex == 0) {
            id_ = ProcessorId::Root();
        } else {
            id_ = ProcessorId::CreateChild(parentInfo.id, childIndex);
        }
        name_ = parentInfo.name + "/" + name_;
    }

    virtual ~Processor() = default;

private:
    virtual Status Execute(ProcessContext& ctx) = 0;

protected:
    std::string name_;
    ProcessorId id_{0};
};

// 基本执行器，对业务算法的适配
template<typename ALGO>
struct AlgoProcessor : Processor {
    using Processor::Processor;

private:
    void Init(const ProcessorInfo& parentInfo, uint32_t childIndex) override {
        Processor::Init(parentInfo, childIndex);
        algo_.Init();
    }

    Status Execute(ProcessContext& ctx) override {
        algo_.Execute(ctx.GetDataContext());
        return Status::OK;
    }

private:
    ALGO algo_;
};

// 组合执行器的公共基类
struct GroupProcessor : Processor {
    using Processor::Processor;

    void AddProcessor(std::unique_ptr<Processor> processor) {
        processors_.emplace_back(std::move(processor));
    }

private:
    void Init(const ProcessorInfo& parentInfo, uint32_t childIndex) override {
        Processor::Init(parentInfo, childIndex);

        for (uint32_t i = 0; i < processors_.size(); ++i) {
            processors_[i]->Init(ProcessorInfo{name_, id_}, i + 1);
        }
    }

protected:
    std::vector<std::unique_ptr<Processor>> processors_;
};

// 串行执行器
struct SequentialProcessor : GroupProcessor {
    using GroupProcessor::GroupProcessor;

private:
    Status Execute(ProcessContext& ctx) override {
        for (auto& processor : processors_) {
            auto status = processor->Process(ctx);
            if (status != Status::OK) {
                return status;
            }
        }
        return Status::OK;
    }
};

struct AsyncResult {
    AsyncResult(const std::string& name, Status status)
    : name(name), status(status) {}

    std::string name;
    Status status;
};

// 并行执行器
struct ParallelProcessor : GroupProcessor {
    using GroupProcessor::GroupProcessor;

private:
    Status Execute(ProcessContext& ctx) override {
        std::vector<std::future<AsyncResult>> futures;
        for (auto& processor : processors_) {
            futures.emplace_back(std::async(std::launch::async, [&]() {
                return AsyncResult(processor->GetName(), processor->Process(ctx));
            }));
        }

        Status overall = Status::OK;
        for (auto& fut : futures) {
            auto ret = fut.get();
            if (ret.status != Status::OK) {
                overall = ret.status;
            }
        }
        return overall;
    }
};

// 并行竞争执行器
struct RaceProcessor : GroupProcessor {
    using GroupProcessor::GroupProcessor;

private:
    Status Execute(ProcessContext& ctx) override {
        std::vector<std::future<AsyncResult>> futures;
        std::promise<Status> finalPromise;
        auto finalFuture = finalPromise.get_future();

        auto innerCtx = ProcessContext::CreateSubContext(ctx);

        for (auto& processor : processors_) {
            futures.emplace_back(std::async(std::launch::async, [&]() {
                Status status = processor->Process(innerCtx);
                if (status == Status::OK) {
                    if (innerCtx.TryStop()) {
                        finalPromise.set_value(status);
                    }
                }
                return AsyncResult(processor->GetName(), status);
            }));
        }

        Status overall = finalFuture.get();
        innerCtx.Stop();

        for (auto& fut : futures) {
            auto ret = fut.get();
        }
        return overall;
    }
};

////////////////////////////////////////////////////////////////////
// Data parallelism

namespace data_parallel_detail {

    template<typename T>
    thread_local uint32_t parallel_id = -1;
    
    template<typename T>
    uint32_t GetParallelId() {
        return parallel_id<T>;
    }
    
    template<typename T>
    void SetParallelId(uint32_t id) {
        parallel_id<T> = id;
    }

    template<typename T>
    struct AutoSwitchParallelId {
        AutoSwitchParallelId(uint32_t id) {
            oriId_ = GetParallelId<T>();
            SetParallelId<T>(id);
        }

        ~AutoSwitchParallelId() {
            SetParallelId<T>(oriId_);
        }
    private:
        uint32_t oriId_;
    };
}

using ProcessorFactory = std::function<std::unique_ptr<Processor>()>;

template<typename DTYPE, uint32_t N>
struct DataGroupProcessor : GroupProcessor {
    DataGroupProcessor(const std::string& name, ProcessorFactory factory)
    : GroupProcessor(name), factory_(factory) {}

private:
    void Init(const ProcessorInfo& parentInfo, uint32_t childIndex) override {
        if (!processors_.empty()) {
            return;
        }
        Processor::Init(parentInfo, childIndex);

        for (uint32_t i = 0; i < N; ++i) {
            auto processor = factory_();
            processor->Init(ProcessorInfo{name_, id_}, i + 1);
            processors_.push_back(std::move(processor));
        }
    }

private:
    ProcessorFactory factory_;
};

template<typename DTYPE, uint32_t N>
struct DataParallelProcessor : DataGroupProcessor<DTYPE, N> {
    using DataGroupProcessor<DTYPE, N>::DataGroupProcessor;

private:
    using DataGroupProcessor<DTYPE, N>::processors_;

    Status Execute(ProcessContext& ctx) override {

        std::vector<std::future<AsyncResult>> futures;

        for (int i = 0; i < processors_.size(); i++) {
            futures.emplace_back(std::async(std::launch::async, [&ctx, i, processor = processors_[i].get()]() {
                data_parallel_detail::AutoSwitchParallelId<DTYPE> switcher(i);
                return AsyncResult(processor->GetName(), processor->Process(ctx));
            }));
        }

        Status overall = Status::OK;
        for (auto& fut : futures) {
            auto ret = fut.get();
            if (ret.status != Status::OK) {
                overall = ret.status;
            }
        }
        return overall;
    }
};

template<typename DTYPE, uint32_t N>
struct DataRaceProcessor : DataGroupProcessor<DTYPE, N> {
    using DataGroupProcessor<DTYPE, N>::DataGroupProcessor;

private:
    using DataGroupProcessor<DTYPE, N>::processors_;

    Status Execute(ProcessContext& ctx) override {
        std::vector<std::future<AsyncResult>> futures;
        std::promise<Status> finalPromise;
        auto finalFuture = finalPromise.get_future();

        auto innerCtx = ProcessContext::CreateSubContext(ctx);

        for (int i = 0; i < processors_.size(); i++) {
            futures.emplace_back(std::async(std::launch::async, [&, i, processor = processors_[i].get()]() {
                data_parallel_detail::AutoSwitchParallelId<DTYPE> switcher(i);
                Status status = processor->Process(innerCtx);
                if (status == Status::OK) {
                    if (innerCtx.TryStop()) {
                        finalPromise.set_value(status);
                    }
                }
                return AsyncResult(processor->GetName(), status);
            }));
        }

        Status overall = finalFuture.get();
        innerCtx.Stop();

        for (auto& fut : futures) {
            auto ret = fut.get();
        }
        return overall;
    }
};

////////////////////////////////////////////////////////////////////
struct GroupTracker : ProcessTracker {
    void AddTracker(std::unique_ptr<ProcessTracker> tracker) {
        trackers_.emplace_back(std::move(tracker));
    }

    void Dump() const override {
        for (auto& tracker : trackers_) {
            tracker->Dump();
        }
    }

private:
    void TrackEnter(const ProcessorInfo& info) override {
        for (auto& tracker : trackers_) {
            tracker->TrackEnter(info);
        }
    }

    void TrackExit(const ProcessorInfo& info, Status status) override {
        for (auto& tracker : trackers_) {
            tracker->TrackExit(info, status);
        }
    }

private:
    std::vector<std::unique_ptr<ProcessTracker>> trackers_;
};

// Processor 调度器
struct Scheduler {
    Scheduler(std::unique_ptr<Processor> rootProcessor)
    : rootProcessor_{std::move(rootProcessor)} {
        rootProcessor_->Init(ProcessorInfo{"root", ProcessorId::Root()}, 0);
    }

    Status Run(DataContext& dataCtx) {
        ProcessContext processCtx{dataCtx};
        processCtx.SetTracker(&tracker_);
        return rootProcessor_->Process(processCtx);
    }

    void AddTracker(std::unique_ptr<ProcessTracker> tracker) {
        tracker_.AddTracker(std::move(tracker));
    }

    void Dump() const {
        tracker_.Dump();
    }

private:
    std::unique_ptr<Processor> rootProcessor_;
    GroupTracker tracker_;
};

////////////////////////////////////////////////////////////////////

// Processor 构造器

template<typename ALGO>
std::unique_ptr<Processor> MakeProcessor(const std::string& name) {
    return std::make_unique<AlgoProcessor<ALGO>>(name);
}

template<typename GROUP_PROCESSOR, typename ...PROCESSORS>
std::unique_ptr<Processor> MakeGroupProcessor(const std::string& name, PROCESSORS&& ...processors) {
    auto processor = std::make_unique<GROUP_PROCESSOR>(name);
    (processor->AddProcessor(std::forward<PROCESSORS>(processors)), ...);
    return processor;
}

template<template <typename, uint32_t> class DATA_PROCESSOR, typename DTYPE, uint32_t N>
std::unique_ptr<Processor> MakeDataGroupProcessor(const std::string& name, ProcessorFactory factory) {
    return std::make_unique<DATA_PROCESSOR<DTYPE, N>>(name, factory);
}

////////////////////////////////////////////////////////////////////

// Processor 构造器宏

#define PROCESS(ALGO) MakeProcessor<ALGO>(#ALGO)
#define SEQUENCE(...) MakeGroupProcessor<SequentialProcessor>("sequential", __VA_ARGS__)
#define PARALLEL(...) MakeGroupProcessor<ParallelProcessor>("parallel", __VA_ARGS__)
#define RACE(...)     MakeGroupProcessor<RaceProcessor>("race", __VA_ARGS__)
#define DATA_PARALLEL(DTYPE, N, ...) MakeDataGroupProcessor<DataParallelProcessor, DTYPE, N>("data_parallel", [&]() { return __VA_ARGS__; })
#define DATA_RACE(DTYPE, N, ...) MakeDataGroupProcessor<DataRaceProcessor, DTYPE, N>("data_race", [&]() { return __VA_ARGS__; })

#define SCHEDULE(...) Scheduler(__VA_ARGS__)

////////////////////////////////////////////////////////////////////

struct TestTracker {
    void Track(const std::string& key, const std::any& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        trackedData_.emplace_back(key, value);
    }

    bool HasKey(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return std::any_of(trackedData_.begin(), trackedData_.end(), [&](const auto& pair) {
            return pair.first == key;
        });
    }

    template<typename T>
    bool HasValue(const std::string& key, const T& value) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return std::any_of(trackedData_.begin(), trackedData_.end(), [&](const auto& pair) {
            return pair.first == key && std::any_cast<T>(pair.second) == value;
        });
    }

    bool KeyAt(const std::string& key, std::size_t pos) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = std::find_if(trackedData_.begin(), trackedData_.end(), [&](const auto& pair) {
            return pair.first == key;
        });
        return it != trackedData_.end() && std::distance(trackedData_.begin(), it) == pos;
    }

    std::size_t Size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return trackedData_.size();
    }

    template<typename T>
    void Print() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::for_each(trackedData_.begin(), trackedData_.end(), [](const auto& pair) {
            std::cout << pair.first << ": " << std::any_cast<T>(pair.second) << "\n";
        });
    }

private:
    std::vector<std::pair<std::string, std::any>> trackedData_;
    mutable std::shared_mutex mutex_;
};

// 模拟业务算法
struct MockAlgo {
    MockAlgo(const std::string& name, 
        std::chrono::milliseconds sleepTime = std::chrono::milliseconds(0))
    : name_{name}, sleepTime_{sleepTime} 
    {}

    void Init() {
    }

    void Execute(DataContext& context) {
        std::this_thread::sleep_for(sleepTime_);
        
        auto tracker = context.Fetch<TestTracker>();
        if (tracker) {
            tracker->Track(name_, 0);
        }
    }

private:
    std::chrono::milliseconds sleepTime_;
    std::string name_;
};

#define DEFINE_MOCK_ALGO(name, sleepTime) \
struct name : MockAlgo { \
    name() : MockAlgo(#name, std::chrono::milliseconds(sleepTime)) {} \
}

DEFINE_MOCK_ALGO(MockAlgo1, 100);
DEFINE_MOCK_ALGO(MockAlgo2, 200);
DEFINE_MOCK_ALGO(MockAlgo3, 300);
DEFINE_MOCK_ALGO(MockAlgo4, 400);
DEFINE_MOCK_ALGO(MockAlgo5, 500);
DEFINE_MOCK_ALGO(MockAlgo6, 600);
DEFINE_MOCK_ALGO(MockAlgo7, 700);
DEFINE_MOCK_ALGO(MockAlgo8, 800);

////////////////////////////////////////////////////////////////////

struct MyDatas : std::vector<int> {
    using std::vector<int>::vector;
};

namespace  {
    std::random_device RD;
    std::mt19937 GEN(RD());
    std::uniform_int_distribution<> DISTR(1, 1000);
}

struct DataReadAlgo {
    void Init() {
    }

    void Execute(DataContext& context) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DISTR(GEN)));

        int instanceId = data_parallel_detail::GetParallelId<MyDatas>();
        auto& myDatas = *context.Fetch<MyDatas>();

        auto tracker = context.Fetch<TestTracker>();
        if (tracker) {
            tracker->Track("DataReadAlgo", myDatas[instanceId]);
        }
    }
};

struct DataWriteAlgo {
    void Init() {
    }

    void Execute(DataContext& context) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DISTR(GEN)));
        
        int instanceId = data_parallel_detail::GetParallelId<MyDatas>();
        auto& myDatas = *context.Fetch<MyDatas>();
        
        auto tracker = context.Fetch<TestTracker>();
        if (tracker) {
            tracker->Track("DataWriteAlgo", myDatas[instanceId]);
        }

        myDatas[instanceId] *= 2;
    }
};

////////////////////////////////////////////////////////////////////

TEST_CASE("Processor Test") {
    auto scheduler = SCHEDULE(
        SEQUENCE(
            PROCESS(MockAlgo1),
            PARALLEL(
                PROCESS(MockAlgo2),
                PROCESS(MockAlgo3),
                PARALLEL(
                    PROCESS(MockAlgo4),
                    RACE(
                        PROCESS(MockAlgo5),
                        SEQUENCE(
                            PROCESS(MockAlgo6),
                            PROCESS(MockAlgo7)
                        )
                    )
                )
            ),
            PROCESS(MockAlgo8)
        )
    );
    
    DataContext dataCtx;
    dataCtx.Create<TestTracker>();

    scheduler.AddTracker(std::make_unique<ConsoleTracker>());
    scheduler.AddTracker(std::make_unique<TimingTracker>());

    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);

    scheduler.Dump();

    auto tracker = dataCtx.Fetch<TestTracker>();
    REQUIRE(tracker->Size() == 7);
    REQUIRE(tracker->KeyAt("MockAlgo1", 0));
    REQUIRE(tracker->KeyAt("MockAlgo2", 1));
    REQUIRE(tracker->KeyAt("MockAlgo3", 2));
    REQUIRE(tracker->KeyAt("MockAlgo8", 6));

    REQUIRE(tracker->HasKey("MockAlgo4"));
    REQUIRE(tracker->HasKey("MockAlgo5"));
    REQUIRE(tracker->HasKey("MockAlgo6"));
}

TEST_CASE("DataParallelProcessor basic Test") {    
    auto scheduler = SCHEDULE(
        DATA_PARALLEL(MyDatas, 5, PROCESS(DataReadAlgo))
    );
    
    DataContext dataCtx;
    dataCtx.Create<TestTracker>();
    dataCtx.Create<MyDatas>(1, 2, 3, 4, 5);

    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);

    auto tracker = dataCtx.Fetch<TestTracker>();
    REQUIRE(tracker->Size() == 5);
    REQUIRE(tracker->HasValue("DataReadAlgo", 1));
    REQUIRE(tracker->HasValue("DataReadAlgo", 2));
    REQUIRE(tracker->HasValue("DataReadAlgo", 3));
    REQUIRE(tracker->HasValue("DataReadAlgo", 4));
    REQUIRE(tracker->HasValue("DataReadAlgo", 5));
}

TEST_CASE("DataParallelProcessor complex Test") {
    auto scheduler = SCHEDULE(
        SEQUENCE(
            PROCESS(MockAlgo1),
            DATA_PARALLEL(MyDatas, 3, 
                SEQUENCE(
                    PROCESS(MockAlgo2),
                    PROCESS(DataWriteAlgo),
                    PROCESS(DataReadAlgo)
                )
            ),
            PROCESS(MockAlgo3)
        )
    );
    
    DataContext dataCtx;
    dataCtx.Create<TestTracker>();
    dataCtx.Create<MyDatas>(1, 2, 3);

    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);

    auto tracker = dataCtx.Fetch<TestTracker>();
    REQUIRE(tracker->Size() == 11);
    REQUIRE(tracker->KeyAt("MockAlgo1", 0));
    REQUIRE(tracker->KeyAt("MockAlgo3", 10));

    REQUIRE(tracker->HasKey("MockAlgo2"));

    REQUIRE(tracker->HasValue("DataWriteAlgo", 1));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 2));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 3));
   
    REQUIRE(tracker->HasValue("DataReadAlgo", 2));
    REQUIRE(tracker->HasValue("DataReadAlgo", 4));
    REQUIRE(tracker->HasValue("DataReadAlgo", 6));
}

TEST_CASE("DataRaceProcessor basic Test") {
    auto scheduler = SCHEDULE(
        DATA_RACE(MyDatas, 5, 
            SEQUENCE(
                PROCESS(DataWriteAlgo),
                PROCESS(DataReadAlgo)
            )
        )
    );
    
    DataContext dataCtx;
    dataCtx.Create<TestTracker>();
    dataCtx.Create<MyDatas>(1, 2, 3, 4, 5);

    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);

    auto tracker = dataCtx.Fetch<TestTracker>();
    REQUIRE(tracker->Size() > 5);
    REQUIRE(tracker->Size() <= 10);

    REQUIRE(tracker->HasValue("DataWriteAlgo", 1));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 2));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 3));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 4));
    REQUIRE(tracker->HasValue("DataWriteAlgo", 5));

    REQUIRE(tracker->HasKey("DataReadAlgo"));
}

TEST_CASE("DataRaceProcessor complex Test") {
    auto scheduler = SCHEDULE(
        SEQUENCE(
            PROCESS(MockAlgo1),
            DATA_RACE(MyDatas, 3, 
                SEQUENCE(
                    PROCESS(MockAlgo2),
                    PROCESS(DataWriteAlgo),
                    PROCESS(DataReadAlgo)
                )
            ),
            PROCESS(MockAlgo3)
        )
    );

    scheduler.AddTracker(std::make_unique<TimingTracker>());

    DataContext dataCtx;
    dataCtx.Create<TestTracker>();
    dataCtx.Create<MyDatas>(1, 2, 3);

    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);

    scheduler.Dump();

    auto tracker = dataCtx.Fetch<TestTracker>();
    REQUIRE(tracker->Size() > 6);
    REQUIRE(tracker->Size() <= 11);

    REQUIRE(tracker->KeyAt("MockAlgo1", 0));
    REQUIRE(tracker->KeyAt("MockAlgo3", tracker->Size() - 1));

    REQUIRE(tracker->HasKey("MockAlgo2")); 
    REQUIRE(tracker->HasKey("DataWriteAlgo")); 
    REQUIRE(tracker->HasKey("DataReadAlgo")); 
}