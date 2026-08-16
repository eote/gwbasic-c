# GW-BASIC C Port - Code Review v56

**日期**: 2026-08-17
**审查范围**: commit `dd2da60` → `11fa61d` — fix: CODE_REVIEW.md v55 MEDIUM issues
**测试状态**: 1084/1084 单元测试通过 ✅ | 12/12 .bas 集成测试通过 ✅
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B (良好) — v55 MEDIUM 大部分修复，但引入 PRINT # 文件输出回归

---

## 一、概述

本次提交 `11fa61d` 修复了 v55 报告的 MEDIUM-1/2/3 和 LOW-1 声称的部分问题：

| v55 问题 | 声称状态 | 实际状态 |
|----------|----------|----------|
| 🟨 MEDIUM-1: "PRINT" 无参数报错 | ✅ 已修复 | ✅ **实测验证**：test_print_bare.bas 通过（输出空行） |
| 🟨 MEDIUM-2: 空语句报错噪音 | ✅ 已修复 | ✅ **实测验证**：test_empty_stmt.bas 通过（no-op） |
| 🟨 MEDIUM-3: 死代码 | ✅ 已修复 | ✅ gwlist.c:99 定义 + gwbasic.h:229 声明已删除 |
| 🟩 LOW-1: 三文件末尾无换行 | ✅ 声称已修复 | ❌ **实际未修复**（见 MEDIUM-1 新问题）|

**⚠️ 新回归:** "PRINT #"（文件输出）路径被破坏 — 见 HIGH-1。

**实测（12/12 .bas 通过）:**

| .bas 测试 | 退出码 | 输出 | 状态 |
|-----------|--------|------|------|
| 原有 10 个（hello/for/for2/for3/if/goto_gosub/let/next_no_var/syntax_error/simple） | 0/-1 | 与 v55 一致 | ✅ 无回归 |
| **test_print_bare.bas** (新) | 0 | Before \| *(空行)* \| After | ✅ 裸 PRINT = 空行 |
| **test_empty_stmt.bas** (新) | 0 | Before \| After | ✅ 空语句 no-op |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **PRINT # 文件输出回归** — "PRINT #1,X"（带空格）不再写入文件，改输出到控制台 | `src/gwsts.c` | 162-167 |

**详情:**

#### HIGH-1: PRINT # 文件输出回归（11fa61d 引入）

**修复前的正确逻辑 (v54/v55):**
```c
else if (starts_with(trimmed, "PRINT ")) {   /* "PRINT " 带空格前缀 */
    if (trimmed[6] == '#') { ...文件输出... }  /* "PRINT #1,X" 的 [6]='#' ✅ */
    else { ...控制台输出... }
}
```

**修复后的错误逻辑 (11fa61d):**
```c
else if (starts_with(trimmed, "PRINT")) {    /* "PRINT" 无空格前缀 */
    if (trimmed[5] == '\0') { ...裸 PRINT 空行... }
    else if (trimmed[5] == '#') { ...文件输出... }   /* 只匹配 "PRINT#1,X"（无空格）*/
    else { ...控制台输出... }
}
```

**根因:** 标准语法 `PRINT #1, X`（带空格）的 `trimmed[5]` 是**空格**（' '），不是 '#' → 永远不进入文件输出分支。只有非标准的 `PRINT#1,X`（无空格）能匹配。

**实测复现（最小验证）:**
```basic
10 OPEN "T1.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 END
```
**结果:**
- stdout: `0\tHELLO\n`（被当控制台输出 — `#1, "HELLO"` 求值为 0 + HELLO）
- T1.DAT: **创建但内容为空 b''**（OPEN 成功、PRINT 未写入）

**影响:** 文件顺序输出功能被破坏。现有的 test_file_seq.bas 使用 `PRINT #1, I; "Data line"`（第 30 行，带空格）— 一旦实现 WHILE/LINE INPUT 使该测试可运行，会立即失败。

**修复建议:**
```c
else if (starts_with(trimmed, "PRINT")) {
    if (trimmed[5] == '\0') { printf("\n"); result = SUCCESS; }
    else {
        /* 定位 '#'：允许 "PRINT#..." 或 "PRINT #..." */
        char *hash = trimmed + 5;
        if (*hash == ' ') hash++;
        if (*hash == '#') {
            /* 文件输出: hash+1 是文件号 */
            char *rest = hash + 1;
            int filenum = (int)gweval_eval_numeric(rest);  /* 注意: 应跳过空格 */
            ...
        } else {
            /* 控制台输出 */
            char *expr = strdup_trim(hash);
            ...
        }
    }
}
```

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | v55 LOW-1 声称修复末尾换行但实际未修复（4 个文件仍无换行） | `next86.c` `gwsts.c` `gwmain.c` `gwlist.c` | EOF |
| 2 | FOR 整数步长 WORD 截断（v55 MEDIUM-4 遗留） | `src/next86.c` | 151 |
| 3 | PRINT 字符串泄漏（v50 HIGH-1 / v55 MEDIUM-5 遗留） | `src/gwsts.c` | 199, 217 |
| 4 | PRINT # 无 .bas 测试覆盖（回归没有被测试捕获） | `tests/bas/` | — |

**详情:**

#### MEDIUM-1: 末尾换行声称修复但实际未修复

