/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef OPTIONAL_PTR_H
#define OPTIONAL_PTR_H

#include <shared_mutex>
#include <utility>

namespace ads_dtf
{ 

template<typename T, bool SYNC = false>
class OptionalPtr {
public:
    explicit OptionalPtr(T* ptr) : ptr_(ptr) {}

    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    T* Get() { return ptr_; }

    T* operator->() {
        assert(ptr_ && "OptionalPtr is null. assertion failed.");
        return ptr_;
    }

    T& operator*() { 
        return *ptr_; 
    }

    template<typename Handle, typename Fail>
    void Match(Handle& handle, Fail& fail) {
        if (ptr_) {
            handle(*ptr_);
        } else {
            fail();
        }
    }

    template<typename Handle>
    void Require(Handle& handle) {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        handle(*ptr_);
    }

private:
    T* ptr_;
};

template<typename T>
class OptionalPtr<const T, false> {
public:
    explicit OptionalPtr(const T* ptr) : ptr_(ptr) {}

    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    const T* Get() const { return ptr_; }

    const T* operator->() const {
        assert(ptr_ && "OptionalPtr is null. assertion failed.");
        return ptr_;
    }

    const T& operator*() const { 
        return *ptr_; 
    }

    template<typename Handle, typename Fail>
    void Match(const Handle& handle, const Fail& fail) const {
        if (ptr_) {
            handle(*ptr_);
        } else {
            fail();
        }
    }

    template<typename Handle>
    void Require(const Handle& handle) const {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        handle(*ptr_);
    }

private:
    const T* ptr_;
};

template<typename T>
class OptionalPtr<T, true> {
public:
    OptionalPtr(T* ptr, std::shared_timed_mutex& mtx) 
    : ptr_(ptr), lock_(mtx) {
    }

    ~OptionalPtr() = default;

    OptionalPtr(const OptionalPtr& other) = delete;
    OptionalPtr& operator=(const OptionalPtr& other) = delete;

    OptionalPtr(OptionalPtr&& other) noexcept = default;
    OptionalPtr& operator=(OptionalPtr&& other) noexcept = default;

    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    T* Get() { return ptr_; }

    T* operator->() {
        assert(ptr_ && "OptionalPtr is null. assertion failed.");
        return ptr_;
    }

    T& operator*() { 
        return *ptr_; 
    }

    template<typename Handle, typename Fail>
    void Match(Handle& handle, Fail& fail) {
        if (ptr_) {
            handle(*ptr_);
        } else {
            fail();
        }
    }

    template<typename Handle>
    void Require(Handle& handle) {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        handle(*ptr_);
    }

private:
    std::unique_lock<std::shared_timed_mutex> lock_;
    T* ptr_;
};

template<typename T>
class OptionalPtr<const T, true> {
public:
    OptionalPtr(T* ptr, std::shared_timed_mutex& mtx) 
    : ptr_(ptr), lock_(mtx)  {
    }

    ~OptionalPtr() = default;

    OptionalPtr(const OptionalPtr& other) = delete;
    OptionalPtr& operator=(const OptionalPtr& other) = delete;

    OptionalPtr(OptionalPtr&& other) noexcept = default;
    OptionalPtr& operator=(OptionalPtr&& other) noexcept = default;


    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    const T* Get() const { return ptr_; }

    const T* operator->() const {
        assert(ptr_ && "OptionalPtr is null. assertion failed.");
        return ptr_;
    }

    const T& operator*() const { 
        return *ptr_; 
    }

    template<typename Handle, typename Fail>
    void Match(const Handle& handle, const Fail& fail) const {
        if (ptr_) {
            handle(*ptr_);
        } else {
            fail();
        }
    }

    template<typename Handle>
    void Require(const Handle& handle) const {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        handle(*ptr_);
    }

private:
    std::shared_lock<std::shared_timed_mutex> lock_;
    const T* ptr_;
};

}   // namespace ads_dtf

#endif
