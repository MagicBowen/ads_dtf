- [x]: add config to data manager, could close the access controller
- [x]: refactor C++17 code to support C++14
- [ ]: add locks to data manager
- [ ]: dependency relationship management
- [ ]: auto parallelize algo processor


---

好的，我理解您的需求。您希望在编译期针对特定的 `USER` 和 `DTYPE`，检查不同 `LifeSpan` 的 `Permission` 特化版本的存在情况，并根据特化数量返回相应的 `mode` 和 `count` 值。具体规则如下：

1. **只有一个 `LifeSpan` 的 `Permission` 特化存在：**
   - 返回对应的 `mode`。
   - 返回 `count = 1`。

2. **有多个 `Permission` 特化存在：**
   - 返回 `mode = AccessMode::None`。
   - 返回真实的特化数量。

3. **没有 `Permission` 特化存在：**
   - 返回 `mode = AccessMode::None`。
   - 返回 `count = 0`。

由于您使用的是 C++14 标准，我们需要采用模板元编程的技巧来实现这一功能。以下是完整的实现代码，并附有详细的解释和测试示例。

### 完整实现代码

```cpp
#include <type_traits>
#include <iostream>

// 定义 AccessMode 枚举
enum class AccessMode {
    None,
    Read,
    Write,
    ReadWrite
};

// 定义 LifeSpan 枚举
enum class LifeSpan {
    Frame,
    Cache,
    Global,
    Max
};

// 默认的 Permission 模板
template<typename USER, typename DTYPE, LifeSpan SPAN>
struct Permission {
    constexpr static AccessMode mode = AccessMode::None; 
};

// 示例特化版本
struct UserA {};
struct DataTypeA {};

template<>
struct Permission<UserA, DataTypeA, LifeSpan::Frame> {
    constexpr static AccessMode mode = AccessMode::Read;
};

template<>
struct Permission<UserA, DataTypeA, LifeSpan::Global> {
    constexpr static AccessMode mode = AccessMode::Write;
};

// 另一个示例特化，仅一个 LifeSpan
struct UserB {};
struct DataTypeB {};

template<>
struct Permission<UserB, DataTypeB, LifeSpan::Cache> {
    constexpr static AccessMode mode = AccessMode::ReadWrite;
};

// 定义 Info 结构，用于存储当前的 mode 和 count
template<AccessMode ModeVal, int CountVal>
struct Info {
    static constexpr AccessMode mode = ModeVal;
    static constexpr int count = CountVal;
};

// 在 C++14 中定义 void_t
template<typename...>
using void_t = void;

// SFINAE 检测特定 Permission 特化是否存在且 mode 不为 None
template<typename USER, typename DTYPE, LifeSpan SPAN, typename = void>
struct has_permission_specialization : std::false_type {};

template<typename USER, typename DTYPE, LifeSpan SPAN>
struct has_permission_specialization<USER, DTYPE, SPAN, void_t<decltype(Permission<USER, DTYPE, SPAN>::mode)>> {
    static constexpr bool value = (Permission<USER, DTYPE, SPAN>::mode != AccessMode::None);
};

// UpdateInfo 用于在递归过程中更新 Info
template<typename USER, typename DTYPE, typename CurrentInfo, LifeSpan SPAN, bool Has = has_permission_specialization<USER, DTYPE, SPAN>::value>
struct UpdateInfo;

// 当存在特化时更新 Info
template<typename USER, typename DTYPE, typename CurrentInfo, LifeSpan SPAN>
struct UpdateInfo<USER, DTYPE, CurrentInfo, SPAN, true> {
    using type = typename std::conditional<
        CurrentInfo::count == 0,
        Info<Permission<USER, DTYPE, SPAN>::mode, 1>,
        typename std::conditional<
            CurrentInfo::count == 1,
            Info<AccessMode::None, 2>,
            Info<AccessMode::None, CurrentInfo::count + 1>
        >::type
    >::type;
};

// 当不存在特化时保持 Info 不变
template<typename USER, typename DTYPE, typename CurrentInfo, LifeSpan SPAN>
struct UpdateInfo<USER, DTYPE, CurrentInfo, SPAN, false> {
    using type = CurrentInfo;
};

// 递归处理所有 LifeSpan 并更新 Info
template<typename USER, typename DTYPE, typename CurrentInfo, LifeSpan... SPANs>
struct ProcessLifespans;

// 基础情况：没有更多的 LifeSpan 需要处理
template<typename USER, typename DTYPE, typename CurrentInfo>
struct ProcessLifespans<USER, DTYPE, CurrentInfo> {
    using type = CurrentInfo;
};

// 递归情况：处理第一个 LifeSpan 并继续处理剩余的
template<typename USER, typename DTYPE, typename CurrentInfo, LifeSpan First, LifeSpan... Rest>
struct ProcessLifespans<USER, DTYPE, CurrentInfo, First, Rest...> {
    using updated_info = typename UpdateInfo<USER, DTYPE, CurrentInfo, First>::type;
    using type = typename ProcessLifespans<USER, DTYPE, updated_info, Rest...>::type;
};

// 定义要检查的 LifeSpan 列表，排除 LifeSpan::Max
#define ENUM_LIFESPAN_VALUES LifeSpan::Frame, LifeSpan::Cache, LifeSpan::Global

// 定义 PermissionResult，用于存储最终的 mode 和 count
template<typename USER, typename DTYPE>
struct PermissionResult {
    using InitialInfo = Info<AccessMode::None, 0>;
    using FinalInfo = typename ProcessLifespans<USER, DTYPE, InitialInfo, ENUM_LIFESPAN_VALUES>::type;

    static constexpr AccessMode mode = FinalInfo::mode;
    static constexpr int count = FinalInfo::count;
};

// 辅助函数，用于将 AccessMode 转换为字符串（仅用于测试输出）
const char* AccessModeToString(AccessMode mode) {
    switch(mode) {
        case AccessMode::None: return "None";
        case AccessMode::Read: return "Read";
        case AccessMode::Write: return "Write";
        case AccessMode::ReadWrite: return "ReadWrite";
        default: return "Unknown";
    }
}

// 测试示例
int main() {
    // 测试 UserA 和 DataTypeA，应有两个特化，mode = None, count = 2
    constexpr int countA = PermissionResult<UserA, DataTypeA>::count;
    constexpr AccessMode modeA = PermissionResult<UserA, DataTypeA>::mode;
    std::cout << "UserA & DataTypeA:\n";
    std::cout << "Mode: " << AccessModeToString(modeA) << "\n";
    std::cout << "Count: " << countA << "\n\n";

    // 测试 UserB 和 DataTypeB，只有一个特化，mode = ReadWrite, count =1
    constexpr int countB = PermissionResult<UserB, DataTypeB>::count;
    constexpr AccessMode modeB = PermissionResult<UserB, DataTypeB>::mode;
    std::cout << "UserB & DataTypeB:\n";
    std::cout << "Mode: " << AccessModeToString(modeB) << "\n";
    std::cout << "Count: " << countB << "\n\n";

    // 测试 UserC 和 DataTypeC，没有特化，mode = None, count =0
    struct UserC {};
    struct DataTypeC {};
    constexpr int countC = PermissionResult<UserC, DataTypeC>::count;
    constexpr AccessMode modeC = PermissionResult<UserC, DataTypeC>::mode;
    std::cout << "UserC & DataTypeC:\n";
    std::cout << "Mode: " << AccessModeToString(modeC) << "\n";
    std::cout << "Count: " << countC << "\n\n";

    return 0;
}
```

