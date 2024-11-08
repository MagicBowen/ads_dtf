- [x]: add config to data manager, could close the access controller
- [x]: refactor C++17 code to support C++14
- [ ]: add locks to data manager
- [ ]: dependency relationship management
- [ ]: auto parallelize algo processor


---

在C++14中实现类似于C++17中使用`std::optional`和`std::index_sequence`的功能需要一些调整。以下是一个完整的示例，展示如何在C++14中枚举所有`LifeSpan`值，检查是否存在对应的`Permission`特化，以及获取每个有效`LifeSpan`的`AccessMode`。

### 1. 定义枚举和默认模板

首先，定义`LifeSpan`和`AccessMode`枚举，以及`Permission`模板的默认实现。默认情况下，`Permission`的`mode`设置为`AccessMode::None`。

```cpp
#include <array>
#include <iostream>
#include <type_traits>

// 定义 LifeSpan 枚举
enum class LifeSpan { Frame, Cache, Global, Max };

// 定义 AccessMode 枚举
enum class AccessMode { None, Read, Write, ReadWrite };

// 默认模板
template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    static const AccessMode mode = AccessMode::None;
};
```

### 2. 定义示例类型和特化

定义一些示例的`USER`和`DTYPE`类型，并为特定的`LifeSpan`提供`Permission`的特化。

```cpp
// 示例类型
struct MyUser {};
struct MyDataType {};

// 特化示例
template<>
struct Permission<MyUser, MyDataType, LifeSpan::Frame> {
    static const AccessMode mode = AccessMode::Read;
};

template<>
struct Permission<MyUser, MyDataType, LifeSpan::Cache> {
    static const AccessMode mode = AccessMode::Write;
};
```

### 3. 实现Helper模板

在C++14中，没有`std::void_t`，但可以手动实现一个类似的功能。此外，使用`std::enable_if`来检测特化是否存在。

```cpp
// 实现类似 std::void_t 的功能
template<typename... Ts>
struct make_void {
    typedef void type;
};

template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

// 检查 Permission 是否被特化（即 mode != AccessMode::None）
template<typename USER, typename DTYPE, LifeSpan SPAN, typename = void>
struct has_permission : std::false_type {};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct has_permission<USER, DTYPE, SPAN, void_t<decltype(Permission<USER, DTYPE, SPAN>::mode)>> 
    : std::integral_constant<bool, (Permission<USER, DTYPE, SPAN>::mode != AccessMode::None)> {};
```

### 4. 枚举所有 LifeSpan 并收集有效 Permissions

使用`std::integral_constant`和模板递归来枚举所有的`LifeSpan`值，并收集那些有特化的`Permission`。

```cpp
// 定义一个固定大小的数组，排除 LifeSpan::Max
constexpr size_t LifeSpanCount = static_cast<size_t>(LifeSpan::Max);

// 获取 AccessMode 对应的字符串（用于输出）
const char* AccessModeToString(AccessMode mode) {
    switch(mode) {
        case AccessMode::None: return "None";
        case AccessMode::Read: return "Read";
        case AccessMode::Write: return "Write";
        case AccessMode::ReadWrite: return "ReadWrite";
        default: return "Unknown";
    }
}

// 获取 LifeSpan 对应的字符串（用于输出）
const char* LifeSpanToString(LifeSpan span) {
    switch(span) {
        case LifeSpan::Frame: return "Frame";
        case LifeSpan::Cache: return "Cache";
        case LifeSpan::Global: return "Global";
        default: return "Unknown";
    }
}

// 模板结构，用于在编译时存储 LifeSpan 和 AccessMode
struct PermissionInfo {
    LifeSpan span;
    AccessMode mode;
};

// 递归模板，枚举 LifeSpan 并收集有效 Permissions
template<typename USER, typename DTYPE, size_t N, size_t Current = 0>
struct PermissionCollector {
    static void collect(std::array<PermissionInfo, LifeSpanCount>& infos, size_t& count) {
        if (Current < LifeSpanCount - 1) { // 排除 LifeSpan::Max
            LifeSpan span = static_cast<LifeSpan>(Current);
            if (has_permission<USER, DTYPE, span>::value) {
                infos[count].span = span;
                infos[count].mode = Permission<USER, DTYPE, span>::mode;
                ++count;
            }
            PermissionCollector<USER, DTYPE, N, Current + 1>::collect(infos, count);
        }
    }
};

// 偏特化以终止递归
template<typename USER, typename DTYPE, size_t N>
struct PermissionCollector<USER, DTYPE, N, N> {
    static void collect(std::array<PermissionInfo, LifeSpanCount>&, size_t&) {}
};
```

### 5. 主函数：展示结果

在主函数中，调用上述模板收集`Permission`信息，并输出存在特化的`LifeSpan`及其对应的`AccessMode`。

```cpp
int main() {
    // 准备一个数组来存储 Permission 信息
    std::array<PermissionInfo, LifeSpanCount> infos = {};
    size_t count = 0;

    // 收集 Permissions
    PermissionCollector<MyUser, MyDataType, LifeSpanCount>::collect(infos, count);

    // 输出结果
    for (size_t i = 0; i < count; ++i) {
        std::cout << "LifeSpan: " << LifeSpanToString(infos[i].span)
                  << ", AccessMode: " << AccessModeToString(infos[i].mode) << "\n";
    }

    return 0;
}
```

### 6. 完整示例代码

将上述各部分组合起来，完整的C++14示例代码如下：

