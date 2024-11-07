/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef OPT_PTR_H
#define OPT_PTR_H

namespace ads_dtf {

template<typename T>
class OptPtr {
public:
    OptPtr() : ptr_(nullptr) {}
    explicit OptPtr(T* ptr) : ptr_(ptr) {}

    OptPtr(const OptPtr& other) = default;
    OptPtr& operator=(const OptPtr& other) = default;
    OptPtr(OptPtr&& other) noexcept = default;
    OptPtr& operator=(OptPtr&& other) noexcept = default;

    bool HasValue() const { return ptr_ != nullptr; }
    explicit operator bool() const { return HasValue(); }
    T* Get() const { return ptr_; }


    T* operator->() const {
        assert(ptr_ && "OptionalPtr is null. assertion failed.");
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
    T* ptr_;
};

} // namespace ads_dtf

#endif