### 代码详解

1. **基础定义：**
   - **`AccessMode` 和 `LifeSpan` 枚举：** 定义了可能的访问模式和生命周期。
   - **`Permission` 模板及其特化：** 默认情况下，`Permission` 的 `mode` 为 `AccessMode::None`。您可以为特定的 `USER`, `DTYPE` 和 `LifeSpan` 提供特化版本。

2. **检测特化存在性：**
   - **`has_permission_specialization` 结构模板：** 使用 SFINAE 技术检测特定的 `Permission` 特化是否存在且其 `mode` 不为 `AccessMode::None`。

3. **信息累积结构体：**
   - **`Info` 结构模板：** 用于存储当前的 `mode` 和 `count`。在递归过程中，这个结构会不断更新以反映当前已检测到的特化数量和对应的 `mode`。

4. **更新信息的机制：**
   - **`UpdateInfo` 结构模板：** 根据当前的 `Info` 和新的 `LifeSpan`，更新 `mode` 和 `count`。具体规则如下：
     - 如果当前 `count` 为 0 且存在特化，则设置 `mode` 为该特化的 `mode`，`count` 为 1。
     - 如果当前 `count` 为 1 且存在另一个特化，则将 `mode` 设置为 `AccessMode::None`，`count` 增加到 2。
     - 如果已有多个特化存在，则继续保持 `mode` 为 `AccessMode::None`，并递增 `count`。

