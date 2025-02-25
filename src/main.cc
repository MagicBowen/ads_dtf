#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}


// struct DataContext {
//     DataContext() = default;
//     DataContext(const DataContext* parent) : parent_(parent) {}

//     template<typename T>
//     T* Fetch() {
//         std::lock_guard<std::mutex> lock(mutex_);
//         auto it = data_map_.find(typeid(T));
//         if (it != data_map_.end()) {
//             return std::any_cast<T>(&it->second);
//         }
//         if (parent_) {
//             return parent_->Fetch<T>();
//         }
//         return nullptr;
//     }

//     template<typename T, typename ...Args>
//     T* Create(Args&& ...args) {
//         std::lock_guard<std::mutex> lock(mutex_);
//         auto it = data_map_.find(typeid(T));
//         if (it != data_map_.end()) {
//             return nullptr;
//         }
//         data_map_[typeid(T)] = T(std::forward<Args>(args)...);
//         return std::any_cast<T>(&data_map_[typeid(T)]);
//     }

// private:
//     const DataContext* parent_ = nullptr;
//     std::unordered_map<std::type_index, std::any> data_map_;
//     mutable std::mutex mutex_;
// };

// struct ProcessContext {
//     ProcessContext(DataContext& dataCtx, std::atomic<bool>& stopFlag)
//     : dataCtx_(dataCtx), stopFlag_(stopFlag) {}

//     ProcessContext(DataContext& dataCtx)
//     : dataCtx_(dataCtx), stopFlag_(default_stop_flag_) {}

//     void Stop() { stopFlag_.store(true); }
//     void Resume() { stopFlag_.store(false); }
//     bool TryStop() {
//         bool expected = false;
//         return stopFlag_.compare_exchange_strong(expected, true);
//     }
//     bool IsStopped() const { return stopFlag_.load(); }
//     DataContext& GetDataContext() { return dataCtx_; }

// private:
//     DataContext& dataCtx_;
//     std::atomic<bool>& stopFlag_;
//     std::atomic<bool> default_stop_flag_{false};
// };

// struct InstanceID {
//     int id;
//     InstanceID(int i) : id(i) {}
// };

// template<typename T>
// class DataParallelProcessor : public Processor {
// public:
//     using ProcessorFactory = std::function<std::unique_ptr<Processor>(int)>;

//     DataParallelProcessor(const std::string& name, ProcessorFactory factory)
//         : Processor(name), factory_(std::move(factory)) {}

// private:
//     void Init() override {
//         // 子处理器的Init在创建后手动调用
//     }

//     Status Execute(ProcessContext& ctx) override {
//         DataContext& dataCtx = ctx.GetDataContext();
//         T* data = dataCtx.Fetch<T>();
//         if (!data) {
//             return Status::ERROR;
//         }

//         size_t numInstances = data->size();
//         std::vector<std::future<AsyncResult>> futures;

//         for (size_t i = 0; i < numInstances; ++i) {
//             auto childDataCtx = std::make_unique<DataContext>(&dataCtx);
//             childDataCtx->Create<InstanceID>(i);

//             auto childProcessCtx = std::make_unique<ProcessContext>(*childDataCtx, ctx.stopFlag_);

//             auto processor = factory_(i);
//             processor->Init();

//             futures.emplace_back(std::async(std::launch::async,
//                 [processor = std::move(processor), 
//                  childDataCtx = std::move(childDataCtx),
//                  childProcessCtx = std::move(childProcessCtx)]() mutable {
//                     Status status = processor->Process(*childProcessCtx);
//                     return AsyncResult(processor->GetName(), status);
//                 }));
//         }

//         Status overall = Status::OK;
//         for (auto& fut : futures) {
//             auto result = fut.get();
//             if (result.status != Status::OK) {
//                 overall = result.status;
//             }
//             std::cout << "DataParallelProcessor(" << GetName() << ") process " << result.name 
//                       << " with status: " << result.status << "\n";
//         }

//         return overall;
//     }

// private:
//     ProcessorFactory factory_;
// };

// #define DATA_PARALLEL(T, ...) MakeGroupProcessor<DataParallelProcessor<T>>("DataParallelProcessor", __VA_ARGS__)

// TEST_CASE("DataParallelProcessor Test") {
//     struct MyData {
//         int value;
//     };
//     using MyDataBatch = std::vector<MyData>;

//     struct MyAlgo {
//         void Init() { std::cout << "MyAlgo Init\n"; }

//         void Execute(DataContext& context) {
//             auto* instanceID = context.Fetch<InstanceID>();
//             auto* batch = context.Fetch<MyDataBatch>();
//             if (instanceID && batch && instanceID->id < batch->size()) {
//                 MyData& data = (*batch)[instanceID->id];
//                 data.value *= 2;
//                 std::cout << "Processed instance " << instanceID->id << "\n";
//             }
//         }
//     };

//     auto factory = [](int id) {
//         return MakeProcessor<MyAlgo>("MyAlgo_" + std::to_string(id));
//     };

//     auto scheduler = SCHEDULE(
//         DATA_PARALLEL(MyDataBatch, factory)
//     );

//     DataContext dataCtx;
//     MyDataBatch* batch = dataCtx.Create<MyDataBatch>(3);
//     for (int i = 0; i < 3; ++i) {
//         (*batch)[i].value = i + 1;
//     }

//     Status status = scheduler.Run(dataCtx);
//     REQUIRE(status == Status::OK);

//     for (int i = 0; i < 3; ++i) {
//         REQUIRE((*batch)[i].value == (i + 1) * 2);
//     }
// }