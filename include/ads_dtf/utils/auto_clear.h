/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef AUTO_CLEAR_H
#define AUTO_CLEAR_H

namespace ads_dtf {

#include <type_traits>
#include <utility>

// Trait to detect member function void clear()
template <typename T>
class has_member_clear {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().clear(), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Trait to detect free function void clear(T&)
template <typename T>
class has_free_clear_ref {
private:
    template <typename U>
    static auto test(int) -> decltype(clear(std::declval<U&>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Trait to detect free function void clear(T*)
template <typename T>
class has_free_clear_ptr {
private:
    template <typename U>
    static auto test(int) -> decltype(clear(std::declval<U*>()), std::true_type());

    template <typename U>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Trait to check if any clear method is available
template <typename T>
struct has_any_clear {
    static constexpr bool value = has_member_clear<T>::value ||
                                  has_free_clear_ref<T>::value ||
                                  has_free_clear_ptr<T>::value;
};

// Overload for member clear()
template <typename T>
typename std::enable_if<has_member_clear<T>::value, void>::type
auto_clear_impl(T* ptr) {
    ptr->clear();
}

// Overload for free function clear(T&)
template <typename T>
typename std::enable_if<!has_member_clear<T>::value && has_free_clear_ref<T>::value, void>::type
auto_clear_impl(T* ptr) {
    clear(*ptr);
}

// Overload for free function clear(T*)
template <typename T>
typename std::enable_if<!has_member_clear<T>::value && 
                        !has_free_clear_ref<T>::value && 
                        has_free_clear_ptr<T>::value, void>::type
auto_clear_impl(T* ptr) {
    clear(ptr); 
}

// Overload to trigger static_assert failure if no clear method is available
template <typename T>
typename std::enable_if<!has_any_clear<T>::value, void>::type
auto_clear_impl(T* ptr) {
    // cannot find clear method for type T, do nothing
    // static_assert(has_any_clear<T>::value, "No clear method available for type T");
}

template <typename T>
void auto_clear(T* ptr) {
    auto_clear_impl(ptr);
}

}

#endif