5. **递归处理所有 `LifeSpan`：**
   - **`ProcessLifespans` 结构模板：** 递归地处理所有指定的 `LifeSpan`，并通过 `UpdateInfo` 更新 `Info` 结构。

6. **最终结果：**
   - **`PermissionResult` 结构模板：** 根据递归后的 `Info` 结构，提供最终的 `mode` 和 `count`。

7. **辅助函数：**
   - **`AccessModeToString` 函数：** 辅助将 `AccessMode` 枚举值转换为字符串，用于测试输出。

8. **测试示例：**
   - **`UserA & DataTypeA`：** 有两个 `Permission` 特化，期望 `mode = None`，`count = 2`。
   - **`UserB & DataTypeB`：** 只有一个 `Permission` 特化，期望 `mode = ReadWrite`，`count = 1`。
   - **`UserC & DataTypeC`：** 没有 `Permission` 特化，期望 `mode = None`，`count = 0`。

### 编译和运行结果

编译并运行上述代码，您将得到以下输出：

```
UserA & DataTypeA:
Mode: None
Count: 2

UserB & DataTypeB:
Mode: ReadWrite
Count: 1

UserC & DataTypeC:
Mode: None
Count: 0
```

这符合您的需求：

1. **UserA & DataTypeA：**
   - 有两个不同 `LifeSpan` 的 `Permission` 特化 (`Frame` 和 `Global`)。
   - 因此，`mode` 被设置为 `AccessMode::None`，`count` 为 2。

2. **UserB & DataTypeB：**
   - 只有一个 `LifeSpan` (`Cache`) 的 `Permission` 特化。
   - 因此，`mode` 被设置为 `AccessMode::ReadWrite`，`count` 为 1。

3. **UserC & DataTypeC：**
   - 没有任何 `Permission` 特化。
   - 因此，`mode` 被设置为 `AccessMode::None`，`count` 为 0。

### 关键点总结

- **模板元编程的递归应用：** 通过递归模板处理所有枚举的 `LifeSpan`，并逐步累积特化的信息。
- **SFINAE 技术的应用：** 使用 SFINAE 检测特定的 `Permission` 特化是否存在且有效。
- **C++14 兼容性：** 整个实现遵循 C++14 标准，不使用 `if constexpr` 等 C++17 以上的特性。
- **灵活性与可扩展性：** 您可以轻松扩展 `LifeSpan` 枚举和相应的 `Permission` 特化，而无需更改核心逻辑。

通过上述实现，您可以在编译期高效地检查并统计特定 `USER` 和 `DTYPE` 的 `Permission` 特化版本数量，并根据需求返回对应的 `mode` 和 `count` 值。