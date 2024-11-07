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

    template<typename DTYPE, LifeSpan span, typename USER>
    auto GetConst(const USER* user) const -> OptPtr<const DTYPE> {
        return OptPtr<const DTYPE>{manager_.GetConst<USER, DTYPE>(span)};
    }

    template<typename DTYPE, LifeSpan span, typename USER>
    auto Get(const USER* user) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Get<USER, DTYPE>(span)};
    }

    template<typename DTYPE, LifeSpan span, typename USER, typename... ARGs>
    auto Create(const USER*, ARGs&&... args) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Create<USER, DTYPE>(span, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, LifeSpan span, typename USER>
    void Destroy(const USER*) {
        manager_.Destroy<USER, DTYPE>(span);
    }

private:
    DataManager& manager_;
};

}

#endif
