/**
 * Copyright (c) wangbo@joycode.art 2024
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "ads_dtf/dtf/access_controller.h"
#include "ads_dtf/utils/placement.h"
#include "ads_dtf/utils/enum_cast.h"
#include "ads_dtf/utils/auto_construct.h"
#include "ads_dtf/utils/auto_clear.h"
#include <unordered_map>
#include <memory>

namespace ads_dtf
{

struct DataContext;
struct DataFramework;

struct DataManager {
    template<typename DTYPE, typename USER>
    bool Apply(const USER*, LifeSpan span, AccessMode mode) {
        UserId user = TypeIdOf<USER>();
        DataType dtype = TypeIdOf<DTYPE>();

        if (!acl_.Register(user, dtype, span, mode)) {
            return false;
        }

        if (mode == AccessMode::Create) {
            return PlacementDataObject<DTYPE>(dtype, span, mode);
        }
        return true;
    }

private:
    struct DataObjectBase {
        virtual ~DataObjectBase() = default;
        virtual void* Alloc() = 0;
        virtual void Destroy() = 0;
        virtual void Clear() = 0;
        virtual void TryConstruct() = 0;
        virtual bool HasConstructed() const = 0;
        virtual bool IsConstructable() const = 0;
    };

    template<typename DTYPE>
    struct DataObjectPlacement : public DataObjectBase {
        DataObjectPlacement() = default;

        void* Alloc() override {
            constructed_ = true;
            return placement.Alloc();
        }

        void Destroy() override {
            placement.Destroy();
            constructed_ = false;
        }

        void Clear() override {
            if (constructed_) {
                auto_clear(placement.GetPointer());
            }
        }

        bool HasConstructed() const override {
            return constructed_;
        }

        bool IsConstructable() const override {
            return defaultConstructable_;
        }

        void TryConstruct() override {
            constructed_ = auto_construct(placement.GetPointer());
            defaultConstructable_ = constructed_;            
        }

        Placement<DTYPE> placement;
        bool constructed_{false};
        bool defaultConstructable_{false};
    };

    template <typename DTYPE>
    bool PlacementDataObject(DataType dtype, LifeSpan span, AccessMode mode) {

        if (span >= LifeSpan::Max) return false;

        DataRepo& repo = repos_[enum_id_cast(span)];
        auto result = repo.find(dtype);
        if (result != repo.end()) {
            return false;
        }

        auto dataObjPtr = std::make_unique<DataObjectPlacement<DTYPE>>();
        if (!dataObjPtr) {
            return false;
        }

        dataObjPtr->TryConstruct();

        repo.emplace(dtype, std::move(dataObjPtr));
        return true;
    }

private:
    using DataRepo = std::unordered_map<DataType, std::unique_ptr<DataObjectBase>>;

    template<typename DTYPE>
    const DTYPE* GetDataPointerOf(const DataRepo& repo, DataType dtype) const {
        auto result = repo.find(dtype);
        if (result == repo.end()) {
            return nullptr;
        }

        if (!result->second->HasConstructed()) {
            return nullptr;
        }
        auto dataObjPtr = static_cast<DataObjectPlacement<DTYPE>*>(result->second.get());
        return dataObjPtr->placement.GetPointer();
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetDataPointer(LifeSpan span) const {
       if (span >= LifeSpan::Max) return nullptr;

        DataType dtype = TypeIdOf<DTYPE>();

        if (ENABLE_ACCESS_CONTROL) {
            UserId user = TypeIdOf<USER>();
            AccessMode mode = acl_.GetAccessMode(user, dtype, span);
            if (mode != AccessMode::Read) {
                return nullptr;
            }
        }

        return GetDataPointerOf<DTYPE>(repos_[enum_id_cast(span)], dtype);
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetMutDataPointer(LifeSpan span) {
      if (span >= LifeSpan::Max) return nullptr;

        DataType dtype = TypeIdOf<DTYPE>();

        if (ENABLE_ACCESS_CONTROL) {
            UserId user = TypeIdOf<USER>();
            AccessMode mode = acl_.GetAccessMode(user, dtype, span);
            if ((mode == AccessMode::None) || (mode == AccessMode::Read)) {
                return nullptr;
            }
        }

        return const_cast<DTYPE*>(GetDataPointerOf<DTYPE>(repos_[enum_id_cast(span)], dtype));
    }

    template<typename DTYPE, typename USER, typename ...ARGs>
    DTYPE* Create(LifeSpan span, ARGs&& ...args) {
        if (span >= LifeSpan::Max) return nullptr;

        DataType dtype = TypeIdOf<DTYPE>();

        if (ENABLE_ACCESS_CONTROL) {
            UserId user = TypeIdOf<USER>();
            AccessMode mode = acl_.GetAccessMode(user, dtype, span);
            if (mode != AccessMode::Create) {
                return nullptr;
            }
        }

        DataRepo& repo = repos_[enum_id_cast(span)];
        auto result = repo.find(dtype);
        if (result == repo.end()) {
            return nullptr;
        }

        if (result->second->HasConstructed()) {
            result->second->Destroy();
        }

        return new (result->second->Alloc()) DTYPE(std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, typename USER>
    void Destroy(LifeSpan span) {
        if (span >= LifeSpan::Max) return;

        DataType dtype = TypeIdOf<DTYPE>();

        if (ENABLE_ACCESS_CONTROL) {
            UserId user = TypeIdOf<USER>();
            AccessMode mode = acl_.GetAccessMode(user, dtype, span);
            if (mode != AccessMode::Destroy) {
                return;
            }
        }

        DataRepo& repo = repos_[enum_id_cast(span)];
        auto result = repo.find(dtype);
        if (result == repo.end()) {
            return;
        }

        if (result->second->HasConstructed()) {
            result->second->Destroy();
        }
    }

    void ResetRepo(LifeSpan span);

private:
    AccessController acl_;
    static constexpr bool ENABLE_ACCESS_CONTROL = true;

private:
    DataRepo repos_[enum_id_cast(LifeSpan::Max)];

private:
    friend struct DataFramework;
    friend struct DataContext;
};

} // namespace ads_dtf

#endif
