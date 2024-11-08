/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PERMISSION_H
#define PERMISSION_H

#include "ads_dtf/dtf/access_mode.h"
#include "ads_dtf/dtf/life_span.h"
#include "ads_dtf/utils/optional_ptr.h"

namespace ads_dtf
{

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    constexpr static AccessMode mode = AccessMode::None; 
    constexpr static bool sync = false;
};

template<typename DTYPE, LifeSpan SPAN>
struct DtypeInfo {
    constexpr static bool sync = false;
    constexpr static std::size_t capacity = 1;
};

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
