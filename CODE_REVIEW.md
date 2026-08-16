# GW-BASIC C Port - Code Review v55

**日期**: 2026-08-17
**审查范围**: commit `d2f8e08` → `dd2da60` — fix: FOR/NEXT nested loops - fix NEXT increment logic
**测试状态**: 1084/1084 单元测试通过 ✅ | **10/10 .bas 集成测试全部通过** ✅
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B+ (良好) — v54 的 2 个 CRITICAL 全部修复并实测验证，遗留若干 MEDIUM

---

## 一、概述

本次提交 `dd2da60` 是针对性修复提交，修复了 v54 审查报告的核心问题：

| v54 问题 | 状态 | 处理方式 |
|----------|------|----------|
| 🟥 CRITICAL-1: 指针截断跳转机制 | ✅ 已修复 | 全部改为**行号传递**（GOTO/GOSUB/RETURN/FOR） |
| 🟥 CRITICAL-2: 未识别语句被当 END 静默终止 | ✅ 已修复 | END 用专属值 **-2**，"Syntax error" + ERROR |
| 🟧 HIGH-1: "NEXT" 无变量名不支持 | ✅ 已修复 | 空 var_name 匹配栈顶 + gwsts 匹配 "NEXT" |
| 其他 | ✅ 修复 | FOR 递增逻辑（移除 first_iter 分支）|

**修复核心（CRITICAL-1）— 跳转机制从指针改为行号:**

```c
/* dd2da60 前: 指针截断 (v54 CRITICAL) */
return (int)(uintptr_t)target->text;        /* GOTO */
curr = gwlist_find_line_by_text_ptr((WORD)rc);  /* main */

/* dd2da60 后: 行号传递 ✅ */
return line_num;                             /* GOTO/GOSUB: 目标行号 */
return entry->for_next_line;                 /* NEXT: FOR 行下一行号 */
curr = gwlist_find_line((WORD)rc);           /* main: 按行号查找 */
```

彻底消除了 64 位指针截断的未定义行为。`struct for_entry.for_text_ptr` 也更名为 `for_next_line`。

**修复核心（CRITICAL-2）— 错误与 END 区分:**

```c
/* END 专属值 -2 (与 ERROR=-1 区分) */
result = -2;   /* gwsts.c END 分支 */
if (rc == -2) { break; }   /* main: END */

/* 未识别语句报错可见 */
printf("Syntax error: %s\n", trimmed);   /* gwsts.c 最终 else */
```

**实测验证（10/10 .bas 全部通过）:**

| .bas 测试 | 退出码 | 输出 | 状态 |
|-----------|--------|------|------|
| test_hello.bas | 0 | Hello, World | ✅ |
| test_for.bas | 0 | Hello ×5 (循环) | ✅ |
| test_for2.bas | 0 | 1-30 (混合步进) | ✅ |
| test_for3.bas | 0 | 嵌套循环 I/J 结果 | ✅ |
| test_if.bas | 0 | A>3 / A<10 / A=5 | ✅ |
| **test_goto_gosub.bas** | **0** | **Start / After GOTO / In subroutine / Back** | ✅ **v54 失败→已修复** |
| test_let.bas | 0 | 5Hello3.140 | ✅ |
| **test_next_no_var.bas** | **0** | **1 2 3 4 5** | ✅ **新增，NEXT 无变量** |
| **test_syntax_error.bas** | **-1** | **Syntax error: XXYYZZ / Error at line 10** | ✅ **新增，错误可见** |
| test_simple.bas | 0 | test | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无。（v54 的 2 个 CRITICAL 均已修复）

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | "PRINT"（无参数）报 Syntax error — GW-BASIC 语义应为输出空行 | `src/gwsts.c` | 160, 599 |
| 2 | 空字符串语句 "" 报 "Syntax error:" 噪音 | `src/gwsts.c` | 599 |
| 3 | `gwlist_find_line_by_text_ptr` 死代码（无调用者） | `src/gwlist.c` `src/gwbasic.h` | 99, 229 |
| 4 | FOR 整数步长 WORD 截断（v54 MEDIUM-1 遗留） | `src/next86.c` | 151 |
| 5 | PRINT 字符串泄漏 — v50 HIGH-1 仍遗留 | `src/gwsts.c` | 195, 213 |

**详情:**

#### MEDIUM-1: "PRINT" 无参数报语法错误（GW-BASIC 语义不符）

