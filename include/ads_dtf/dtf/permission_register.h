/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_TYPE_REGISTER_H
#define DATA_TYPE_REGISTER_H

#include "ads_dtf/dtf/permission.h"
#include "ads_dtf/dtf/data_framework.h"

namespace ads_dtf
{

template <typename USER, typename DTYPE, LifeSpan SPAN>
struct PermissionRegister {
    PermissionRegister() {
        DataFramework::Instance().Register<USER, DTYPE, SPAN, MODE>();
    }
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
    static PermissionRegister<USER, DTYPE, SPAN, AccessMode::Create> reg_##USER##_##DTYPE##_##SPAN##_Create;

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
    static PermissionRegister<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_CreateSync;

#define DECLARE_READ_PERMISSION(USER, SPAN, DTYPE)                  \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::Read;        \
    };                                                              \
    static PermissionRegister<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_Read;

#define DECLARE_WRITE_PERMISSION(USER, SPAN, DTYPE)                 \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        constexpr static AccessMode mode = AccessMode::Write;       \
    };                                                              \
    static PermissionRegister<USER, DTYPE, SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_Write;

}  // namespace ads_dtf

#endif
