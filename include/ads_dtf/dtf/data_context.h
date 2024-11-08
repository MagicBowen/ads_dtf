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

    template<typename DTYPE, LifeSpan SPAN, typename USER>
    auto Fetch(const USER* user) {
        return manager_.Fetch<USER, DTYPE, SPAN>();
    }

    template<typename DTYPE, LifeSpan SPAN, typename USER, typename... ARGs>
    auto Create(const USER*, ARGs&&... args) {
        return manager_.Create<USER, DTYPE, SPAN>(std::forward<ARGs>(args)...);
    }

    template<typename DTYPE, LifeSpan SPAN, typename USER>
    void Destroy(const USER*) {
        manager_.Destroy<USER, DTYPE, SPAN>();
    }

private:
    DataManager& manager_;
};

}

#endif
