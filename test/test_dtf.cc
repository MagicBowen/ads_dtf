#include "catch2/catch.hpp"
#include "ads_dtf/dtf/data_framework.h"
#include "ads_dtf/dtf/permission_register.h"
#include <iostream>

using namespace ads_dtf;

//////////////////////////////////////////////////////////////////
struct FrameMsg {
    int frameId{0};
};

struct FrameData {
    FrameData(const FrameMsg* msg) : msg(msg) {}
    const FrameMsg* msg{nullptr};
    int value{0};
};

struct ProcessData {
    ProcessData(int value) : value(value) {}
    int value{0};
};

struct DeliveryData {
    int result{0};
};

//////////////////////////////////////////////////////////////////
struct AlgoProcessor {
    // virtual bool Init(DataManager& data_manager) = 0;
    virtual bool Exec(DataContext& data_context) = 0;
    virtual ~AlgoProcessor() = default;
};

//////////////////////////////////////////////////////////////////
struct FrameRecvProcessor : AlgoProcessor {
    bool Exec(DataContext& context) override;
};

PERMISSION_REGISTER_FOR_CREATE(FrameRecvProcessor, Frame, FrameData, 1);

//////////////////////////////////////////////////////////////////
struct CalcProcessor : AlgoProcessor {
    bool Exec(DataContext& context) override;
};

PERMISSION_REGISTER_FOR_CREATE(CalcProcessor, Frame, ProcessData, 1);
PERMISSION_REGISTER_FOR_CREATE(CalcProcessor, Cache, ProcessData, 1);
PERMISSION_REGISTER_FOR_READ(CalcProcessor, Frame, FrameData);

//////////////////////////////////////////////////////////////////
struct DeliveryProcessor : AlgoProcessor {
    bool Exec(DataContext& context) override;
};

PERMISSION_REGISTER_FOR_CREATE(DeliveryProcessor, Frame, DeliveryData, 1);
PERMISSION_REGISTER_FOR_WRITE(DeliveryProcessor, Frame, FrameData);
PERMISSION_REGISTER_FOR_READ(DeliveryProcessor, Frame, ProcessData);
PERMISSION_REGISTER_FOR_READ(DeliveryProcessor, Cache, ProcessData);

////////////////////////////////////////////////////////////////////////////
bool FrameRecvProcessor::Exec(DataContext& context) {
    static FrameMsg msg;
    msg.frameId++;

    auto frame_data = context.Create<FrameData>(this, &msg);
    if (!frame_data) {
        std::cerr << "Failed to mount FrameData\n";
        return false;
    }

    std::cout << "Create Frame: " << frame_data->msg->frameId << "\n";
    return true;
}

bool CalcProcessor::Exec(DataContext& context) {
    auto frame_data = context.Fetch<FrameData>(this);
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

bool DeliveryProcessor::Exec(DataContext& context) {
    auto frame_data = context.Fetch<FrameData>(this);
    if (!frame_data) {
        std::cerr << "Failed to get FrameData\n";
        return false;
    }

    frame_data->value = 100;

    auto process_data = context.Fetch<ProcessData, LifeSpan::Frame>(this);
    if (!process_data) {
        std::cerr << "Failed to mount ProcessData\n";
        return false;
    }

    auto delivery_data = context.Fetch<DeliveryData>(this);
    if (!delivery_data) {
        std::cerr << "Failed to get DeliveryData\n";
        return false;
    }

    delivery_data->result = process_data->value * 2 + frame_data->msg->frameId;

    std::cout << "delivery data result: " << delivery_data->result << "\n";
    return true;
}

////////////////////////////////////////////////////////////////////////////
SCENARIO("Data Tree Framework Test") {
    auto& manager = DataFramework::Instance().GetManager();
    auto& context = DataFramework::Instance().GetContext();

    FrameRecvProcessor recvProcessor;
    CalcProcessor calcProcessor;
    DeliveryProcessor dlvrProcessor;

    REQUIRE(recvProcessor.Exec(context));
    REQUIRE(calcProcessor.Exec(context));
    REQUIRE(dlvrProcessor.Exec(context));

    DataFramework::Instance().ResetRepo(LifeSpan::Frame);

    REQUIRE(recvProcessor.Exec(context));
    REQUIRE(calcProcessor.Exec(context));
    REQUIRE(dlvrProcessor.Exec(context));
}
