/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef ENUM_CAST_H
#define ENUM_CAST_H

#include <type_traits>

namespace ads_dtf {

template<typename E>
constexpr auto enum_id_cast(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}

}

#endif
