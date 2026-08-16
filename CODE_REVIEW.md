# GW-BASIC C Port - Code Review v57

**日期**: 2026-08-17
**审查范围**: commit `11fa61d` → `a651c39` — fix: PRINT # file output regression + EOF newlines
**测试状态**: 1084/1084 单元测试通过 ✅ | 13/13 .bas 集成测试通过 ✅
**编译状态**: 无错误、无警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B+ (良好) — v56 HIGH-1 回归修复成功，但新增回归测试无效、缩进问题复发

---

## 一、概述

本次提交 `a651c39` 修复了 v56 报告的 HIGH-1（PRINT # 回归）和 MEDIUM-1（末尾换行）：

| v56 问题 | 状态 | 处理方式 |
|----------|------|----------|
| 🟧 HIGH-1: PRINT # 文件输出回归 | ✅ 已修复 | 跳过可选空格再判断 '#'（`while (*p == ' ') p++;`） |
| 🟨 MEDIUM-1: 4 文件末尾无换行 | ✅ 已修复 | next86.c / gwsts.c / gwmain.c / gwlist.c 全部补齐 |

**实测验证:**
- ✅ 1084/1084 单元测试通过（无 warning）
- ✅ **PRINT # 最小复现通过**：`PRINT #1, "HELLO"` → T1.DAT = `b'HELLO\n'`，stdout 空（v56 时 stdout="0\tHELLO"、文件空）
- ✅ 13/13 .bas 通过（含新增 test_print_file.bas）

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | test_print_file.bas 回归测试**无效** — 不验证文件内容 | `tests/bas/test_print_file.bas` | 全部 |
| 2 | gwsts.c PRINT 控制台块缩进断裂 | `src/gwsts.c` | 194-240 |

**详情:**

#### MEDIUM-1: test_print_file.bas 回归测试无效

```basic
' test_print_file.bas (4 行)
10 OPEN "T1.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 END
```

**问题:** 程序只执行 OPEN → PRINT# → CLOSE → END，**从不读取文件验证内容**。运行结果恒为 exit 0（只要不崩溃），**即使 PRINT # 再次回归（内容写到控制台、文件为空），该测试仍会通过** — 无法捕获 v56 的回归类型。

**验证:** a651c39 修复前的 v56 状态下，此测试也会 exit 0 通过（虽然 PRINT # 已坏）— 它对我的独立复现（检查文件内容）没有帮助。

**修复建议:** 加入回读校验（需要 INPUT#/LINE INPUT# 支持 — 若尚未实现，可用文件系统断言替代）：
```basic
10 OPEN "T1.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 OPEN "T1.DAT" FOR INPUT AS #1
50 LINE INPUT #1, A$
60 IF A$ <> "HELLO" THEN PRINT "FAIL": END
70 CLOSE #1
80 KILL "T1.DAT"
90 PRINT "PASS"
```
或将验证移到 shell/python 层（运行 .bas 后检查文件内容并断言）。

#### MEDIUM-2: gwsts.c PRINT 控制台块缩进断裂

```c
            } else {
                /* Regular PRINT to console */
                char *expr = strdup_trim(trimmed + 5);  /* After "PRINT" */
                if (expr) {
                /* Handle multiple expressions separated by commas/semicolons */  ← 缩进断裂
                char *text = expr;                                               ← 与 if 同级（应 +4）
                bool trailing_comma = false;
                ...
                result = SUCCESS;
            }
        }
    }  /* close else (console output) */
}  /* close else (non-bare PRINT) */
```

`if (expr) {` 之后整个控制台块（`/* Handle...` 到 `result = SUCCESS;`）未增加缩进；末端连续两个 `}` 注释（close else console / close else non-bare）表明嵌套深度已达 4 级。花括号配对正确（编译通过），但可读性差。**这是 v45 LOW-4 以来 PRINT 区域缩进问题的再次复发。**

**修复建议:** 用 clang-format 格式化 gwsts.c，或至少统一该块缩进。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | main() 函数内内联 `extern WORD g_curlin` 仍在 | `src/gwmain.c` 242 |
| 2 | `strcasecmp` 平台依赖（MSVC 不提供）| `src/next86.c` 118 |

---

## 三、架构评估

### 文件组织