```c
/* gwsts.c — 实际行为 */
else if (starts_with(trimmed, "PRINT ")) { ... }   /* 只匹配带空格 */
/* "PRINT" (无参数) 落到最终 else → "Syntax error: PRINT" */

/* 单元测试输出（1084 全通过但 stdout 被污染）*/
RUN: gwsts_empty_input... Syntax error: 
RUN: gwsts_print_empty... Syntax error: PRINT
```

GW-BASIC 中 `PRINT` 单独成行输出一个空行（合法且常用）。当前实现：
- `"PRINT"` 无参数 → 最终 else → `Syntax error: PRINT` 输出 + ERROR
- 测试 `test_gwsts_print_empty` 注释明确期望 "PRINT alone prints a newline, should succeed"，但断言只 `TEST_PASS` 未检查返回值 — **测试断言太弱掩盖了行为缺陷**

**修复建议:**
```c
else if (starts_with(trimmed, "PRINT")) {
    if (trimmed[5] == '\0' || ...) { printf("\n"); result = SUCCESS; }
    else if (trimmed[5] == ' ') { ...现有处理... }
}
/* 且空字符串/纯空白语句应返回 SUCCESS: */
if (!*trimmed) { gwram_free(trimmed); return SUCCESS; }
```

#### MEDIUM-3: gwlist_find_line_by_text_ptr 死代码

dd2da60 将 main 改为 `gwlist_find_line((WORD)rc)`（按行号）后，`gwlist_find_line_by_text_ptr`（gwlist.c:99 定义 + gwbasic.h:229 声明）**已无任何调用者**。应删除（声明+定义），避免后续误用及触发编译器 unused 警告。

#### MEDIUM-4: FOR 整数步长 WORD 截断（v54 遗留）

```c
/* next86.c int_next 仍保留截断 */
v->value.i += (WORD)entry->step;   /* float step → WORD：STEP 1.5 变 +1 */
```

v54 报告后未修复。整数 FOR 的浮点步长会截断（`FOR I=1 TO 10 STEP 0.5` 步进错误）。虽然 `next86_for` 用默认 VT_SINGLE 使多数循环走 float 分支（无截断），int 分支仍存在缺陷。

#### MEDIUM-5: PRINT 字符串泄漏（v50 HIGH-1 仍遗留）

`gwsts.c` 的 PRINT console 路径 (line 195) 和文件路径 (line 213)：
```c
char *str_val = gweval_eval_string(...);
if (str_val) {
    printf("%s", str_val);   /* 未释放 — bump allocator 不回收 */
}
```
字符串堆 (16KB) 在连续字符串 PRINT 后耗尽，之后字符串输出静默变 "0"。v50/v54 已两次报告，尚未修复。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | next86.c / gwsts.c / gwmain.c 三个文件末尾均无换行符 | 3 个文件 EOF |
| 2 | `for_entry.text_ptr` 字段注释未更新（GOSUB 栈中存行号，字段名 text_ptr 易误导） | `src/next86.c` 372 |
| 3 | main() 函数内内联 `extern WORD g_curlin` 仍在 | `src/gwmain.c` 242 |

---

## 三、架构评估

### 文件组织

| 操作 | 文件 | 变更 |
|------|------|------|
| 修改 | `src/next86.c` | +97/-74 (跳转行号化 + NEXT 递增修复) |
| 修改 | `src/gwsts.c` | +13 (NEXT/END/语法错误) |
| 修改 | `src/gwmain.c` | +10 (跳转行号化 + END=-2) |
| 修改 | `src/gwbasic.h` | +4 (for_next_line 重命名) |
| 新增 | `tests/bas/test_next_no_var.bas` | +3 |
| 新增 | `tests/bas/test_syntax_error.bas` | +2 |
| 修改 | `CODE_REVIEW.md` | v54→v55 文档 |

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 跳转改行号传递 | ✅ 优秀 — 消除指针截断 UB，语义清晰 |
| NEXT 空变量名匹配栈顶 | ✅ 正确 — 符合 GW-BASIC 语义 |
| END 用 -2 专属值 | ✅ 正确 — 与 ERROR(-1) 区分 |
| 未识别语句打印 Syntax error | ✅ 正确 — 错误可见 |
| FOR 递增移除 first_iter 分支 | ✅ 正确 — NEXT 总是先加后比较（GW-BASIC 语义） |
| 新 .bas 测试 (next_no_var / syntax_error) | ✅ 优秀 — 针对性覆盖修复的功能 |
| 死代码 gwlist_find_line_by_text_ptr 未删除 | ⚠️ 应删除 |
| "PRINT" 无参数仍报错 | ❌ GW-BASIC 语义不符 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1084/1084 |
| .bas 集成测试 | ✅ 10/10（含新增 2 个）|
| 错误路径（语法错误）| ✅ 可见 + 非零退出码 |
| 嵌套循环 (for3) | ✅ 通过 |
| NEXT 无变量 | ✅ 通过 |
| PRINT 无参数 | ❌ 报错（语义不符）|
| 字符串堆压力 (PRINT 大量字符串) | ⚠️ 无测试（MEDIUM-5）|

