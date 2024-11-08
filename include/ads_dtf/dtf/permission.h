/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PERMISSION_H
#define PERMISSION_H

#include "ads_dtf/dtf/access_mode.h"
#include "ads_dtf/dtf/life_span.h"
#include "ads_dtf/utils/void_t.h"

namespace ads_dtf
{

//////////////////////////////////////////////////////////////////////////
template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    constexpr static AccessMode mode = AccessMode::None; 
    constexpr static bool sync = false;
};

template<typename USER, typename DTYPE, LifeSpan SPAN, typename = void>
struct has_permission_specialization : std::false_type {};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct has_permission_specialization<USER, DTYPE, SPAN, void_t<decltype(Permission<USER, DTYPE, SPAN>::mode)>> {
    static constexpr bool value = (Permission<USER, DTYPE, SPAN>::mode != AccessMode::None);
};

//////////////////////////////////////////////////////////////////////////
template<typename DTYPE, LifeSpan SPAN>
struct DtypeInfo {
    constexpr static bool sync = false;
    constexpr static std::size_t capacity = 1;
};

template<AccessMode MODE, LifeSpan SPAN, int COUNT>
struct PermissionInfo {
    static constexpr AccessMode mode = MODE;
    static constexpr LifeSpan span = SPAN;
    static constexpr int count = COUNT;
};

//////////////////////////////////////////////////////////////////////////
template<typename USER, typename DTYPE, typename INFO, LifeSpan SPAN, bool Has = has_permission_specialization<USER, DTYPE, SPAN>::value>
struct PermissionInfoUpdate;

template<typename USER, typename DTYPE, typename INFO, LifeSpan SPAN>
struct PermissionInfoUpdate<USER, DTYPE, INFO, SPAN, true> {
    using type = typename std::conditional<
        INFO::count == 0,
        PermissionInfo<Permission<USER, DTYPE, SPAN>::mode, SPAN, 1>,
        typename std::conditional<
            INFO::count == 1,
            PermissionInfo<AccessMode::None, LifeSpan::Max, 2>,
            PermissionInfo<AccessMode::None, LifeSpan::Max, INFO::count + 1>
        >::type
    >::type;
};

template<typename USER, typename DTYPE, typename INFO, LifeSpan SPAN>
struct PermissionInfoUpdate<USER, DTYPE, INFO, SPAN, false> {
    using type = INFO;
};

template<typename USER, typename DTYPE, typename INFO, LifeSpan... SPANs>
struct PermissionOfLifespans;

template<typename USER, typename DTYPE, typename INFO>
struct PermissionOfLifespans<USER, DTYPE, INFO> {
    using type = INFO;
};

template<typename USER, typename DTYPE, typename INFO, LifeSpan First, LifeSpan... Rest>
struct PermissionOfLifespans<USER, DTYPE, INFO, First, Rest...> {
    using updated_info = typename PermissionInfoUpdate<USER, DTYPE, INFO, First>::type;
    using type = typename PermissionOfLifespans<USER, DTYPE, updated_info, Rest...>::type;
};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct PermissionQuery {
private:    
    using InitialInfo = PermissionInfo<AccessMode::None, LifeSpan::Max, 0>;
    using FinalInfo = typename PermissionOfLifespans<USER, DTYPE, InitialInfo, SPAN>::type;
public:
    static constexpr AccessMode mode = FinalInfo::mode;
    static constexpr LifeSpan span = FinalInfo::span;
    static constexpr int count = FinalInfo::count;
};

template<typename USER, typename DTYPE>
struct PermissionQuery<USER, DTYPE, LifeSpan::Max> {
private:    
    using InitialInfo = PermissionInfo<AccessMode::None, LifeSpan::Max, 0>;
    using FinalInfo = typename PermissionOfLifespans<USER, DTYPE, InitialInfo, LifeSpan::Frame, LifeSpan::Cache, LifeSpan::Global>::type;
public:
    static constexpr AccessMode mode = FinalInfo::mode;
    static constexpr LifeSpan span = FinalInfo::span;
    static constexpr int count = FinalInfo::count;
};

//////////////////////////////////////////////////////////////////////////
template <typename USER, typename DTYPE, LifeSpan SPAN>
class return_optional_ptr {
    constexpr static AccessMode mode = Permission<USER, DTYPE, SPAN>::mode;
    constexpr static bool sync = Permission<USER, DTYPE, SPAN>::sync;
public:
    static constexpr bool value = (!sync && ((mode == AccessMode::Write) || (mode == AccessMode::Create)));
};

template <typename USER, typename DTYPE, LifeSpan SPAN>
class return_const_optional_ptr {
    constexpr static AccessMode mode = Permission<USER, DTYPE, SPAN>::mode;
    constexpr static bool sync = Permission<USER, DTYPE, SPAN>::sync;
public:
    static constexpr bool value = (!sync && (mode == AccessMode::Read));
};

template <typename USER, typename DTYPE, LifeSpan SPAN>
class return_sync_write_optional_ptr {
    constexpr static AccessMode mode = Permission<USER, DTYPE, SPAN>::mode;
    constexpr static bool sync = Permission<USER, DTYPE, SPAN>::sync;
public:
    static constexpr bool value = (sync && ((mode == AccessMode::Write) || (mode == AccessMode::Create)));
};

template <typename USER, typename DTYPE, LifeSpan SPAN>
class return_sync_read_optional_ptr {
    constexpr static AccessMode mode = Permission<USER, DTYPE, SPAN>::mode;
    constexpr static bool sync = Permission<USER, DTYPE, SPAN>::sync;
public:
    static constexpr bool value = (sync && (mode == AccessMode::Read));
};

} // namespace ads_dtf

#endif
