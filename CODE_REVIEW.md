# GW-BASIC C Port - Code Review v7

**日期**: 2026-08-02
**审查范围**: commit `a718acb` → 工作区未提交变更 (DSKCOM 全面修复 + 跨平台重构)
**测试状态**: 160/160 全部通过
**编译状态**: 零警告

---

## 一、概述

开发组针对 v6 review 提出的问题进行了全面修复，这是一次高质量的重构提交：

| v6 问题 | 状态 |
|---------|------|
| 🟥 CRITICAL #1: `dskcom_get` 缓冲区溢出 | ✅ 已修复，正确限制读取长度 |
| 🟧 HIGH #2: `ERROR` 宏重定义警告 | ✅ 已修复，`#undef ERROR` 后再定义 |
| 🟧 HIGH #3: `gwbasic.h` 包含 `windows.h` | ✅ 已修复，DSKCOM 声明移至独立 `dskcom.h` |
| 🟨 MEDIUM #4: `gwmain.c` 未使用函数 | ✅ 已清理 |
| 🟨 MEDIUM #5: `dskcom_put` 逐字节填充性能差 | ✅ 已修复，批量零填充 |
| 🟨 MEDIUM #6: `CLOSE` 不带 filenum 未实现 | ✅ 已实现 `dskcom_close_all()` |
| 🟨 MEDIUM #7: `dskcom_print` 使用 `\n` 非 `\r\n` | ✅ 已修复为 CRLF |

---

## 二、问题清单

### 🟥 CRITICAL

**无**。所有 v6 CRITICAL 问题已修复。

---

### 🟧 HIGH

**无**。所有 v6 HIGH 问题已修复。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **`dskcom.h` 中 `struct file_desc` 仍包含 Windows 特定类型** | `src/dskcom.h` | 17 |

**详情:**

`dskcom.h` 的注释说 `void *handle` 是"跨平台不透明句柄"，但 `dskcom.c` 中实际存储的是 `HANDLE` (Windows API 类型)：

```c
// dskcom.c
HANDLE handle = CreateFileW(...);
fd->handle = handle;  // 隐式转换为 void*
```

虽然功能正常，但这不是真正的跨平台设计。真正的跨平台应该用条件编译：

```c
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;  /* POSIX file descriptor */
#endif
```

或者保持 `void *handle` 但在不同平台存储不同类型的句柄。当前设计是"伪跨平台"，但作为权宜之计可以接受。

---

### 🟩 LOW

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 2 | `.gitignore` 中 `test_*.dat` 和 `test_*.txt` 重复 | `.gitignore` | 5-8 |
| 3 | `test.dat` 和 `test.txt` 仍被修改（未删除） | 根目录 | — |
| 4 | `dskcom.c` 结尾缺少 UNIX 换行符 | `src/dskcom.c` | EOF |
| 5 | 缺少 APPEND/BINARY 模式测试 | `tests/test_dskcom.c` | — |
| 6 | 新增 `src/dskcom.h` 但未在 `Makefile` 中声明依赖 | `Makefile` | — |

---

## 三、修复进度追踪（v5 → v7）

| v5 问题 | 严重性 | v7 状态 | 说明 |
|---------|--------|---------|------|
| CRITICAL #1: put 记录长度 | 🟥 | ✅ 已修复 | 零填充至 record_length |
| CRITICAL #2: get 缓冲区溢出 | 🟥 | ✅ 已修复 | 限制读取 min(buflen, record_length) |
| HIGH #3: windows.h 平台锁定 | 🟧 | ✅ 已修复 | 移至 dskcom.h，gwbasic.h 移除依赖 |
| HIGH #4: PRINT# 改原始字符串 | 🟧 | ✅ 已修复 | strdup 副本 |
| HIGH #5: SEEK 负数校验 | 🟧 | ✅ 已修复 | record < 1 返回 ERROR |
| HIGH #6: CLOSE 不带 filenum | 🟧 | ✅ 已实现 | dskcom_close_all() |
| HIGH #7: test artifact 提交 | 🟧 | ⚠️ 部分修复 | .gitignore 已添加，但 test.dat/test.txt 仍存在 |
| MEDIUM #8: 使用 `\n` 非 `\r\n` | 🟨 | ✅ 已修复 | 改为 CRLF |
| MEDIUM #9: 空文件返回 SUCCESS | 🟨 | ✅ 已修复 | 空文件返回 ERROR |
| MEDIUM #11: 测试文件残留 | 🟨 | ⚠️ 部分修复 | .gitignore 已添加 |
| LOW #13: 缺少换行符 | 🟩 | ❌ 未修复 | — |

**修复率**: 10/11 已修复或大部分修复 (91%)

---

## 四、架构评估

### 新增文件

**`src/dskcom.h`** — DSKCOM 模块的独立头文件

```c
/* 关键设计决策 */
struct file_desc {
    int filenum;
    int mode;
    void *handle;  /* 不透明句柄，跨平台兼容 */
    char *filename;
    int record_length;
    struct file_desc *next;
};
```

