/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DATA_TYPE_REGISTER_H
#define DATA_TYPE_REGISTER_H

#include "ads_dtf/dtf/permission.h"
#include "ads_dtf/dtf/data_framework.h"

namespace ads_dtf
{
////////////////////////////////////////////////////////////////////////////////////////
template <typename USER, typename DTYPE, LifeSpan SPAN, AccessMode MODE>
struct PermissionRegister {
    PermissionRegister() {
        DataFramework::Instance().Register<USER, DTYPE, SPAN, MODE>();
    }
};

//////////////////////////////////////////////////////////////////////////////////////// 
#define PERMISSION_REGISTER_FOR_CREATE(USER, SPAN, DTYPE, CAPACITY) \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Create;      \
    };                                                              \
    template<>                                                      \
    struct DtypeInfo<DTYPE, LifeSpan::SPAN> {                       \
        constexpr static bool sync = false;                         \
        constexpr static std::size_t capacity = CAPACITY;           \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Create> reg_##USER##_##DTYPE##_##SPAN##_Create

#define PERMISSION_REGISTER_FOR_CREATE_SYNC(USER, SPAN, DTYPE, CAPACITY) \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::CreateSync;  \
    };                                                              \
    template<>                                                      \
    struct DtypeInfo<DTYPE, LifeSpan::SPAN> {                       \
        constexpr static bool sync = true;                          \
        constexpr static std::size_t capacity = CAPACITY;           \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::CreateSync> reg_##USER##_##DTYPE##_##SPAN##_CreateSync

#define PERMISSION_REGISTER_FOR_READ(USER, SPAN, DTYPE)             \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Read;        \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Read> reg_##USER##_##DTYPE##_##SPAN##_Read

#define PERMISSION_REGISTER_FOR_WRITE(USER, SPAN, DTYPE)            \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Write;       \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Write> reg_##USER##_##DTYPE##_##SPAN##_Write

}  // namespace ads_dtf

#endif