| 操作 | 文件 | 变更 |
|------|------|------|
| 修改 | `src/gwsts.c` | +65/-47 (PRINT # 空格处理 + 缩进) |
| 修改 | `src/gwlist.c` | +1 (末尾换行) |
| 修改 | `src/gwmain.c` | +1 (末尾换行) |
| 修改 | `src/next86.c` | +1 (末尾换行) |
| 新增 | `tests/bas/test_print_file.bas` | +4 |

### 设计决策评价

| 决策 | 评价 |
|------|------|
| `while (*p == ' ') p++;` 跳过可选空格 | ✅ 正确 — 兼容 "PRINT#1,X" 和 "PRINT #1,X" |
| 4 文件末尾换行补齐 | ✅ 正确 |
| 新增 test_print_file.bas | ⚠️ 方向对但**无内容断言**（MEDIUM-1）|
| 控制台块缩进 | ❌ 断裂（v45 模式复发）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1084/1084 |
| .bas 集成测试 | ✅ 13/13（含 test_print_file.bas）|
| PRINT# 文件内容验证 | ⚠️ 独立复现已验证（非自动化）|
| test_print_file.bas 有效性 | ❌ 不验证内容（MEDIUM-1）|

**测试全面性评分: B** — 修复功能已被验证（独立复现），但自动化测试无法捕获同类回归。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟨 MEDIUM-1**: 增强 test_print_file.bas（回读校验或脚本级断言），使其真正覆盖文件输出验证
2. **🟨 MEDIUM-2**: 格式化 gwsts.c PRINT 块缩进

### 中期 (下个 sprint)

3. **🟨 遗留**: FOR int 步长 WORD 截断（next86.c:151）
4. **🟨 遗留**: PRINT 字符串泄漏（gwsts.c:199,217 — v50 起）
5. **🟨 遗留**: printf → giocon、strcasecmp → strcasecmp_ascii、main 内联 extern

### 长期

6. 文件 I/O 完整实现（INPUT#/LINE INPUT#/EOF）→ test_file_seq.bas 验收 + test_print_file.bas 增强
7. WHILE/WEND → test_while_wend.bas 验收
8. 引入 clang-format 配置文件统一项目格式

---

## 五、总结

**总体评价**: `a651c39` 正确修复了 v56 的 HIGH-1（PRINT # 回归 — 独立复现确认文件内容写对）和 MEDIUM-1（4 文件末尾换行补齐）。1084 unit + 13 .bas 全部通过。剩余问题为测试有效性（test_print_file.bas 无内容断言）和缩进风格。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 0（v56 的 1 个已修复并实测确认）
- 🟨 MEDIUM: 2（测试无效、缩进断裂）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1084/1084 单元测试通过，无警告
- ✅ 13/13 .bas 通过
- ✅ 独立复现: PRINT #1 → T1.DAT = "HELLO\n" ✓（v56: 文件空 + 控制台 "0\tHELLO"）
- ⚠️ test_print_file.bas 不验证文件内容（回归同类问题仍会漏检）
- ⚠️ gwsts.c PRINT 控制台块缩进断裂

**问题追踪 (v50→v57)**:

| 问题 | 首次报告 | v57 状态 |
|------|----------|----------|
| 指针截断跳转 | v54 CRITICAL | ✅ v55 修复 |
| 错误静默吞掉 | v54 CRITICAL | ✅ v55 修复 |
| PRINT # 回归 | v56 HIGH | ✅ 本提交修复 |
| 末尾换行 | v55/v56 | ✅ 本提交修复（4 文件）|
| FOR 步长截断 | v54 MEDIUM | **遗留** |
| PRINT 字符串泄漏 | v50 HIGH-1 | **遗留** |
| 裸 PRINT / 空语句 | v55 | ✅ v56 修复 |
| test_print_file 无效 | — | MEDIUM-1 (新) |
| PRINT 块缩进断裂 | — | MEDIUM-2 (新) |

**建议**: 本提交可合并。下批迭代建议：增强 test_print_file.bas（真正验证文件内容），然后处理遗留的 PRINT 字符串泄漏（v50 起，字符串堆耗尽风险）和 FOR 步长截断。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-17
**审查版本**: v57 — fix: PRINT # file output regression + EOF newlines