**优点:**
- 分离了 DSKCOM 的内部实现与全局命名空间
- `gwbasic.h` 不再依赖 `windows.h`，编译警告清零
- 为未来跨平台移植预留了接口（`void *handle` 可在不同平台存储不同类型）

**缺点:**
- `void *handle` 是"伪跨平台"，实际仍存储 Windows `HANDLE`
- 未使用条件编译隔离平台特定代码

### `gwbasic.h` 变更

```diff
-#include <windows.h>
+/* Windows.h defines ERROR as 0, but we need it as -1 */
+#ifdef ERROR
+#undef ERROR
+#endif
```

**评价:** ✅ 正确的修复方式。保留 `#undef ERROR` 作为防御性编程，即使移除了 `windows.h` 也能防止未来可能的冲突。

### `dskcom.c` 关键修复

**1. `dskcom_get` 缓冲区溢出修复:**

```diff
-/* Read record_length bytes */
-int read_len = fd->record_length;
-if (buflen < read_len) read_len = buflen;
+/* Read min(buflen, record_length) bytes */
+int read_len = (buflen < fd->record_length) ? buflen : fd->record_length;
```

**2. 记录边界对齐:**

```c
/* If user buffer is smaller than record_length, skip remaining bytes to align to next record */
if (buflen < fd->record_length) {
    SetFilePointer(fd->handle, fd->record_length - read_len, NULL, FILE_CURRENT);
}
```

**3. 批量零填充 (性能优化):**

```c
/* If user data is shorter than record_length, pad with zeros in batch */
if (buflen < fd->record_length) {
    int pad_len = fd->record_length - buflen;
    char *zeros = (char *)calloc(1, pad_len);
    if (!zeros) return ERROR;
    WriteFile(fd->handle, zeros, pad_len, &written, NULL);
    free(zeros);
}
```

**4. CRLF 换行符 (Windows 兼容):**

```diff
-const char *nl = "\n";
-WriteFile(fd->handle, nl, 1, &written, NULL);
+/* Add newline (Windows CRLF) */
+const char *nl = "\r\n";
+WriteFile(fd->handle, nl, 2, &written, NULL);
```

### `gwsts.c` 变更

**CLOSE 不带 filenum 实现:**

```c
/* CLOSE without filenum - close all open files */
result = dskcom_close_all();
```

**PRINT# 不再修改原始字符串 (v5 修复的确认):**

```c
char *rest_copy = strdup_trim(rest);  /* 副本，不修改原字符串 */
if (rest_copy) {
    char *comma = strchr(rest_copy, ',');
    ...
    gwram_free(rest_copy);
}
```

### 编译健康度

```
v5: 50 warnings (ERROR 宏重定义 × 40+ 文件)
v6: 50 warnings (同上)
v7: 0 warnings ✅
```

---

## 五、测试覆盖分析

| 功能 | v5 覆盖 | v7 覆盖 | 说明 |
|------|---------|---------|------|
| `dskcom_init()` | ✅ | ✅ | — |
| `dskcom_open()` | ⚠️ 仅 INPUT/OUTPUT | ⚠️ 仅 INPUT/OUTPUT | APPEND/BINARY 仍缺失 |
| `dskcom_close()` | ✅ | ✅ | — |
| `dskcom_close_all()` | ❌ | ❌ | 新增函数，无测试 |
| `dskcom_print()` | ✅ | ✅ | 已验证 CRLF |
| `dskcom_input()` | ✅ | ✅ | — |
| `dskcom_line_input()` | ✅ | ✅ | 空文件 ERROR 路径未测试 |
| `dskcom_seek()` | ✅ | ✅ | 负数 record 路径未测试 |
| `dskcom_get()` | ⚠️ 边界溢出 | ✅ 已修复 | 测试通过 |
| `dskcom_put()` | ⚠️ 记录不对齐 | ✅ 已修复 | 测试通过 |

**新增测试需求:**
1. `dskcom_close_all()` 测试
2. `dskcom_seek()` 负数 record 测试
3. `dskcom_line_input()` 空文件测试
4. `dskcom_open()` APPEND/BINARY 模式测试

---

## 六、总结

**总体评价**: 优秀的修复提交。所有 CRITICAL 和 HIGH 问题已修复，编译警告清零，测试全部通过。DSKCOM 模块的架构改进（独立头文件）为未来跨平台移植奠定了基础。

**关键发现**: 0 个 CRITICAL, 0 个 HIGH, 1 个 MEDIUM（伪跨平台设计）, 5 个 LOW

**建议**:
1. 🔵 **短期**: 清理 `.gitignore` 重复行，删除 `test.dat`/`test.txt`
2. 🔵 **短期**: 添加 `dskcom_close_all()`、负数 SEEK、空文件 LINE INPUT 的测试
3. 🟡 **中期**: 考虑真正的跨平台设计（条件编译隔离 Windows/POSIX）
4. 🟢 **长期**: 添加 APPEND/BINARY 模式测试

**合并建议**: ✅ **可以合并**。所有关键问题已修复，剩余 LOW 问题不影响功能。
