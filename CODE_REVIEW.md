# BIPTRU/BIPTRG 模块 Code Review (v3)

**日期**: 2026-08-01
**审查范围**: 数组处理 (BIPTRG)、格式化打印 (BIPRTU)、数据管理 (GWDATA)、初始化 (GWINIT)
**基于提交**: `24c27a6` — fix: resolve CRITICAL memory issues and test crashes
**上一轮 Review**: v2 (基于 `ebc500e`)
**测试状态**: 92/92 全部通过
**编译警告**: 1 个 (`biprtu.c:21` — `has_sign` 未使用)

---

## 变更摘要

`24c27a6` 修复了 v2 review 中的以下问题:

| 问题 | 状态 | 说明 |
|------|------|------|
| CRITICAL #1: `dim_sizes` 永不释放 | ✅ 已修复 | 添加 `biptrg_free_array()` 函数，含 magic 校验 |
| CRITICAL #2: 覆盖数组变量时泄漏 | ✅ 已修复 | `gwdata_alloc_variable()` 调用 `biptrg_free_array()` |

新增内容:
- `array_desc_t` 添加 `magic` 字段 (`ARRAY_MAGIC = 0x41525241`) 用于安全识别
- `biptrg_free_array()` 函数，含 magic 校验防止误释放非数组数据
- `gwdata_init()` 改为重置状态而非 no-op，实现测试间隔离
- `gwdata_alloc_variable()` 在复用变量前调用 `biptrg_free_array()`
- `tests/test_isolate.c` 调试用隔离测试文件

---

## 当前问题

### CRITICAL

#### 1. Union 别名 bug：`gwdata_alloc_variable` 误判浮点变量为数组

**位置**: `src/gwdata.c:61-66`

```c
if (strcmp(existing->name, name) == 0) {
    if (existing->value.s != NULL) {       // ❌ union 别名问题
        biptrg_free_array(existing);       //    浮点值被误读为指针
    }
    existing->type = type;
    existing->value.s = NULL;
    return existing;
}
```

**根因**: `struct variable` 的 `value` 是 union，`value.s`（char*）和 `value.f`（float）共享内存。当变量被赋值为浮点数后（如 `value.f = 42.0`），union 的字节被重新解释为 `value.s`，结果非 NULL。

**触发场景**: REPL 中对同一变量二次赋值:
```
A = 42    → value.f = 42.0, value.s = 0x0000000042280000 (非 NULL!)
A = 99    → gwdata_alloc_variable("A", VT_SINGLE) 找到已有 "A"
            → value.s 非 NULL → biptrg_free_array() 被调用
            → desc = (array_desc_t*)0x0000000042280000 (无效指针!)
            → desc->magic 读取无效地址 → ACCESS VIOLATION (段错误)
```

**为什么测试通过**: `gwdata_init()` 现在重置 `var_table` 为 NULL，每个测试从干净状态开始。测试中没有对同一变量名重复调用 `gwdata_alloc_variable`，所以 `existing` 分支不会被触发。但生产 REPL 中会触发。

**影响**: 用户在 REPL 中对同一变量二次赋值时崩溃

**修复建议**: 不依赖 `value.s` 判断是否为数组。使用独立标记或检查 type:
```c
/* 方案 1: 检查变量类型是否为数组类型 */
/* 需要 array_desc_t 存储标记，或单独的 is_array 标志 */

/* 方案 2: 在 struct variable 中添加 is_array 字段 */
struct variable {
    char name[MAX_VAR_NAME + 1];
    var_type_t type;
    bool is_array;          /* 新增: 标记是否为数组 */
    union { ... } value;
    struct variable *next;
};

/* 在 gwdata_alloc_variable 中: */
if (existing->is_array) {
    biptrg_free_array(existing);
    existing->is_array = false;
}
existing->type = type;
existing->value.s = NULL;
return existing;

/* 在 biptrg_dim_array 中: */
v->is_array = true;
```

---

#### 2. 生产代码中的 debug printf

**位置**: `src/gwinit.c:10-18`

