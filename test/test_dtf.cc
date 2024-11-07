#include "catch2/catch.hpp"
#include "ads_dtf/dtf/data_framework.h"
#include <iostream>

using namespace ads_dtf;

struct AlgoProcessor {
    virtual bool Init(DataManager& data_manager) = 0;
    virtual bool Exec(DataContext& data_context) = 0;
    virtual ~AlgoProcessor() = default;
};

struct FrameMsg {
    int frameId{0};
};

struct FrameData {
    FrameData(const FrameMsg* msg) : msg(msg) {}
    const FrameMsg* msg{nullptr};
};

struct ProcessData {
    ProcessData(int value) : value(value) {}
    int value{0};
};

struct DeliveryData {
    int result{0};
};

struct FrameRecvProcessor : AlgoProcessor {
    bool Init(DataManager& manager) override {
        if (!manager.Apply<FrameRecvProcessor, FrameData>(LifeSpan::Frame, AccessMode::Create)) {
            std::cerr << "Failed to apply mount of FrameData\n";
            return false;
        }
        return true;
    }

    bool Exec(DataContext& context) override {
        static FrameMsg msg;
        msg.frameId++;

        auto frame_data = context.Create<FrameData, LifeSpan::Frame>(this, &msg);
        if (!frame_data) {
            std::cerr << "Failed to mount FrameData\n";
            return false;
        }

        std::cout << "Create Frame: " << frame_data->msg->frameId << "\n";
        return true;
    }
};

struct CalcProcessor : AlgoProcessor {
    bool Init(DataManager& manager) override {
        if (!manager.Apply<CalcProcessor, FrameData>(LifeSpan::Frame, AccessMode::Read)) {
            std::cerr << "Failed to apply read of FrameData\n";
            return false;
        }

        if (!manager.Apply<CalcProcessor, ProcessData>(LifeSpan::Frame, AccessMode::Create)) {
            std::cerr << "Failed to apply mount of ProcessData\n";
            return false;
        }
        return true;
    }

    bool Exec(DataContext& context) override {
        auto frame_data = context.GetConst<FrameData, LifeSpan::Frame>(this);
        if (!frame_data) {
            std::cerr << "Failed to get FrameData\n";
            return false;
        }

        auto process_data = context.Create<ProcessData, LifeSpan::Frame>(this, frame_data->msg->frameId + 1);
        if (!process_data) {
            std::cerr << "Failed to mount ProcessData\n";
            return false;
        }

        std::cout << "Process Frame: " << frame_data->msg->frameId << "\n";
        std::cout << "Process Data: " << process_data->value << "\n";

        return true;
    }
};

struct DeliveryProcessor : AlgoProcessor {
    bool Init(DataManager& manager) override {
        if (!manager.Apply<DeliveryProcessor, FrameData>(LifeSpan::Frame, AccessMode::Read)) {
            std::cerr << "Failed to apply read of FrameData\n";
            return false;
        }
        
        if (!manager.Apply<DeliveryProcessor, ProcessData>(LifeSpan::Frame, AccessMode::Read)) {
            std::cerr << "Failed to apply read of ProcessData\n";
            return false;
        }

        if (!manager.Apply<DeliveryProcessor, DeliveryData>(LifeSpan::Frame, AccessMode::Create)) {
            std::cerr << "Failed to apply mount of DeliveryData\n";
            return false;
        }
        return true;
    }

    bool Exec(DataContext& context) override {
        auto frame_data = context.GetConst<FrameData, LifeSpan::Frame>(this);
        if (!frame_data) {
            std::cerr << "Failed to get FrameData\n";
            return false;
        }

        auto process_data = context.GetConst<ProcessData, LifeSpan::Frame>(this);
        if (!process_data) {
            std::cerr << "Failed to mount ProcessData\n";
            return false;
        }

        auto delivery_data = context.Get<DeliveryData, LifeSpan::Frame>(this);
        if (!delivery_data) {
            std::cerr << "Failed to get DeliveryData\n";
            return false;
        }

        delivery_data->result = process_data->value * 2 + frame_data->msg->frameId;

        std::cout << "delivery data result: " << delivery_data->result << "\n";
        return true;
    }
};

SCENARIO("Data Tree Framework Test") {
    auto& manager = DataFramework::Instance().GetManager();
    auto& context = DataFramework::Instance().GetContext();

    FrameRecvProcessor recvProcessor;
    CalcProcessor calcProcessor;
    DeliveryProcessor dlvrProcessor;

    REQUIRE(recvProcessor.Init(manager));
    REQUIRE(calcProcessor.Init(manager));
    REQUIRE(dlvrProcessor.Init(manager));

    REQUIRE(recvProcessor.Exec(context));
    REQUIRE(calcProcessor.Exec(context));
    REQUIRE(dlvrProcessor.Exec(context));

    DataFramework::Instance().ResetRepo(LifeSpan::Frame);

    REQUIRE(recvProcessor.Exec(context));
    REQUIRE(calcProcessor.Exec(context));
    REQUIRE(dlvrProcessor.Exec(context));
}
