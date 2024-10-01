/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "ads_dtf/dtf/data_manager.h"

namespace ads_dtf {

struct DataContext {
    DataContext(DataManager& manager) 
    : manager_(manager) {}

    template<typename DTYPE, typename USER>
    const DTYPE* GetFrameOf(const USER* user) const {
        return manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetFrameOfMul(const USER* user) {
        return manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetCacheOf(const USER* user) const {
        return manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Cache);
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetCacheOfMut(const USER* user) {
        return manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Cache);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetGlobalOf(const USER* user) const {
        return manager_.GetDataPointer<DTYPE, USER>(LifeSpan::Global);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetGlobalOfMut(const USER* user) const {
        return manager_.GetMutDataPointer<DTYPE, USER>(LifeSpan::Global);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountFrameOf(const USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Frame, std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountCacheOf(const USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Cache, std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountGlobalOf(const USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Global, std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* UnmountFrameOf(const USER*, ARGs&&... args) {
        return manager_.Unmount<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* UnmountCacheOf(const USER*, ARGs&&... args) {
        return manager_.Unmount<DTYPE, USER>(LifeSpan::Cache);
    }
    
private:
    DataManager& manager_;
};

}

#endif
