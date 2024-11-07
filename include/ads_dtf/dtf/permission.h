/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PERMISSION_H
#define PERMISSION_H

#include "ads_dtf/dtf/data_framework.h"

namespace ads_dtf
{

template <typename USER, typename DTYPE, LifeSpan SPAN>
struct PermissionRegistrar {
    PermissionRegistrar() {
        DataFramework::Instance().Register<USER, DTYPE, SPAN, MODE>();
    }
};

template<typename DTYPE, LifeSpan SPAN>
struct DtypeInfo {
    using dtype = DTYPE;
    using lspan = SPAN;
    constexpr static bool sync = false;
    constexpr static std::size_t capacity = 1;
};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    constexpr static AccessMode mode = MODE;
};

#define DECLARE_CREATE_PERMISSION(USER, SPAN, DTYPE, CAPACITY)      \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::Create;      \
    };                                                              \
    template<>                                                      \
    struct DtypeInfo<DTYPE, SPAN> {                                 \
        constexpr static bool sync = false;                         \
        constexpr static std::size_t capacity = CAPACITY;           \
    };                                                              \
    static PermissionRegistrar<USER, DTYPE, SPAN, AccessMode::Create> reg_##USER##_##DTYPE##_##SPAN##_Create;

#define DECLARE_CREATE_SYNC_PERMISSION(USER, SPAN, DTYPE, CAPACITY) \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::CreateSync;  \
    };                                                              \
    template<>                                                      \
    struct DtypeInfo<DTYPE, SPAN> {                                 \
        constexpr static bool sync = true;                          \
        constexpr static std::size_t capacity = CAPACITY;           \
    };                                                              \
    static PermissionRegistrar<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_CreateSync;

#define DECLARE_READ_PERMISSION(USER, SPAN, DTYPE)                  \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::Read;        \
    };                                                              \
    static PermissionRegistrar<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_Read;

#define DECLARE_WRITE_PERMISSION(USER, SPAN, DTYPE)                 \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::Write;       \
    };                                                              \
    static PermissionRegistrar<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_Write;

} // namespace ads_dtf

#endif