```c
printf("DEBUG: gwinit_init start\n");
fflush(stdout);
// ...
printf("DEBUG: after gwdata_alloc_variable\n");
fflush(stdout);
// ...
printf("DEBUG: gwinit_init end\n");
fflush(stdout);
```

**影响**: `gwbasic.exe` 运行时每次调用 `gwinit_init` 都会输出 DEBUG 信息，污染用户界面。每个测试也输出这些信息（测试输出中可见）。

**修复**: 删除所有 `printf("DEBUG: ...")` 语句

---

### HIGH

#### 3. DIM 语句不支持字符串数组声明 (`$` 后缀)

**位置**: `src/gwsts.c:77`

```c
biptrg_dim_array(var_name, size, VT_SINGLE);  // 总是 VT_SINGLE
```

**说明**: `biptrg_dim_array` 支持 `VT_STRING`，但 DIM 语句硬编码 `VT_SINGLE`。用户无法通过 `DIM S$(10)` 声明字符串数组。

**修复建议**: 检测变量名 `$` 后缀:
```c
var_type_t type = VT_SINGLE;
if (name_len > 0 && var_name[name_len - 1] == '$') {
    type = VT_STRING;
}
```

---

#### 4. `biptrg_get_array_element` 缺少 VT_STRING 类型检查

**位置**: `src/biptrg.c:93-110`

```c
array_desc_t *desc = (array_desc_t *)v->value.s;
if (!desc) return 0.0f;
if (!desc->data) return 0.0f;

/* 没有检查 desc->magic 是否有效 */
/* 没有检查 desc->type 是否为 VT_STRING */
float *data = (float *)desc->data;
return data[index];
```

**影响**: 对字符串数组调用 `biptrg_get_array_element` 会将 `char**` 数据当作 `float*` 读取，导致未定义行为。

**修复建议**:
```c
if (desc->magic != ARRAY_MAGIC) return 0.0f;
if (desc->type == VT_STRING) return 0.0f;  /* 字符串数组不能作为数值读取 */
```

---

### MEDIUM

#### 5. DIM 语句不支持表达式计算大小

**位置**: `src/gwsts.c:70`

```c
size = atoi(size_str);  /* atoi("5+3") = 5, 不支持表达式 */
```

**说明**: GW-BASIC 支持 `DIM A(5+3)`。当前 `atoi` 在遇到非数字字符时截断。

**修复建议**: 使用 `gweval_eval_numeric(size_str)` 替代 `atoi`

---

#### 6. `atoi` 无输入验证

**位置**: `src/gwsts.c:70`, `src/gwsts.c:247`

```c
size = atoi(size_str);      /* atoi("abc") = 0 */
*index = atoi(index_buf);   /* atoi("abc") = 0, 无法区分合法的 0 */
```

**修复建议**: 使用 `strtol` 并检查 `endptr`

---

#### 7. `gwdata_init()` 重置变量表时泄漏变量内存

**位置**: `src/gwdata.c:22-25`

```c
for (int i = 0; i < MAX_VARIABLES; i++) {
    g_state->var_table[i] = NULL;  /* 指针直接置 NULL，不释放 */
}
```

**说明**: 注释说 "let OS reclaim at exit"。对测试可接受，但生产 REPL 中每次 `gwdata_init` 调用都会泄漏所有变量和数组内存。

**修复建议**: 添加 `gwdata_cleanup()` 函数，遍历变量表释放所有变量和数组

---

#### 8. DIM 变量名前有空格时解析失败

**位置**: `src/gwsts.c:55-56`

`DIM  A (10)` 会解析出变量名 `" A "`（含空格），导致后续查找失败。

**修复建议**: 截取变量名前去除首尾空格

---

### LOW

#### 9. 调试产物未清理

| 文件 | 问题 |
|------|------|
| `tests/test_isolate.c` | 调试用隔离测试文件，未加入 Makefile，应删除或文档化 |
| `tests/test_gwinit.c` | 大量 `printf("DEBUG: ...")` 语句 |
| `src/gwinit.c` | debug printf 已标记为 CRITICAL #2 |

---

