- [x]: add config to data manager, could close the access controller
- [x]: refactor C++17 code to support C++14
- [ ]: add locks to data manager
- [ ]: dependency relationship management
- [ ]: auto parallelize algo processor


---

我要用 C++ 实现一个数据管理框架，用户类可以向框架 Apply 一个数据类型的权限，然后后续过程中可以获得这个数据的指针进行使用；

```cpp

enum class AccessMode {
    Read,
    Write,
    Create,
};

struct DataManager {
    template<typename USER, typename DTYPE, AccessMode mode>
    bool Apply() {
        // 按照用户的 USER 类型， 数据类型 DTYPE， 记录权限 mode
        // 内部管理 DTYPE 的内存；
    }

    template<typename USER, typename DTYPE>
    auto Fetch() {
        // 根据用户的 USER 类型, 和之前注册的权限 mode， 返回 DTYPE 的指针
        // 如果用户没有权限，如果没有权限，则应该 static_assert 失败
        // 如果用户有 Read 权限， 则返回 DTYPE 的 const 指针
        // 如果用户有 Write 权限， 则返回 DTYPE 的指针
    }

    template<typename USER, typename DTYPE, typename ...Args>
    auto Create(Args&& ...args) {
        // 根据用户的 USER 类型, 判断用户是否有 Create 权限，如果没有则 static_assert 失败
        // 如果用户有 Create 权限， 则使用 Args... 构造一个 DTYPE 对象， 并返回 DTYPE 的指针
    }
};
```

数据框架`DataManager`的接口如上所示，我需要你按照要求完成对应的代码实现；注意，你需要使用 C++14 的特性来实现这个框架；
Fetch 的返回值类型需要静态推导是否需要 const 修饰；Fetch 和 Create 接口中判断如果用户没有权限，需要 static_assert 失败；

---

我建议你如下修改代码：

1. DataManager 不要是模板类， DataManager 可以是一个单例类，对外提供一个 Apply 接口，用于用户注册需要 DataManager 托管的数据类型预分配的内存；
```cpp

enum class AccessMode {
    Read,
    Write,
    Create,
    CreateSync,
    None,
};

enum class LifeSpan {
    Frame,
    Cache,
    Global,
};

struct DataManager {
    template<typename USER, typename DTYPE, LifeSpan span, AccessMode mode>
    bool Apply();

    template<typename DTYPE, typename USER, LifeSpan span>
    auto Get();
};
```

2，每个User类，可以都定义如下的permission的特化类，来静态注册权限；具体可以是现在每个 User 类型的文件组后，采用类似下面宏来声明权限；

```cpp
template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    using utype = USER;
    using dtype = DTYPE;
    using span = SPAN;
    constexpr static AccessMode mode = AccessMode::None;
};

#define DECLARE_CREATE_PERMISSION(USER, SPAN, DTYPE)  \
    template<>                                                  \
    struct Permission<USER, DTYPE, SPAN> {                      \
        using utype = USER;                                     \
        using dtype = DTYPE;                                    \
        using span = SPAN;                                      \
        constexpr static AccessMode mode = AccessMode::Create; 
    };

#define DECLARE_CREATE_SYNC_PERMISSION(USER, SPAN, DTYPE) \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        using utype = USER;                                         \
        using dtype = DTYPE;                                        \
        using span = SPAN;                                          \
        constexpr static AccessMode mode = AccessMode::CreateSync;
    };

#define DECLARE_READ_PERMISSION(USER, SPAN, DTYPE) \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        using utype = USER;                                         \
        using dtype = DTYPE;                                        \
        using span = SPAN;                                          \
        constexpr static AccessMode mode = AccessMode::Read;
    };

#define DECLARE_WRITE_PERMISSION(USER, SPAN, DTYPE) \
    template<>                                                      \
    struct Permission<USER, DTYPE, SPAN> {                          \
        using utype = USER;                                         \
        using dtype = DTYPE;                                        \
        using span = SPAN;                                          \
        constexpr static AccessMode mode = AccessMode::Write;
    };
```

```cpp
struct Data {
    int value;
};

struct UserA {

};

DECLARE_CREATE_PERMISSION(UserA, LifeSpan::Frame, Data);

struct UserB {

};

DECLARE_READ_PERMISSION(UserB, LifeSpan::Frame, Data);
```

3，实现一个静态对象自动构造的机制，默认在 DECLARE_CREATE_PERMISSION 和 DECLARE_CREATE_SYNC_PERMISSION 宏的后面，使用静态对象构造机制，自动向 DataManager Apply，触发内存的预先分配；

4，DataManager 的 Get 方法，使用 Permission 判断是否有权限，根据权限返回不同的类型（OptPtr 或者 SyncReadPtr 或者 SyncWritePtr）；另外如果数据没有人注册 Create，要返回 nullptr 的 OptPtr 或者 SyncReadPtr 或者 SyncWritePtr 的封装；