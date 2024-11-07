/**
* Copyright (c) wangbo@joycode.art 2024
*/

#ifndef DEFAULT_CONSTRUCTOR_H
#define DEFAULT_CONSTRUCTOR_H

#include <type_traits>
#include <cstring>
#include <new>
#include <iostream>

namespace ads_dtf {

// 优先级标签，优先级从高到低
struct priority_tag_trivially_constructible {};
struct priority_tag_default_constructible {};
struct priority_tag_pointer {};
struct priority_tag_non_constructible {};

template <typename T>
struct DefaultConstructor {
    static bool Construct(void* memory) {
        return helper::Construct(memory, typename construct_priority::type());
    }

private:
    // 元函数用于选择优先级标签
    struct construct_priority {
        typedef typename std::conditional<
            std::is_trivially_default_constructible<T>::value,
            priority_tag_trivially_constructible,
            typename std::conditional<
                std::is_default_constructible<T>::value,
                priority_tag_default_constructible,
                typename std::conditional<
                    std::is_pointer<T>::value,
                    priority_tag_pointer,
                    priority_tag_non_constructible
                >::type
            >::type
        >::type type;
    };

    // Helper 结构体，用于根据优先级执行构造逻辑
    struct helper {
        // 对于可平凡默认构造的类型，使用 memset 清零
        static bool Construct(void* memory, priority_tag_trivially_constructible) {
            std::memset(memory, 0, sizeof(T));
            return true;
        }

        // 对于可默认构造的类型，使用默认构造函数
        static bool Construct(void* memory, priority_tag_default_constructible) {
            new (memory) T();
            return true;
        }

        // 对于指针类型，初始化为 nullptr
        static bool Construct(void* memory, priority_tag_pointer) {
            new (memory) T(nullptr);
            return true;
        }

        // 对于无法构造的类型，不进行构造
        static bool Construct(void*, priority_tag_non_constructible) {
            return false;
        }
    };
};

template <typename T>
bool auto_construct(T* ptr) {
    return DefaultConstructor<T>::Construct(ptr);
}

}

#endif
