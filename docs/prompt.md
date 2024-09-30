我在 C++中有很多算法类（下面 AlgoProcessor 的子类），这些算法类之间的关系类似“管道过滤器模式”，每次进程从上游收到定频的单帧数据后触发算法的依次调用。
这些算法类会交叉共享使用很多数据对象，这些数据包含三类：

- 进程从上游统一收到的单帧消息中的数据对象结构中的数据；
- 某些算法会把自己的计算结果往下游传递的数据对象；
- 某些需要缓存在本地内存跨帧使用的数据对象；

我希望能够把这些数据统一交给一个数据对象框架管理，这样规范算法对数据的使用，以及可以精细管理算法对数据的依赖关系。

我希望算法类可以在统一初始化的时候，向传入的它们的某个数据框架接口注册自己可以使用的数据类型以及权限（读/写、挂载），（算法类都有Init 接口，可以将数据框架的数据声明接口类作为参数传给算法进行注册）；然后算法在执行的时候可以通过传入的上下文对象从数据框架获取自己申请的可访问数据（必须是自己在初始化的时候申请过的），算法有 Exec 接口，可以将数据框架的上下文对象接口类作为参数传给算法，供算法进行数据获取和读写；数据管理框架需要能够管理上面提到的三种数据，支持对不同生命周期的用法。

并且数据的生命周期由数据框架统一管理，数据框架统一管理数据的内存。

最后，我的程序希望能够极致性能，因此不要使用 rtti；算法声明自己的数据依赖的时候，可以使用一些编译期算好的静态数据作为信息表达。

目前我已经有如下的定义：

```cpp
// type_id.h

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
```

```cpp
template <typename T>
struct Placement
{
    Placement& operator=(const T& rhs)
    {
        assignBy(rhs);
        return *this;
    }

    void* alloc()
    {
        return (void*)&mem;
    }

    void free()
    {
       getObject()->~T();
    }

    T* operator->() const
    {
        return getObject();
    }

    T& operator*() const
    {
        return getRef();
    }

    T* getObject() const
    {
        return (T*)&mem;
    }

    T& getRef() const
    {
        return (T&)mem;
    }

    void destroy()
    {
        getObject()->~T();
    }

private:
    void assignBy(const T& rhs)
    {
        T* p = (T*)alloc();
        *p = rhs;
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> mem;
};
```

```cpp
struct AlgoProcessor {
    virtual bool Init(DataManager& data_manager) = 0;
    virtual bool Exec(DataContext& data_context) = 0;
    virtual ~AlgoProcessor() = default;
};
```

```cpp
enum class LifeSpan {
    Frame,
    Cache,
    Global,
    Max
};

enum class AccessMode {
    // mark the AlgoProcessor who has only the const read right
    Read,
    // mark the AlgoProcessor who has only the read & write right
    Write,
    // mark the AlgoProcessor who can call the Mount interface to construct the data object
    Mount,
    // mark the AlgoProcessor who can call the Unmount interface to destruct the data object
    Unmount,
    // mark the AlgoProcessor who has no right to access the data
    None,
};

using UserId = TypeId;
using DataType = TypeId;
using DataPtr  = void*;
```

```cpp
// used by DataManager to store the access right of user to data
struct AccessController {
    // register the user access right of data, if conflict or duplicate return false
    bool Register(UserId user, DataType data, LifeSpan span, AccessMode mode);

    // get the access mode of user to data, if not found return None
    AccessMode GetAccessMode(UserId user, DataType data, LifeSpan span);
```

```cpp
    struct DataManager {
        // AlgoProcessor uses DataManager::Apply when Init, and pass this as the first parameter to declare the TYPE of AlogProcesser
        // DTYPE is the type of data, USER is the type of AlgoProcessor
        // Apply should cast USER and DTYPE to TypeId and register;
        // the DataManager should preoccupy the memory(use above Placement class) for DTYPE and save the ptr;
        // if the DTYPE support default constructor, then the DataManager should call the default constructor by placement new, and mark the data has been constructed;
        template<typename DTYPE, typename USER>
        bool Apply(USER*, LifeSpan span, AccessMode mode);

        // AlgoProcessor used DataContext when Exec, and get data by context
        DataContext& GetContext();

    private:
        friend class DataContext;

    private:
        // other methods and some used by DataContext    
    };
```

```cpp
    struct DataContext {
        // AlgoProcessor uses DataContext::Get when Exec, and pass this as the first parameter to declare the TYPE of AlogProcesser
        // DTYPE is the type of data, USER is the type of AlgoProcessor
        // Get should cast USER and DTYPE to TypeId and get the data;
        // the DataContext should check the access right of USER to DTYPE, if not found return nullptr;

        template<typename DTYPE, typename USER>
        DTYPE* Get(USER*, LifeSpan span);

        // When AlgoProcessor applied AccessMode::Read to the data, it should use GetConst to get the const data
        template<typename DTYPE, typename USER>
        const DTYPE* GetConst(USER*, LifeSpan span);

        // When DTYPE do not support default constructor, the DataManager only occupied the memory, and the AlgoProcessor who has Mount right should call the DataContext::New in the preoccupied memory of DTYPE in DataManager
        template<typename DTYPE, typename ...ARGs, typename USER>
        DTYPE* Mount(USER*, LifeSpan span, ARGs&&... args);

        // AlgoProcessor who has Unmount right could use this interface to destroy the data
        template<typename DTYPE, typename ...ARGs, typename USER>
        void Unmount(DTYPE ptr, USER*);
    };
```

请根据上面的代码示例，接口和注释提示，帮我写出完整可靠的代码实现。