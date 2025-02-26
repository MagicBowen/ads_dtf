// // 辅助函数和变量 - 用于在线程间传递当前处理的实例ID
// namespace data_parallel_detail {

//     template<typename T>
//     thread_local int currentInstanceId = -1;
    
//     template<typename T>
//     int GetCurrentInstanceId() {
//         return currentInstanceId<T>;
//     }
    
//     template<typename T>
//     void SetCurrentInstanceId(int id) {
//         currentInstanceId<T> = id;
//     }
    
//     // 辅助函数 - 根据实例ID获取数据实例
//     template<typename T, typename ElementType = typename T::value_type>
//     ElementType* FetchInstance(DataContext& context, int instanceId) {
//         auto* dataContainer = context.Fetch<T>();
//         if (!dataContainer || instanceId < 0 || instanceId >= dataContainer->size()) {
//             return nullptr;
//         }
//         return &(*dataContainer)[instanceId];
//     }
    
//     // 辅助函数 - 获取当前线程对应的数据实例
//     template<typename T, typename ElementType = typename T::value_type>
//     ElementType* FetchCurrentInstance(DataContext& context) {
//         int instanceId = GetCurrentInstanceId<T>();
//         return FetchInstance<T, ElementType>(context, instanceId);
//     }
    
// }  // namespace data_parallel_detail
    
// // DataParallelProcessor实现
// template<typename T>
// struct DataParallelProcessor : GroupProcessor {
//     using ProcessorFactory = std::function<std::unique_ptr<Processor>()>;
    
//     DataParallelProcessor(const std::string& name, ProcessorFactory factory)
//     : GroupProcessor(name), factory_(factory) {}
    
// private:
//     void Init() override {
//         // 在Execute阶段再创建处理器实例，因为需要知道数据实例的数量
//     }
    
//     Status Execute(ProcessContext& ctx) override {
//         auto* dataContainer = ctx.GetDataContext().Fetch<T>();
//         if (!dataContainer) {
//             std::cout << "DataParallelProcessor(" << name_ << ") no data container found\n";
//             return Status::ERROR;
//         }
        
//         int numInstances = dataContainer->size();
        
//         // 创建处理器实例
//         processors_.clear();
//         for (int i = 0; i < numInstances; i++) {
//             auto processor = factory_();
//             processor->Init(); // 初始化每个处理器实例
//             processors_.push_back(std::move(processor));
//         }
        
//         // 并行执行处理器实例
//         std::vector<std::future<AsyncResult>> futures;
//         for (int i = 0; i < processors_.size(); i++) {
//             futures.emplace_back(std::async(std::launch::async, [&ctx, i, processor = processors_[i].get()]() {
//                 // 保存当前线程的实例ID
//                 int oldInstanceId = data_parallel_detail::GetCurrentInstanceId<T>();
                
//                 // 设置当前线程处理的实例ID
//                 data_parallel_detail::SetCurrentInstanceId<T>(i);
                
//                 // 处理
//                 AsyncResult result(processor->GetName(), processor->Process(ctx));
                
//                 // 恢复当前线程的实例ID
//                 data_parallel_detail::SetCurrentInstanceId<T>(oldInstanceId);
                
//                 return result;
//             }));
//         }
        
//         Status overall = Status::OK;
//         for (auto& fut : futures) {
//             auto ret = fut.get();
//             if (ret.status != Status::OK) {
//                 overall = ret.status;
//             }
//             std::cout << "DataParallelProcessor(" << name_ << ") process " << ret.name << " with status: " << ret.status << "\n";
//         }
        
//         return overall;
//     }
    
// private:
//     ProcessorFactory factory_;
// };

// // 构造器和宏
// template<typename T>
// std::unique_ptr<Processor> MakeDataParallelProcessor(const std::string& name, std::function<std::unique_ptr<Processor>()> factory) {
//     return std::make_unique<DataParallelProcessor<T>>(name, factory);
// }

// #define DATA_PARALLEL(TYPE, ...) MakeDataParallelProcessor<TYPE>("DataParallelProcessor", [&]() { return __VA_ARGS__; })

// // 测试算法和测试用例
// struct TestData {
//     int value;
// };

// struct DataParallelTestAlgo {
//     void Init() {
//         std::cout << "DataParallelTestAlgo Init\n";
//     }
    
//     void Execute(DataContext& context) {
//         // 获取当前处理的实例ID
//         int instanceId = data_parallel_detail::GetCurrentInstanceId<std::vector<TestData>>();
        
//         // 获取对应的数据实例
//         auto* data = data_parallel_detail::FetchInstance<std::vector<TestData>, TestData>(context, instanceId);
        
//         if (!data) {
//             std::cout << "DataParallelTestAlgo: no data found for instance " << instanceId << "\n";
//             return;
//         }
        
//         std::cout << "DataParallelTestAlgo: processing instance " << instanceId << " with value " << data->value << "\n";
        
//         // 修改数据
//         data->value *= 2;
        
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
//         std::cout << "DataParallelTestAlgo: finished processing instance " << instanceId << ", new value: " << data->value << "\n";
//     }
// };

// TEST_CASE("DataParallelProcessor Basic Test") {
//     // 创建数据上下文
//     DataContext dataCtx;
    
//     // 创建数据实例
//     auto* instances = dataCtx.Create<std::vector<TestData>>(std::vector<TestData>{{1}, {2}, {3}, {4}, {5}});
    
//     // 创建调度器
//     auto scheduler = SCHEDULE(
//         DATA_PARALLEL(std::vector<TestData>, PROCESS(DataParallelTestAlgo))
//     );
    
//     // 运行调度器
//     auto status = scheduler.Run(dataCtx);
//     REQUIRE(status == Status::OK);
    
//     // 验证结果
//     REQUIRE(instances->size() == 5);
//     REQUIRE((*instances)[0].value == 2);
//     REQUIRE((*instances)[1].value == 4);
//     REQUIRE((*instances)[2].value == 6);
//     REQUIRE((*instances)[3].value == 8);
//     REQUIRE((*instances)[4].value == 10);
// }

// TEST_CASE("DataParallelProcessor Complex Test") {
//     // 创建数据上下文
//     DataContext dataCtx;
    
//     // 创建数据实例
//     auto* instances = dataCtx.Create<std::vector<TestData>>(std::vector<TestData>{{1}, {2}, {3}});
    
//     // 创建调度器 - 使用复合处理器
//     auto scheduler = SCHEDULE(
//         SEQUENCE(
//             PROCESS(MockAlgo1),
//             DATA_PARALLEL(std::vector<TestData>, 
//                 SEQUENCE(
//                     PROCESS(MockAlgo2),
//                     PROCESS(DataParallelTestAlgo),
//                     PROCESS(MockAlgo3)
//                 )
//             ),
//             PROCESS(MockAlgo7)
//         )
//     );
    
//     // 运行调度器
//     auto status = scheduler.Run(dataCtx);
//     REQUIRE(status == Status::OK);
    
//     // 验证结果
//     REQUIRE(instances->size() == 3);
//     REQUIRE((*instances)[0].value == 2);
//     REQUIRE((*instances)[1].value == 4);
//     REQUIRE((*instances)[2].value == 6);
// }