/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "ads_dtf/dtf/data_manager.h"
#include "ads_dtf/utils/opt_ptr.h"

namespace ads_dtf {

struct DataContext {
    DataContext(DataManager& manager) 
    : manager_(manager) {}

    template<typename DTYPE, typename USER>
    auto GetFrameOf(const USER* user) const -> OptPtr<const DTYPE> {
        return OptPtr<const DTYPE>{manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Frame)};
    }

    template<typename DTYPE, typename USER>
    auto GetFrameOfMut(const USER* user) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Frame)};
    }

    template<typename DTYPE, typename USER>
    auto GetCacheOf(const USER* user) const -> OptPtr<const DTYPE> {
        return OptPtr<const DTYPE>{manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Cache)};
    }

    template<typename DTYPE, typename USER>
    auto GetCacheOfMut(const USER* user) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Cache)};
    }

    template<typename DTYPE, typename USER>
    auto GetGlobalOf(const USER* user) const -> OptPtr<const DTYPE> {
        return OptPtr<const DTYPE>{manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Global)};
    }

    template<typename DTYPE, typename USER>
    auto GetGlobalOfMut(const USER* user) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Global)};
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    auto MountFrameOf(const USER*, ARGs&&... args) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Mount<DTYPE, USER>(LifeSpan::Frame, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    auto MountCacheOf(const USER*, ARGs&&... args) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Mount<DTYPE, USER>(LifeSpan::Cache, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    auto MountGlobalOf(const USER*, ARGs&&... args) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Mount<DTYPE, USER>(LifeSpan::Global, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    void UnmountFrameOf(const USER*, ARGs&&... args) {
        manager_.Unmount<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    void UnmountCacheOf(const USER*, ARGs&&... args) {
        manager_.Unmount<DTYPE, USER>(LifeSpan::Cache);
    }
    
private:
    DataManager& manager_;
};

}

#endif
