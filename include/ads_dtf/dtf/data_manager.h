/**
 * Copyright (c) wangbo@joycode.art 2024
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "ads_dtf/dtf/access_controller.h"
#include "ads_dtf/utils/placement.h"
#include "ads_dtf/utils/enum_cast.h"
#include <unordered_map>
#include <memory>

namespace ads_dtf
{

struct DataContext;

struct DataManager {
    template<typename DTYPE, typename USER>
    bool Apply(USER*, LifeSpan span, AccessMode mode) {
        UserId user = TypeIdOf<USER>();
        DataType dtype = TypeIdOf<DTYPE>();

        if (!acl_.Register(user, dtype, span, mode)) {
            return false;
        }

        if (mode == AccessMode::Mount) {
            return PlacementData<DTYPE>(dtype, span, mode);
        }
    }

private:
    struct PlacementBase {
        virtual ~PlacementBase() = default;
        virtual bool IsConstructed() const = 0;
        virtual void Destroy() = 0;
    };

    template<typename DTYPE>
    struct PlacementDType : public PlacementBase {

        PlacementDType() : placement() {}

        bool IsConstructed() const override {
            return placement.IsConstructed();
        }

        void Destroy() override {
            placement.Destroy();
        }

        Placement<DTYPE> placement;
    };

    template <typename DTYPE>
    bool PlacementData(DataType dtype, LifeSpan span, AccessMode mode) {

        if (span >= LifeSpan::Max) return nullptr;

        DataObjects& objects = dataObjects_[enum_id_cast(span)];
        auto it = objects->find(dtype);
        if (it != objects->end()) {
            return false;
        }

        auto placementPtr = std::make_unique<PlacementDType<DTYPE>>();
        if (!placementPtr) {
            return false;
        }

        if constexpr (std::is_default_constructible_v<DTYPE>) {
            placementPtr->placement.New();
        }
        objects->emplace(dtype, std::move(placementPtr));
        return true;
    }

private:
    template<typename DTYPE, typename USER>
    const DTYPE* GetDataPointer(LifeSpan span) const {
       if (span >= LifeSpan::Max) return nullptr;

        UserId user = TypeIdOf<USER>();
        DataType dtype = TypeIdOf<DTYPE>();

        AccessMode mode = acl_.GetAccessMode(user, dtype, span);
        if (mode != AccessMode::READ) {
            return nullptr;
        }

        DataObjects& objects = dataObjects_[enum_id_cast(span)];
        DataType dtype = TypeIdOf<DTYPE>();

        auto it = objects.find(dtype);
        if (it == objects.end()) {
            return nullptr;
        }

        if (!it->second->IsConstructed()) {
            return nullptr;
        }

        return (static_cast<PlacementDType<DTYPE>*>(it->second.get()))->placement.GetPointer();
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetMutDataPointer(LifeSpan span) {
        return const_cast<DTYPE*>(GetDataPointerConst<DTYPE>(span));
    }

    template<typename DTYPE, typename USER, typename ...ARGs>
    DTYPE* Mount(LifeSpan span, ARGs&& ...args) {
        if (span >= LifeSpan::Max) return nullptr;

        UserId user = TypeIdOf<USER>();
        DataType dtype = TypeIdOf<DTYPE>();

        AccessMode mode = acl_.GetAccessMode(user, dtype, span);
        if (mode != AccessMode::Mount) {
            return nullptr;
        }

        DataObjects& objects = dataObjects_[enum_id_cast(span)];
        auto it = objects.find(dtype);
        if (it == objects.end()) {
            return nullptr;
        }

        if (it->second->IsConstructed()) {
            return nullptr;
        }

        auto placementPtr = static_cast<PlacementDType<DTYPE>*>(it->second.get());
        return placementPtr->placement.New(std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, typename USER>
    void Unmount(LifeSpan span) {
        if (span >= LifeSpan::Max) return;

        UserId user = TypeIdOf<USER>();
        DataType dtype = TypeIdOf<DTYPE>();

        AccessMode mode = acl_.GetAccessMode(user, dtype, span);
        if (mode != AccessMode::Unmount) {
            return;
        }

        DataObjects& objects = dataObjects_[enum_id_cast(span)];
        auto it = objects.find(dtype);
        if (it == objects.end()) {
            return;
        }

        if (it->second->IsConstructed()) {
            it->second->Destroy();
        }
    }

    void Reset(LifeSpan span) {
        if (span >= LifeSpan::Max) return;

        DataObjects& objects = dataObjects_[enum_id_cast(span)];
        for (auto& [type_id, placementPtr] : objects) {
            placementPtr->Destroy();
        }
    }

private:
    AccessController acl_;

private:
    using DataObjects = std::unordered_map<DataType, std::unique_ptr<PlacementBase>>;
    DataObjects dataObjects_[enum_id_cast(LifeSpan::Max)];

private:
    DataManager() = default;
    friend struct DataContext; 
};

} // namespace ads_dtf

#endif
