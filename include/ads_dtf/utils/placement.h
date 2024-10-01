/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PLACEMENT_H
#define PLACEMENT_H

#include <new>

namespace ads_dtf {

template <typename T>
struct Placement {
    Placement() {
    }

    Placement(const Placement&) = delete;
    Placement& operator=(const Placement&) = delete;

    Placement(Placement&&) = delete;
    Placement& operator=(Placement&&) = delete;

    void* Alloc() {
        return reinterpret_cast<void*>(&mem_);
    }

    void Destroy() {
        GetPointer()->~T();
    }

    T* operator->() {
        return GetPointer();
    }

    const T* operator->() const {
        return GetPointer();
    }

    T& operator*() {
        return *GetPointer();
    }

    const T& operator*() const {
        return *GetPointer();
    }

    T* GetPointer() {
        return reinterpret_cast<T*>(&mem_);
    }

    const T* GetPointer() const {
        return reinterpret_cast<const T*>(&mem_);
    }


    typename std::aligned_storage<sizeof(T), alignof(T)>::type mem_;
};

}

#endif
