/**
 * Copyright (c) wangbo@joycode.art 2024
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "ads_dtf/dtf/access_controller.h"
#include "ads_dtf/dtf/permission.h"
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
    template<typename USER, typename DTYPE>
    bool Apply(LifeSpan span, AccessMode mode) {
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
    const DTYPE* GetDataPtr(const DataRepo& repo, DataType dtype) const {
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

    template<typename USER, typename DTYPE, LifeSpan SPAN>
    typename std::enable_if<return_optional_ptr<USER, DTYPE, SPAN>::value, OptionalPtr<DTYPE, SyncMode::Relax>>::type
    Fetch() {
        static_assert(SPAN < LifeSpan::Max, "Invalid LifeSpan");
        static_assert((Permission<USER, DTYPE, SPAN>::mode == AccessMode::Write) || 
                      (Permission<USER, DTYPE, SPAN>::mode == AccessMode::Create), "Invalid AccessMode");
        static_assert(!DtypeInfo<DTYPE, SPAN>::sync, "Invalid Sync");

        return OptionalPtr<DTYPE, SyncMode::Relax>(const_cast<DTYPE*>(GetDataPtr<DTYPE>(repos_[enum_id_cast(SPAN)], TypeIdOf<DTYPE>()))); 
    }

    template<typename USER, typename DTYPE, LifeSpan SPAN>
    typename std::enable_if<return_const_optional_ptr<USER, DTYPE, SPAN>::value, OptionalPtr<const DTYPE, SyncMode::Relax>>::type
    Fetch() const {
        static_assert(SPAN < LifeSpan::Max, "Invalid LifeSpan");
        static_assert(Permission<USER, DTYPE, SPAN>::mode == AccessMode::Read, "Invalid AccessMode");
        static_assert(!DtypeInfo<DTYPE, SPAN>::sync, "Invalid Sync");

        return OptionalPtr<const DTYPE, SyncMode::Relax>(GetDataPtr<DTYPE>(repos_[enum_id_cast(SPAN)], TypeIdOf<DTYPE>()));
    }

    template<typename USER, typename DTYPE, LifeSpan SPAN, typename ...ARGs>
    typename std::enable_if<return_optional_ptr<USER, DTYPE, SPAN>::value, OptionalPtr<DTYPE, SyncMode::Relax>>::type
    Create(ARGs&& ...args) {
        static_assert(SPAN < LifeSpan::Max, "Invalid LifeSpan");
        static_assert(Permission<USER, DTYPE, SPAN>::mode == AccessMode::Create, "Invalid AccessMode");
        static_assert(!DtypeInfo<DTYPE, SPAN>::sync, "Invalid Sync");

        DataType dtype = TypeIdOf<DTYPE>();

        DataRepo& repo = repos_[enum_id_cast(SPAN)];
        auto result = repo.find(dtype);
        if (result == repo.end()) {
            std::cout << "Failed to find dtype: " << dtype << std::endl;
            return OptionalPtr<DTYPE, SyncMode::Relax>(nullptr);
        }

        if (result->second->HasConstructed()) {
            result->second->Destroy();
        }

        return OptionalPtr<DTYPE, SyncMode::Relax>(new (result->second->Alloc()) DTYPE(std::forward<ARGs>(args)...));
    }

    template<typename USER, typename DTYPE, LifeSpan SPAN>
    void Destroy(LifeSpan span) {
        static_assert(SPAN < LifeSpan::Max, "Invalid LifeSpan");
        static_assert((Permission<USER, DTYPE, SPAN>::mode == AccessMode::Create) ||
                      (Permission<USER, DTYPE, SPAN>::mode == AccessMode::CreateSync)  , "Invalid AccessMode");

        DataType dtype = TypeIdOf<DTYPE>();

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