**测试全面性评分: A-** — 主要功能和错误路径均有覆盖，但 PRINT 无参数行为与测试断言强度（TEST_PASS 不查状态）是弱点。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟨 MEDIUM-1/2**: 修复 "PRINT" 无参数（输出空行）和空语句（no-op），并在 test_gwsts_print_empty 增加返回值断言
2. **🟨 MEDIUM-3**: 删除 gwlist_find_line_by_text_ptr 死代码
3. **🟩 LOW-1**: 补三个文件末尾换行

### 中期 (下个 sprint)

4. **🟨 MEDIUM-4**: FOR int 步长改用 float 全程
5. **🟨 MEDIUM-5**: PRINT 字符串释放（v50 起的遗留 HIGH — 建议优先）
6. **🟨 遗留项**: printf 改 giocon、strcasecmp_ascii 替换 strcasecmp、main 内联 extern 清理

### 长期

7. .bas 集成测试自动化接入（含 stdout 断言）
8. 强度断言：测试应检查返回值/输出而非仅 TEST_PASS

---

## 五、总结

**总体评价**: `dd2da60` 是一次高质量的针对性修复 — v54 报告的 2 个 CRITICAL（指针截断跳转、错误静默吞掉）和 HIGH（NEXT 无变量）全部修复，且经过独立实测验证（10/10 .bas 通过，含之前失败的 test_goto_gosub.bas）。FOR 递增逻辑修复符合 GW-BASIC 语义。

**关键发现**:
- 🟥 CRITICAL: 0（v54 的 2 个已修复）
- 🟧 HIGH: 0（v54 的 1 个已修复）
- 🟨 MEDIUM: 5（2 新 + 3 遗留）
- 🟩 LOW: 3

**编译与测试**:
- ✅ 1084/1084 单元测试通过
- ✅ 10/10 .bas 集成测试通过（实测确认）
- ✅ test_goto_gosub.bas: "Start / After GOTO / In subroutine / Back from GOSUB" — v54 失败已修复
- ✅ test_syntax_error.bas: 错误可见 + exit -1
- ✅ test_next_no_var.bas: 1 2 3 4 5
- ⚠️ 单元测试 stdout 污染: "Syntax error: PRINT" / "Syntax error: "（MEDIUM-1/2）

**问题追踪 (v50→v55)**:

| 问题 | v50 | v53 | v54 | v55 |
|------|-----|-----|-----|-----|
| 指针截断跳转 | — | — | CRITICAL-1 | ✅ 修复 |
| 错误静默吞掉 | — | — | CRITICAL-2 | ✅ 修复 |
| NEXT 无变量 | — | — | HIGH-1 | ✅ 修复 |
| PRINT 字符串泄漏 | HIGH-1 | 代码还原 | 重现 | **遗留** (MEDIUM-5) |
| FOR 步长截断 | — | — | MEDIUM-1 | **遗留** (MEDIUM-4) |
| printf 绕过 giocon | MEDIUM | 代码还原 | 重现 | **遗留** |
| 退出码/调试/缩进/误提交 | — | ✅ 修复 | — | — |
| PRINT 无参数报错 | — | — | — | **MEDIUM-1 (新)** |
| 死代码 | — | — | — | **MEDIUM-3 (新)** |

**建议**: 本次提交可合并。下批迭代优先处理 MEDIUM-5（PRINT 字符串泄漏 — 是 v50 起的遗留，字符串堆耗尽会导致 PRINT 静默输出 "0"）和 MEDIUM-1（PRINT 无参数语义）。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-17
**审查版本**: v55 — fix: FOR/NEXT nested loops - fix NEXT increment logic