/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "ads_dtf/dtf/data_manager.h"
#include <cassert>

namespace ads_dtf {

struct DataContext {
    template<typename DTYPE, typename USER>
    const DTYPE* GetFrameOf(USER* user) const {
        return manager_.GetDataPointer<DTYPE>(user, LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetFrameOfMul(USER* user) {
        return manager_.GetMulDataPointer<DTYPE>(user, LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetCacheOf(USER* user) const {
        return manager_.GetDataPointer<DTYPE>(user, LifeSpan::Cache);
    }

    template<typename DTYPE, typename USER>
    DTYPE* GetCacheOfMut(USER* user) {
        return manager_.GetMulDataPointer<DTYPE>(user, LifeSpan::Cache);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetGlobalOf(USER* user) const {
        return manager_.GetDataPointer<DTYPE>(user, LifeSpan::Global);
    }

    template<typename DTYPE, typename USER>
    const DTYPE* GetGlobalOfMut(USER* user) const {
        return manager_.GetMutDataPointer<DTYPE>(user, LifeSpan::Global);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountFrameOf(USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountCacheOf(USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Cache);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* MountGlobalOf(USER*, ARGs&&... args) {
        return manager_.Mount<DTYPE, USER>(LifeSpan::Global);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* UnmountFrameOf(USER*, ARGs&&... args) {
        return manager_.Unmount<DTYPE, USER>(LifeSpan::Frame);
    }

    template<typename DTYPE, typename USER, typename... ARGs>
    DTYPE* UnmountCacheOf(USER*, ARGs&&... args) {
        return manager_.Unmount<DTYPE, USER>(LifeSpan::Cache);
    }

private:
    DataContext(DataManager& manager) 
    : manager_(manager) {}

private:
    DataManager& manager_;
};

}

#endif
