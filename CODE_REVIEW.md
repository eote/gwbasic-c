# BIPTRU/BIPTRG 模块 Code Review (v2)

**日期**: 2026-08-01
**审查范围**: 数组处理 (BIPTRG) 和格式化打印 (BIPRTU) 模块
**基于提交**: `ebc500e` — fix: address code review issues
**上一轮 Review**: 已处理 4 个问题 (CRITICAL #2, HIGH #4, HIGH #6, CRITICAL #3)
**测试状态**: 92/92 全部通过

---

## 变更摘要

`ebc500e` 修复了上一轮 review 中的以下问题:

| 问题 | 状态 | 说明 |
|------|------|------|
| CRITICAL #2: 数组索引越界 | ✅ 已修复 | 改为 GW-BASIC 兼容的 clamp 行为 (负索引→0, 超界→size-1) |
| HIGH #4: DIM 多数组声明 | ✅ 已修复 | `continue` 替代 `break`, 支持逗号分隔的多个数组 |
| HIGH #6: 代码重复 | ✅ 已修复 | 提取 `assign_value()` 公共函数消除重复 |
| CRITICAL #3: parse_var_ref 指针泄漏 | ✅ 已修复 | `assign_value()` 统一管理 `base_name` 生命周期 |

---

## 当前问题

### CRITICAL

#### 1. 内存泄漏：`biptrg_dim_array` 中 `dim_sizes` 永不释放

**位置**: `src/biptrg.c:36-40`

```c
desc->dim_sizes = (int *)gwram_alloc(sizeof(int));
if (!desc->dim_sizes) {
    gwram_free(desc);
    return ERROR;
}
```

**说明**: `dim_sizes` 分配后，整个项目中没有代码释放它。`array_desc_t` 结构体（含 `dim_sizes`、`data`）存储在 `v->value.s` 中，但:
- 没有 `biptrg_free_array()` 函数
- 没有 `gwdata_free_variable()` 函数
- 没有 `gwdata_cleanup()` 函数

程序退出时这些内存由操作系统回收，但长期运行的 REPL 会话中会累积泄漏。

**影响**: 每个数组声明泄漏 `sizeof(array_desc_t) + sizeof(int) + size * sizeof(float)` ≈ 16 + 4 + 40 = 60 字节 (以 DIM A(10) 为例)

**修复建议**:
```c
/* 在 gwbasic.h 中添加声明 */
void biptrg_free_array(struct variable *v);

/* 在 biptrg.c 中实现 */
void biptrg_free_array(struct variable *v) {
    if (!v) return;
    array_desc_t *desc = (array_desc_t *)v->value.s;
    if (!desc) return;
    if (desc->data) gwram_free(desc->data);
    if (desc->dim_sizes) gwram_free(desc->dim_sizes);
    gwram_free(desc);
    v->value.s = NULL;
}
```

---

#### 2. `gwdata_alloc_variable` 覆盖已有数组变量时泄漏旧数组

**位置**: `src/gwdata.c:47-57`

```c
struct variable *existing = g_state->var_table[idx];
while (existing) {
    if (strcmp(existing->name, name) == 0) {
        existing->type = type;  // ❌ 直接覆盖，旧数组描述符泄漏
        return existing;
    }
    existing = existing->next;
}
```

**场景**: 用户先执行 `DIM A(10)`，再执行 `A = 5`（将 A 作为普通变量使用）。`gwdata_alloc_variable("A", VT_SINGLE)` 找到已存在的 A，直接覆盖 type，但 `v->value.s` 中存储的 `array_desc_t`（含 `dim_sizes` 和 `data`）永远不会被释放。

**影响**: 每次此操作泄漏 ~60 字节

**修复建议**: 在覆盖前检查旧值是否为数组，若是则调用 `biptrg_free_array`

---

### HIGH

#### 3. DIM 语句不支持字符串数组声明 (`$` 后缀)

**位置**: `src/gwsts.c:77`

```c
biptrg_dim_array(var_name, size, VT_SINGLE);  // ❌ 总是 VT_SINGLE
```

**说明**: `biptrg_dim_array` 的 `type` 参数被硬编码为 `VT_SINGLE`。虽然 `biptrg.c` 支持 `VT_STRING`、`VT_INTEGER`、`VT_DOUBLE`，但通过 DIM 语句无法使用这些类型。

**影响**: 用户无法通过 `DIM S$(10)` 声明字符串数组（虽然底层 API 支持）

**修复建议**:
```c
/* 检测变量名后缀 */
var_type_t type = VT_SINGLE;
size_t name_len = paren - token;
if (name_len > 1 && token[name_len - 1] == '$') {
    type = VT_STRING;
    name_len--;  /* 去掉后缀 */
}
/* ... 分配 var_name 时使用 name_len ... */
biptrg_dim_array(var_name, size, type);
```

---

#### 4. 数组读写缺少类型检查

**位置**: `src/biptrg.c:81-100` (`biptrg_get_array_element`)

```c
array_desc_t *desc = (array_desc_t *)v->value.s;
if (!desc) return 0.0f;
if (!desc->data) return 0.0f;

/* 没有检查 desc->type，直接按 float 读取 */
float *data = (float *)desc->data;
return data[index];
```

**说明**: 如果数组是 `VT_STRING` 类型，`desc->data` 存储的是 `char**`，但函数将其当作 `float*` 读取，导致未定义行为。

**影响**: 对字符串数组调用 `biptrg_get_array_element` 会读取垃圾数据

**修复建议**: 在 `VT_STRING` 分支返回 0.0f 或添加类型检查

---

### MEDIUM

#### 5. DIM 语句不支持表达式计算大小

**位置**: `src/gwsts.c:70`

```c
size = atoi(size_str);  /* ❌ 只支持字面量，不支持表达式 */
```

**说明**: GW-BASIC 支持 `DIM A(5+3)`。当前实现将 `size_str` 传给 `atoi`，`atoi("5+3")` 返回 5。

**修复建议**: 使用 `gweval_eval_numeric(size_str)` 替代 `atoi`

---

#### 6. `atoi` 无输入验证

**位置**: `src/gwsts.c:70`, `src/gwsts.c:247`

```c
size = atoi(size_str);   /* atoi("abc") = 0, 无法区分错误 */
*index = atoi(index_buf); /* atoi("abc") = 0, 无法区分错误 */
```

**说明**: `atoi` 对非法输入返回 0，无法区分合法的 "0" 和错误输入。虽然当前 `size > 0` 检查能防御部分情况，但 `index` 为 0 是合法的（`A(0)`）。

**修复建议**: 使用 `strtol` 并检查 `endptr`

---

#### 7. DIM 变量名前有空格时解析失败

**位置**: `src/gwsts.c:55-56`

```c
char *paren = strchr(token, '(');
/* 若 token = " A (10)", var_name = " A " (包含空格) */
```

**说明**: `DIM  A (10)` 会解析出变量名 `" A "`，导致后续查找失败。

**修复建议**: 在截取变量名前去除首尾空格

---

### LOW

#### 8. 测试覆盖不足

**当前测试**: 92 个断言全部通过，但缺少以下场景:

| 缺失场景 | 优先级 |
|----------|--------|
| 数组边界测试 (index=0, size-1, size, -1) | HIGH |
| DIM 多数组声明 (`DIM A(10), B(5), C(3)`) | HIGH |
| 数组表达式索引 (`A(2+3)`) | MEDIUM |
| 错误处理 (NULL 指针、无效类型) | MEDIUM |
| 字符串数组边界测试 | MEDIUM |
| 内存泄漏检测 (valgrind/ASAN) | LOW |
| 类型覆盖 (INTEGER, DOUBLE) | LOW |

**建议**: 优先添加边界测试和多数组测试

---

#### 9. PRINT USING 实现不完整

**当前支持**: `##.##` 格式的数值和 `\\` 格式的字符串

**GW-BASIC 完整格式**:
- `#.##` — 自适应格式
- `###,###` — 千位分隔符
- `**##.##` — 前导星号填充
- `$$##.##` — 浮动美元符号
- `+##.##` / `-##.##` — 符号位
- `^^^^` — 科学计数法
- `&` — 字符串可变宽度
- `!` — 单字符

**影响**: 只覆盖了最基础的用例

---

## 代码质量评价

### 优点

- ✅ **模块化良好**: 公共函数 `assign_value()` 消除了代码重复
- ✅ **GW-BASIC 行为兼容**: 数组索引 clamp 而非报错，符合原始行为
- ✅ **防御性编程**: `desc->data` NULL 检查、负索引处理
- ✅ **内存管理对称**: `gwram_alloc` / `gwram_free` 配对使用
- ✅ **测试全面**: 92 个断言覆盖核心模块，全部通过
- ✅ **代码可读性**: 静态函数前置声明，清晰的注释

### 需要改进

- ❌ 缺少数组/变量清理函数（CRITICAL #1, #2）
- ❌ DIM 语句类型支持不完整（HIGH #3）
- ❌ 类型系统使用与声明不一致（HIGH #4）
- ❌ 输入验证依赖 `atoi`（MEDIUM #6）
- ❌ 测试缺少边界和错误场景（LOW #8）

---

## 优先级修复顺序

1. **立即修复**: 添加 `biptrg_free_array` + 在 `gwdata_alloc_variable` 中调用（CRITICAL #1, #2）
2. **高优先级**: DIM 字符串数组支持（HIGH #3）
3. **高优先级**: 数组类型检查（HIGH #4）
4. **中优先级**: DIM 表达式支持（MEDIUM #5）
5. **中优先级**: `atoi` → `strtol`（MEDIUM #6）
6. **低优先级**: 测试覆盖增强（LOW #8）
7. **低优先级**: PRINT USING 完善（LOW #9）

---

## 总结

`ebc500e` 提交有效修复了上一轮 review 中的 4 个问题，代码质量明显提升。`assign_value()` 公共函数的提取使代码结构更清晰，GW-BASIC 兼容的 clamp 行为也更接近原始实现。

当前最大的风险是内存管理：缺少清理函数导致 REPL 长期运行时累积泄漏（CRITICAL #1, #2）。其次是功能完整性：DIM 语句无法声明字符串数组（HIGH #3），以及类型系统使用不一致（HIGH #4）。

建议优先处理 CRITICAL 问题，然后逐步完善 HIGH 和 MEDIUM 项。测试套件已经覆盖了 happy path，可以在此基础上扩展边界测试。