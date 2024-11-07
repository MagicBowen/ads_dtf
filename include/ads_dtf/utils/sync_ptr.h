/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef SYNC_PTR_H
#define SYNC_PTR_H

#include <shared_mutex>
#include <utility>

namespace ads_dtf
{ 

template <typename T>
struct SyncReadPtr {
    SyncReadPtr(std::shared_mutex& mtx, const T* ptr)
    : lock_(mtx), ptr_(ptr) {
    }

    SyncReadPtr() = delete;
    SyncReadPtr(const SyncReadPtr&) = delete;
    SyncReadPtr& operator=(const SyncReadPtr&) = delete;

    SyncReadPtr(SyncReadPtr&&) = default;
    SyncReadPtr& operator=(SyncReadPtr&&) = default;

    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    T* Get() const { return ptr_; }

    const T* operator->() const { 
        assert(ptr_ && "SyncReadPtr is null. assertion failed.");
        return ptr_; 
    }

    const T& operator*() const { 
        return *ptr_; 
    }

    template<typename FEmpty, typename FNonEmpty>
    void Match(FEmpty empty_func, FNonEmpty non_empty_func) const {
        if (ptr_) {
            non_empty_func(*ptr_);
        } else {
            empty_func();
        }
    }

    template<typename FNonEmpty>
    void Require(FNonEmpty non_empty_func) const {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        non_empty_func(*ptr_);
    }

private:
    std::shared_lock<std::shared_mutex> lock_;
    const T* ptr_;
};

template <typename T>
struct SyncWritePtr {
    SyncWritePtr(std::shared_mutex& mtx, T* ptr)
        : lock_(mtx), ptr_(ptr) {}

    SyncWritePtr() = delete;
    SyncWritePtr(const SyncWritePtr&) = delete;
    SyncWritePtr& operator=(const SyncWritePtr&) = delete;

    SyncWritePtr(SyncWritePtr&&) = default;
    SyncWritePtr& operator=(SyncWritePtr&&) = default;

    explicit operator bool() const { return HasValue(); }
    bool HasValue() const { return ptr_ != nullptr; }
    T* Get() const { return ptr_; }

    T* operator->() { 
        assert(ptr_ && "SyncWritePtr is null. assertion failed.");
        return ptr_; 
    }

    T& operator*() { 
        return *ptr_; 
    }

    template<typename FEmpty, typename FNonEmpty>
    void Match(FEmpty empty_func, FNonEmpty non_empty_func) const {
        if (ptr_) {
            non_empty_func(*ptr_);
        } else {
            empty_func();
        }
    }

    template<typename FNonEmpty>
    void Require(FNonEmpty non_empty_func) const {
        assert(ptr_ && "OptionalPtr is null. require() assertion failed.");
        non_empty_func(*ptr_);
    } 

private:
    std::unique_lock<std::shared_mutex> lock_;
    T* ptr_;
};

}

#endif