#### 10. 测试覆盖不足

**当前**: 92 个断言全部通过，但缺少:

| 缺失场景 | 优先级 |
|----------|--------|
| 变量二次赋值（触发 CRITICAL #1） | CRITICAL |
| 数组边界 (index=0, size-1, size, -1) | HIGH |
| DIM 多数组声明 | HIGH |
| 数组表达式索引 `A(2+3)` | MEDIUM |
| 错误处理 (NULL、无效类型) | MEDIUM |
| 内存泄漏检测 (valgrind/ASAN) | LOW |

---

#### 11. PRINT USING 实现不完整

**当前支持**: `##.##` 数值格式、`\\` 字符串格式

**缺失**: 千位分隔符、前导星号、浮动美元符、科学计数法、`&` 可变宽度字符串、`!` 单字符

---

#### 12. 编译警告未处理

**位置**: `src/biprtu.c:21`

```c
bool has_sign = false;  // 赋值但从未使用
```

**修复**: 删除 `has_sign` 变量，或实际使用它

---

#### 13. `biptrg.c` 文件末尾无换行符

文件以 `}` 结束，没有换行符（`git diff` 显示 `\ No newline at end of file`）。

---

## 代码质量评价

### 优点

- ✅ **Magic number 设计**: `biptrg_free_array` 使用 magic 校验防止误释放，是良好的防御性编程
- ✅ **测试隔离**: `gwdata_init()` 重置状态实现测试间隔离，解决了测试崩溃问题
- ✅ **内存清理**: `biptrg_free_array` 正确释放 `data`、`dim_sizes`、`desc` 三层
- ✅ **GW-BASIC 兼容**: 数组索引 clamp 行为
- ✅ **公共函数**: `assign_value()` 消除代码重复

### 需要改进

- ❌ **Union 别名 bug** (CRITICAL #1): `value.s != NULL` 检查无法区分数组和浮点变量
- ❌ **Debug printf 残留** (CRITICAL #2): 生产代码不应包含调试输出
- ❌ **缺少 `is_array` 标记**: `struct variable` 没有独立标记区分数组和普通变量
- ❌ **DIM 类型支持** (HIGH #3): 无法声明字符串数组
- ❌ **类型检查缺失** (HIGH #4): 数组读写不验证类型
- ❌ **测试覆盖** (LOW #10): 缺少变量二次赋值和边界测试

---

## 优先级修复顺序

1. **立即修复**: Union 别名 bug — 添加 `is_array` 字段或改用 type 判断 (CRITICAL #1)
2. **立即修复**: 删除 `gwinit.c` 中的 debug printf (CRITICAL #2)
3. **高优先级**: DIM 字符串数组 `$` 后缀支持 (HIGH #3)
4. **高优先级**: 数组读写 magic + 类型检查 (HIGH #4)
5. **中优先级**: 添加 `gwdata_cleanup()` 函数 (MEDIUM #7)
6. **中优先级**: DIM 表达式 + `strtol` (MEDIUM #5, #6)
7. **低优先级**: 清理调试产物 (LOW #9)
8. **低优先级**: 测试覆盖增强 (LOW #10)
9. **低优先级**: PRINT USING 完善 (LOW #11)

---

## 总结

`24c27a6` 成功修复了 v2 review 中的两个 CRITICAL 内存泄漏问题，测试崩溃问题也已解决（`gwdata_init` 重置状态）。`biptrg_free_array` 的 magic 校验是良好的设计。

但新引入了一个严重的 union 别名 bug（CRITICAL #1）：`gwdata_alloc_variable` 用 `value.s != NULL` 判断是否为数组，但 union 的浮点值会被误读为非 NULL 指针。这在测试中不会触发（变量表被重置），但在生产 REPL 中对同一变量二次赋值时会段错误。建议添加 `is_array` 字段从根本解决类型区分问题。

同时 `gwinit.c` 中的 debug printf 不应保留在生产代码中，应立即删除。

**测试状态**: 92/92 通过（但缺少变量二次赋值的测试场景，CRITICAL #1 未被测试覆盖）。