commit message 声称 "Fixed missing EOF newlines in next86.c, gwsts.c, gwmain.c"，但实测 **4 个文件末尾均无换行符**（next86.c / gwsts.c / gwmain.c / gwlist.c）。git diff 也确认：提交只改了 gwsts.c 和 gwlist.c 的内容，且末尾未补 `\n`；next86.c 和 gwmain.c 完全未修改。**这是对 v46 gwlist.c 事件的重复** — commit message 与实际变更不符。

#### MEDIUM-4: PRINT # 回归无测试覆盖

12 个 .bas 测试不含文件输出验证（test_file_seq.bas 依赖未实现的 WHILE/LINE INPUT 无法运行）。HIGH-1 回归因此未被测试捕获。**建议独立添加最小 PRINT# 测试**（OPEN + PRINT# + CLOSE + 检查文件内容 + KILL）或单元测试直接调用 gwsts_exec_statement("PRINT #1, ...") 并检查 dskcom 输出。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | 4 源文件末尾无换行符（同 MEDIUM-1，重复记录） | 4 文件 |
| 2 | main() 函数内内联 `extern WORD g_curlin` 仍在 | `src/gwmain.c` |
| 3 | `strcasecmp` 平台依赖（MSVC 不提供）| `src/next86.c` |

---

## 三、架构评估

### 文件组织

| 操作 | 文件 | 变更 |
|------|------|------|
| 修改 | `src/gwsts.c` | +20 (PRINT 裸输出/空语句/PRINT# 检查) |
| 修改 | `src/gwlist.c` | -12 (死代码删除) |
| 修改 | `src/gwbasic.h` | -1 (声明删除) |
| 新增 | `tests/bas/test_print_bare.bas` | +3 |
| 新增 | `tests/bas/test_empty_stmt.bas` | +3 |

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 裸 PRINT 输出空行 (`trimmed[5]=='\0'`) | ✅ 正确 — GW-BASIC 语义 |
| 空语句 no-op (`!*trimmed → SUCCESS`) | ✅ 正确 |
| 死代码删除 | ✅ 正确 |
| "PRINT" 前缀 + [5]=='#' 检查 | ❌ **回归** — 未处理 "PRINT #" 的空格 |
| 新 .bas 测试 (print_bare / empty_stmt) | ✅ 优秀 — 覆盖修复的功能 |
| 但 PRINT # 无测试 | ⚠️ 盲区 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| 裸 PRINT | ✅ test_print_bare.bas |
| 空语句 | ✅ test_empty_stmt.bas |
| PRINT # 文件输出 | ❌ **未覆盖（回归逃脱）** |
| FOR 步长截断 | ❌ 未覆盖 |
| 字符串堆压力 | ❌ 未覆盖 |

**测试全面性评分: B** — 修复的功能有测试，但 PRINT # 关键路径无覆盖。

---

## 四、改进建议

### 短期 (当前 sprint) — 立即修复

1. **🟧 HIGH-1**: 修复 PRINT # 空格处理（跳过可选空格再判断 '#'）+ 添加最小 PRINT# 回归测试
2. **🟨 MEDIUM-1**: 实际补上 4 个文件的末尾换行（或改用 .gitattributes 全局管理）

### 中期 (下个 sprint)

3. **🟨 MEDIUM-2**: FOR int 步长改 float
4. **🟨 MEDIUM-3**: PRINT 字符串释放（v50 起遗留 — 建议彻底解决：字符串所有权约定 + 释放）
5. **🟨 遗留**: printf → giocon、strcasecmp → strcasecmp_ascii、main 内联 extern 清理

### 长期

6. 文件 I/O 功能完整（OPEN/PRINT#/INPUT#/EOF/LINE INPUT）→ test_file_seq.bas 验收
7. WHILE/WEND 实现 → test_while_wend.bas 验收

---

## 五、总结

**总体评价**: 本次提交干净修复了 v55 的 MEDIUM-1/2/3（裸 PRINT、空语句、死代码），新增的 2 个 .bas 测试实测通过，无回归的原有 10 个测试。**但引入了一个 HIGH 回归**：PRINT #（带空格）文件输出路径被破坏，且该路径无测试覆盖。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 1 (**PRINT # 文件输出回归** — 11fa61d 引入)
- 🟨 MEDIUM: 4 (换行声称未修复 + 3 遗留)
- 🟩 LOW: 3

**编译与测试**:
- ✅ 1084/1084 单元测试通过
- ✅ 12/12 .bas 通过（含新增 test_print_bare / test_empty_stmt）
- ❌ 最小复现: `PRINT #1, "HELLO"` → 控制台 "0\tHELLO"、T1.DAT 空（回归）
- ⚠️ 4 个源文件末尾仍无换行（声称修复但未改）

**问题追踪 (v55→v56)**:

| 问题 | v55 | v56 |
|------|-----|-----|
| 裸 PRINT / 空语句 | MEDIUM-1/2 | ✅ 修复 |
| 死代码 | MEDIUM-3 | ✅ 修复 |
| PRINT # 文件输出 | — | **HIGH-1 回归 (新)** |
| 末尾换行声称未改 | — | MEDIUM-1 (新) |
| FOR 步长截断 | MEDIUM-4 | **遗留** |
| PRINT 字符串泄漏 | MEDIUM-5 | **遗留** |

**建议**: 优先修复 HIGH-1（PRINT # 回归 — 一行逻辑 + 一个回归测试），这是本次提交引入的功能缺陷。修复后可合并。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-17
**审查版本**: v56 — fix: CODE_REVIEW.md v55 MEDIUM issues