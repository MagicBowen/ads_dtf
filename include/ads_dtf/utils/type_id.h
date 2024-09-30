/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef TYPE_ID_H
#define TYPE_ID_H

namespace ads_dtf {

template <typename T>
struct TypeIdGenerator {
  constexpr static char ID = 0;
};

// should be removed in C++17
template <typename T> constexpr char TypeIdGenerator<T>::ID;

using TypeId = const void*;

template <typename T>
constexpr inline TypeId TypeIdOf() {
    return &TypeIdGenerator<T>::ID;
}

}

#endif
