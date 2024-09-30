#include "catch2/catch.hpp"
#include "ads_dtf/dtf/data_context.h"
#include <iostream>

using namespace ads_dtf;

struct AlgoProcessor {
    virtual bool Init(DataManager& data_manager) = 0;
    virtual bool Exec(DataContext& data_context) = 0;
    virtual ~AlgoProcessor() = default;
};

// 示例数据结构
struct InputData {
    int value = 0;
};

struct OutputData {
    int result = 0;
};

// 示例 AlgoProcessor 实现
struct ExampleProcessor : public AlgoProcessor {
    bool Init(DataManager& data_manager) override {
        // 注册需要只读访问 Frame 数据
        if (!data_manager.Apply<InputData, ExampleProcessor>(LifeSpan::Frame, AccessMode::Read)) {
            std::cerr << "Failed to apply InputData\n";
            return false;
        }

        // 注册需要只写访问 OutputData，必须通过 Mount 来构造数据
        if (!data_manager.Apply<OutputData, ExampleProcessor>(LifeSpan::Frame, AccessMode::Write)) {
            std::cerr << "Failed to apply OutputData\n";
            return false;
        }

        // 注册一个 Cache 级别的只读数据
        if (!data_manager.Apply<int, ExampleProcessor>(LifeSpan::Cache, AccessMode::Read)) {
            std::cerr << "Failed to apply cached int\n";
            return false;
        }

        return true;
    }

    bool Exec(DataContext& context) override {
        // 获取 Frame 级别的 InputData
        const InputData* input = context.GetConst<InputData, ExampleProcessor>(LifeSpan::Frame);
        if (!input) {
            std::cerr << "Failed to get InputData\n";
            return false;
        }

        // 获取 Frame 级别的 OutputData
        OutputData* output = context.Get<OutputData, ExampleProcessor>(LifeSpan::Frame);
        if (!output) {
            std::cerr << "Failed to get OutputData\n";
            return false;
        }

        // 执行计算
        output->result = input->value * 2;

        // 访问 Cache 级别的数据
        const int* cached_value = context.GetConst<int, ExampleProcessor>(LifeSpan::Cache);
        if (cached_value) {
            std::cout << "Cached value: " << *cached_value << "\n";
        } else {
            std::cout << "No cached value.\n";
        }

        return true;
    }
};

SCENARIO("Data Tree Framework Test") {
   // 创建 DataManager
    DataManager data_manager;

    // 创建 AlgoProcessor
    ExampleProcessor processor;

    // 初始化 processor
    REQUIRE(!processor.Init(data_manager));

    // 模拟接收一帧数据并调用 ResetFrame
    {
        // 创建一个新的帧数据映射
        std::unordered_map<TypeId, DataPtr> new_frame_data;

        // 例如，创建 InputData 并添加到帧数据中
        InputData input;
        input.value = 10;
        new_frame_data[TypeIdOf<InputData>()] = reinterpret_cast<DataPtr>(&input);

        // 调用 ResetFrame 更新帧数据
        data_manager.ResetFrame(new_frame_data);
    }

    // 执行 processor
    {
        DataContext exec_context = data_manager.GetContext();
        REQUIRE(!processor.Exec(exec_context));
    }

    // 需要进行 Mount 操作来构造 OutputData
    {
        DataContext exec_context = data_manager.GetContext();
        OutputData* output = exec_context.Get<OutputData, ExampleProcessor>(LifeSpan::Frame);
        REQUIRE(output != nullptr);
    }

    // 重置帧数据准备下一帧
    data_manager.ResetFrame({});
}
