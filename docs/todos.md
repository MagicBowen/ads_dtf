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