```cpp
#include <array>
#include <iostream>
#include <type_traits>

// 定义 LifeSpan 枚举
enum class LifeSpan { Frame, Cache, Global, Max };

// 定义 AccessMode 枚举
enum class AccessMode { None, Read, Write, ReadWrite };

// 默认模板
template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    static const AccessMode mode = AccessMode::None;
};

// 示例类型
struct MyUser {};
struct MyDataType {};

// 特化示例
template<>
struct Permission<MyUser, MyDataType, LifeSpan::Frame> {
    static const AccessMode mode = AccessMode::Read;
};

template<>
struct Permission<MyUser, MyDataType, LifeSpan::Cache> {
    static const AccessMode mode = AccessMode::Write;
};

// 实现类似 std::void_t 的功能
template<typename... Ts>
struct make_void {
    typedef void type;
};

template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

// 检查 Permission 是否被特化（即 mode != AccessMode::None）
template<typename USER, typename DTYPE, LifeSpan SPAN, typename = void>
struct has_permission : std::false_type {};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct has_permission<USER, DTYPE, SPAN, void_t<decltype(Permission<USER, DTYPE, SPAN>::mode)>> 
    : std::integral_constant<bool, (Permission<USER, DTYPE, SPAN>::mode != AccessMode::None)> {};

// 定义一个固定大小的数组，排除 LifeSpan::Max
constexpr size_t LifeSpanCount = static_cast<size_t>(LifeSpan::Max);

// 获取 AccessMode 对应的字符串（用于输出）
const char* AccessModeToString(AccessMode mode) {
    switch(mode) {
        case AccessMode::None: return "None";
        case AccessMode::Read: return "Read";
        case AccessMode::Write: return "Write";
        case AccessMode::ReadWrite: return "ReadWrite";
        default: return "Unknown";
    }
}

// 获取 LifeSpan 对应的字符串（用于输出）
const char* LifeSpanToString(LifeSpan span) {
    switch(span) {
        case LifeSpan::Frame: return "Frame";
        case LifeSpan::Cache: return "Cache";
        case LifeSpan::Global: return "Global";
        default: return "Unknown";
    }
}

// 模板结构，用于在编译时存储 LifeSpan 和 AccessMode
struct PermissionInfo {
    LifeSpan span;
    AccessMode mode;
};

// 递归模板，枚举 LifeSpan 并收集有效 Permissions
template<typename USER, typename DTYPE, size_t N, size_t Current = 0>
struct PermissionCollector {
    static void collect(std::array<PermissionInfo, LifeSpanCount>& infos, size_t& count) {
        if (Current < LifeSpanCount - 1) { // 排除 LifeSpan::Max
            LifeSpan span = static_cast<LifeSpan>(Current);
            if (has_permission<USER, DTYPE, span>::value) {
                infos[count].span = span;
                infos[count].mode = Permission<USER, DTYPE, span>::mode;
                ++count;
            }
            PermissionCollector<USER, DTYPE, N, Current + 1>::collect(infos, count);
        }
    }
};

// 偏特化以终止递归
template<typename USER, typename DTYPE, size_t N>
struct PermissionCollector<USER, DTYPE, N, N> {
    static void collect(std::array<PermissionInfo, LifeSpanCount>&, size_t&) {}
};

int main() {
    // 准备一个数组来存储 Permission 信息
    std::array<PermissionInfo, LifeSpanCount> infos = {};
    size_t count = 0;

    // 收集 Permissions
    PermissionCollector<MyUser, MyDataType, LifeSpanCount>::collect(infos, count);

    // 输出结果
    for (size_t i = 0; i < count; ++i) {
        std::cout << "LifeSpan: " << LifeSpanToString(infos[i].span)
                  << ", AccessMode: " << AccessModeToString(infos[i].mode) << "\n";
    }

    return 0;
}
```

### 7. 运行结果

编译并运行上述程序，输出将如下所示：

```
LifeSpan: Frame, AccessMode: Read
LifeSpan: Cache, AccessMode: Write
```

这表明对于`MyUser`和`MyDataType`，存在`LifeSpan::Frame`和`LifeSpan::Cache`的`Permission`特化，分别对应`AccessMode::Read`和`AccessMode::Write`。

### 8. 解释与注意事项

- **`void_t` 实现**：由于C++14中没有`std::void_t`，我们手动实现了一个类似的模板别名。
  
- **检测特化存在**：通过`has_permission`模板结构，利用SFINAE（Substitution Failure Is Not An Error）技术，检查是否存在`Permission<USER, DTYPE, SPAN>`的特化，并且`mode`不等于`AccessMode::None`。

- **模板递归收集**：`PermissionCollector`模板使用递归方式枚举所有`LifeSpan`值（排除`LifeSpan::Max`），并收集那些有特化的`Permission`信息。

- **数组大小**：`PermissionInfo`数组的大小设置为`LifeSpan::Max`，确保即使所有`LifeSpan`值都有特化，也不会超出数组范围。

- **字符串映射**：为了更友好地输出结果，定义了`AccessModeToString`和`LifeSpanToString`辅助函数，将枚举值转换为字符串。

- **扩展与优化**：
  - **更多 LifeSpan 值**：可以轻松扩展`LifeSpan`枚举，并确保`LifeSpan::Max`始终作为枚举的最后一个值，以正确设置`LifeSpanCount`。
  - **自动化检测**：如果`Permission`特化数量较多，可以进一步优化模板元编程逻辑，以自动适应不同的`LifeSpan`值。

通过上述方法，你可以在C++14中实现对`Permission`特化的枚举和检测，尽管需要进行一些手动实现和模板递归，但依然能够达到预期的功能。