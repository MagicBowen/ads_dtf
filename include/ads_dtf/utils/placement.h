/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef PLACEMENT_H
#define PLACEMENT_H

#include <new>

namespace ads_dtf {

template <typename T>
struct Placement {
    Placement() : constructed(false) {}

    ~Placement() {
        Destroy();
    }

    Placement(const Placement&) = delete;
    Placement& operator=(const Placement&) = delete;

    Placement(Placement&&) = delete;
    Placement& operator=(Placement&&) = delete;

    template<typename... Args>
    void New(Args&&... args) {
        if (!IsConstructed()) {
            new (Alloc()) T(std::forward<Args>(args)...);
            constructed = true;
        }
    }

    void Destroy() {
        if (IsConstructed()) {
            GetPointer()->~T();
            constructed = false;
        }
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

    bool IsConstructed() const {
        return constructed;
    }

    T* GetPointer() {
        return reinterpret_cast<T*>(&mem);
    }

    const T* GetPointer() const {
        return reinterpret_cast<const T*>(&mem);
    }

private:
    void* Alloc() {
        return reinterpret_cast<void*>(&mem);
    }

    typename std::aligned_storage<sizeof(T), alignof(T)>::type mem;
    bool constructed;
};

}

#endif
