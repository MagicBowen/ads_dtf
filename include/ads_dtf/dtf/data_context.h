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

    template<typename DTYPE, LifeSpan SPAN, typename USER>
    auto GetConst(const USER* user) const {
        return OptPtr<const DTYPE>{manager_.GetConst<USER, DTYPE>(SPAN)};
        // return manager_.GetObj<USER, DTYPE, SPAN>();
    }

    template<typename DTYPE, LifeSpan SPAN, typename USER>
    auto Get(const USER* user) {
        return OptPtr<DTYPE>{manager_.Get<USER, DTYPE>(SPAN)};
        // return manager_.GetObj<USER, DTYPE, SPAN>();
    }

    template<typename DTYPE, LifeSpan SPAN, typename USER, typename... ARGs>
    auto Create(const USER*, ARGs&&... args) {
        return OptPtr<DTYPE>{manager_.Create<USER, DTYPE>(SPAN, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, LifeSpan SPAN, typename USER>
    void Destroy(const USER*) {
        manager_.Destroy<USER, DTYPE>(SPAN);
    }

private:
    DataManager& manager_;
};

}

#endif
