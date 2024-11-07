/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_FRAMEWORK_H
#define DATA_FRAMEWORK_H

#include "ads_dtf/dtf/data_context.h"

namespace ads_dtf {

struct DataFramework {
    static DataFramework& Instance() {
        static DataFramework instance;
        return instance;
    }

    template<typename USER, typename DTYPE, LifeSpan SPAN, AccessMode MODE>
    void Register() {
        manager_.Apply<DTYPE, USER>(SPAN, MODE);
    }

    DataManager& GetManager() {
        return manager_;
    }

    DataContext& GetContext() {
        return context_;
    }

    void ResetRepo(LifeSpan span) {
        manager_.ResetRepo(span);
    }

    DataFramework(const DataFramework&) = delete;
    DataFramework& operator=(const DataFramework&) = delete;

private:
    DataFramework() 
    : manager_{}, context_{manager_} {
    }

private:
    DataManager manager_;
    DataContext context_;
};

}

#endif
