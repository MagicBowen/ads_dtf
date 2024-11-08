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
        constexpr static bool sync = false;                         \
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
        constexpr static AccessMode mode = AccessMode::Create;      \
        constexpr static bool sync = true;                          \
    };                                                              \
    template<>                                                      \
    struct DtypeInfo<DTYPE, LifeSpan::SPAN> {                       \
        constexpr static bool sync = true;                          \
        constexpr static std::size_t capacity = CAPACITY;           \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Create> reg_##USER##_##DTYPE##_##SPAN##_Create_Sync

#define PERMISSION_REGISTER_FOR_READ(USER, SPAN, DTYPE)             \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Read;        \
        constexpr static bool sync = false;                         \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Read> reg_##USER##_##DTYPE##_##SPAN##_Read

#define PERMISSION_REGISTER_FOR_READ_SYNC(USER, SPAN, DTYPE)        \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Read;        \
        constexpr static bool sync = true;                          \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Read> reg_##USER##_##DTYPE##_##SPAN##_Read_Sync

#define PERMISSION_REGISTER_FOR_WRITE(USER, SPAN, DTYPE)            \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Write;       \
        constexpr static bool sync = false;                         \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Write> reg_##USER##_##DTYPE##_##SPAN##_Write

#define PERMISSION_REGISTER_FOR_WRITE_SYNC(USER, SPAN, DTYPE)       \
    template<>                                                      \
    struct Permission<USER, DTYPE, LifeSpan::SPAN> {                \
        constexpr static AccessMode mode = AccessMode::Write;       \
        constexpr static bool sync = true;                          \
    };                                                              \
    static PermissionRegister<USER, DTYPE, LifeSpan::SPAN, AccessMode::Write> reg_##USER##_##DTYPE##_##SPAN##_Write_Sync

}  // namespace ads_dtf

#endif
