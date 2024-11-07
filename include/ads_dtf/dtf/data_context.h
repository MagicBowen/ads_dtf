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
        return OptPtr<const DTYPE>{manager_.GetConst<DTYPE, USER>(span)};
    }

    template<typename DTYPE, LifeSpan span, typename USER>
    auto Get(const USER* user) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Get<DTYPE, USER>(span)};
    }

    template<typename DTYPE, LifeSpan span, typename USER, typename... ARGs>
    auto Create(const USER*, ARGs&&... args) -> OptPtr<DTYPE> {
        return OptPtr<DTYPE>{manager_.Create<DTYPE, USER>(span, std::forward<ARGs>(args)...)};
    }

    template<typename DTYPE, LifeSpan span, typename USER>
    void Destroy(const USER*) {
        manager_.Destroy<DTYPE, USER>(span);
    }

private:
    DataManager& manager_;
};

}

#endif
