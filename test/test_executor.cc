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
#include <chrono>
#include <string>
#include <any>

////////////////////////////////////////////////////////////////////

// 数据上下文，支持具体的算法类进行数据存取
struct DataContext {
    template<typename T>
    T* Fetch() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_map_.find(typeid(T));
        if (it == data_map_.end()) {
            return nullptr;
        }
        return std::any_cast<T>(&it->second);
    }

    template<typename T, typename ...Args>
    T* Create(Args&& ...args) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_map_.find(typeid(T));
        if (it != data_map_.end()) {
            return nullptr;
        }
        data_map_[typeid(T)] = T(std::forward<Args>(args)...);
        return std::any_cast<T>(&data_map_[typeid(T)]);
    }

private:
    std::unordered_map<std::type_index, std::any> data_map_;
    mutable std::mutex mutex_;
};

// 业务格式各样的算法类满足如下原型
// struct Algorithm {
//     void Init() {
//     }
//     void Execute(DataContext& context) {
//     }
// };

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

// 执行器的上下文
struct ProcessContext {
    static ProcessContext CreateSubContext(ProcessContext& parentCtx) {
        return ProcessContext{parentCtx.GetDataContext(), &parentCtx.stopFlag_};
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

    DataContext& GetDataContext() {
        return dataCtx_;
    }
private:
    ProcessContext(DataContext& dataCtx, const std::atomic<bool>* parentStopFlag)
    : dataCtx_(dataCtx), parentStopFlag_(parentStopFlag) {}

private:
    DataContext& dataCtx_;
    std::atomic<bool> stopFlag_{false};
    const std::atomic<bool>* parentStopFlag_{nullptr};
};

////////////////////////////////////////////////////////////////////

// 执行器接口
struct Processor {
    Processor(const std::string& name)
    : name_(name) {}

    const std::string& GetName() const {
        return name_;
    }

    virtual void Init() = 0;
    Status Process(ProcessContext& ctx) {
        if (ctx.IsStopped()) {
            std::cout << "Processor " << name_ << " is skiped!\n";
            return Status::CANCELLED;
        }
        std::cout << "Processor " << name_ << " is processing...\n";
        auto status = Execute(ctx);
        std::cout << "Processor " << name_ << " is finished with status: " << status << "\n";
        return status;
    }

    virtual ~Processor() = default;

private:
    virtual Status Execute(ProcessContext& ctx) = 0;

protected:
    std::string name_;
};

// 基本执行器，对业务算法的适配
template<typename ALGO>
struct AlgoProcessor : Processor {
    using Processor::Processor;

private:
    void Init() override {
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
    void Init() override {
        for (auto& processor : processors_) {
            processor->Init();
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
            std::cout << "SequentialProcessor(" << name_ << ") process " << processor->GetName() << " with status: " << status << "\n";
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
            std::cout << "ParallelProcessor(" << name_ << ") process " << ret.name << " with status: " << ret.status << "\n";
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
            std::cout << "RaceProcessor(" << name_ << ") process " << ret.name << " with status: " << ret.status << "\n";
        }
        return overall;
    }
};

////////////////////////////////////////////////////////////////////

// Processor 调度器
struct Scheduler {
    Scheduler(std::unique_ptr<Processor> rootProcessor)
    : rootProcessor_(std::move(rootProcessor)) {
        rootProcessor_->Init();
    }

    Status Run(DataContext& dataCtx) {
        std::cout << "........................Scheduler is running........................\n";
        ProcessContext processCtx{dataCtx};
        auto ret = rootProcessor_->Process(processCtx);
        std::cout << "........................Scheduler is finished with status: " << ret << "........................\n";
        return ret;
    }

private:
    std::unique_ptr<Processor> rootProcessor_;
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


////////////////////////////////////////////////////////////////////

// Processor 构造器宏

#define PROCESS(ALGO) MakeProcessor<ALGO>(#ALGO)
#define SEQUENCE(...) MakeGroupProcessor<SequentialProcessor>("SequentialProcessor", __VA_ARGS__)
#define PARALLEL(...) MakeGroupProcessor<ParallelProcessor>("ParallelProcessor", __VA_ARGS__)
#define RACE(...)     MakeGroupProcessor<RaceProcessor>("RaceProcessor", __VA_ARGS__)

#define SCHEDULE(...) Scheduler(__VA_ARGS__)

////////////////////////////////////////////////////////////////////

// 模拟业务算法
struct MockAlgo {
    MockAlgo(std::string algoName, std::chrono::milliseconds sleepTime = std::chrono::milliseconds(0))
    : algoName_{algoName}, sleepTime_{sleepTime} 
    {}

    void Init() {
        std::cout << "Algorithm " << algoName_ << " Init\n";
    }

    void Execute(DataContext& context) {
        std::cout << "Algorithm " << algoName_ << " is processing...\n";
        std::this_thread::sleep_for(sleepTime_);
        std::cout << "Algorithm " << algoName_ << " is finished\n";
    }

private:
    std::chrono::milliseconds sleepTime_;
    std::string algoName_;
};

struct MockAlgo1 : MockAlgo {
    MockAlgo1() : MockAlgo("MockAlgo1", std::chrono::milliseconds(100)) {}
};

struct MockAlgo2 : MockAlgo {
    MockAlgo2() : MockAlgo("MockAlgo2", std::chrono::milliseconds(200)) {}
};

struct MockAlgo3 : MockAlgo {
    MockAlgo3() : MockAlgo("MockAlgo3", std::chrono::milliseconds(300)) {}
};

struct MockAlgo4 : MockAlgo {
    MockAlgo4() : MockAlgo("MockAlgo4", std::chrono::milliseconds(400)) {}
};

struct MockAlgo5 : MockAlgo {
    MockAlgo5() : MockAlgo("MockAlgo5", std::chrono::milliseconds(500)) {}
};

struct MockAlgo6 : MockAlgo {
    MockAlgo6() : MockAlgo("MockAlgo6", std::chrono::milliseconds(600)) {}
};

struct MockAlgo7 : MockAlgo {
    MockAlgo7() : MockAlgo("MockAlgo7", std::chrono::milliseconds(700)) {}
};

////////////////////////////////////////////////////////////////////

TEST_CASE("Processor Test") {
    auto scheduler = SCHEDULE(
        SEQUENCE(
            PROCESS(MockAlgo1),
            PARALLEL(
                PROCESS(MockAlgo2),
                PROCESS(MockAlgo3),
                RACE(
                    PROCESS(MockAlgo4),
                    SEQUENCE(
                        PROCESS(MockAlgo5),
                        PROCESS(MockAlgo6)
                    )
                )
            ),
            PROCESS(MockAlgo7)
        )
    );
    
    DataContext dataCtx;
    auto status = scheduler.Run(dataCtx);
    REQUIRE(status == Status::OK);
}