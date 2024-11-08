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

    template<typename DTYPE, LifeSpan SPAN = LifeSpan::Max, typename USER>
    auto Fetch(const USER* user) {
        static_assert(PermissionQuery<USER, DTYPE, SPAN>::span != LifeSpan::Max, "Invalid access to data of lifespan!");
        return manager_.Fetch<USER, DTYPE, PermissionQuery<USER, DTYPE, SPAN>::span>();
    }

    template<typename DTYPE, LifeSpan SPAN = LifeSpan::Max, typename USER, typename... ARGs>
    auto Create(const USER*, ARGs&&... args) {
        static_assert(PermissionQuery<USER, DTYPE, SPAN>::span != LifeSpan::Max, "Invalid access to data of lifespan!");
        return manager_.Create<USER, DTYPE, PermissionQuery<USER, DTYPE, SPAN>::span>(std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, LifeSpan SPAN = LifeSpan::Max, typename USER>
    void Destroy(const USER*) {
        static_assert(PermissionQuery<USER, DTYPE, SPAN>::span != LifeSpan::Max, "Invalid access to data of lifespan!");
        manager_.Destroy<USER, DTYPE, PermissionQuery<USER, DTYPE, SPAN>::span>();
    }

private:
    DataManager& manager_;
};

}

#endif
