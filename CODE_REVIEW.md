# GW-BASIC C Port - Code Review v90

**日期**: 2026-09-03
**审查范围**: 工作区未提交变更（HEAD = `35e9de1`）— v89 CRITICAL 修复 + 字符串函数重构 + CVI 修复
**测试状态**: 1086/1086 单元测试通过 ✅ | .bas 全量回归恢复 ✅
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: A- (优秀) — v89 CRITICAL 全部修复且实测恢复；根目录调试文件为残留问题

---

## 一、概述

工作区未提交变更（v89 后）修复了 v89 报告的 2 个 CRITICAL，并重构字符串函数：

| 变更 | 内容 | 评估 |
|------|------|------|
| `src/gweval.c` | **比较运算符恢复**（700-705：`<>` `<` `<=` `>` `>=` `=` → -1/0）+ **除零 set_error(11) 恢复**（653）| ✅ v89 CRITICAL-1/2 修复 |
| `src/bistrs.c` | MID$/UCASE$/LCASE$ 等新增/重构（+120）| ✅ |
| `src/gweval.c` | 字符串拼接增强（"Line " + STR$(I) 链式）| ✅ |
| `src/dskcom.c`/`gwbasic.h` | +1 各 | ⚠️ 抽查 |
| `tests/bas/*.expected` ×7 | 行为变化同步（file_rand/line_input/put_get/read_data/resume_line/string_arrays/string_funcs）| ✅ |
| `tests/test_bistrs.c` | +2 断言 | ✅ |
| **根目录 20+ 调试 .bas** | test_debug*/test_cvi*/test_simple*/test_str/test_write*/test_print_num* | ❌ **未跟踪散落**（模式再现）|
| `tests/bas/test_on_error.expected` | 新增但**未跟踪** | ⚠️ |

**实测验证（v89 回归全部恢复 — 输出内容级）:**

| 测试 | v89 | v90 (工作区) | 结论 |
|------|-----|-------------|------|
| test_cond.bas | "FAIL" | **"TRUE = -1 / PASS"** | ✅ 比较运算符修复 |
| test_on_error.bas | handler 未触发 | **"Error handler, ERR= 11"** | ✅ 除零检测修复 |
| test_put_get.bas | "5Record\x04" | " 5Record  5 ..." | ✅ 无乱码 |
| test_read_data.bas | "102030" | " 10 20 30" | ✅ 数字空格恢复 |
| test_def_fn.bas | ✅ | "SQUARE(5) =  25 ..." | ✅ 保持 |
| test_string/string_funcs | ✅ | LEN/LEFT$/RIGHT$/MID$/UCASE$/LCASE$/STR$/VAL/CVI 全对 | ✅ |
| test_multi_array/math | ✅ | PASS / SIN-COS 正确 | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无。（v89 CRITICAL-1/2 已修复并在工作区实测确认）

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 状态 |
|---|------|------|
| 1 | **根目录 20+ 调试 .bas 未跟踪散落**（test_debug*/test_cvi*/test_simple*/test_str/test_write*/test_print_num* — 与 v85/v86 模式再现）| ❌ |
| 2 | **tests/bas/test_on_error.expected 新增未跟踪**（ON ERROR 测试的期望输出未入库 — run_bas_tests.sh 对比失效）| ❌ |
| 3 | **工作区修复未提交** — v89 CRITICAL 修复仍在工作区（HEAD 35e9de1 仍是坏的）— 需提交 | ⚠️ |

**详情:**

#### MEDIUM-1: 根目录调试文件再现（第 2 次）

```
?? test_cvi2.bas / test_cvi_simple.bas / test_debug.bas~6 / test_print_num.bas~3
?? test_simple.bas~5 / test_str.bas / test_write3~5.bas / test_write_read.bas~2
```

- v85 曾出现根目录 .bas 调试文件（ddabca9 清理 + 4058bcb .gitignore）— **本次再现**（20+ 文件）
- .gitignore 模式（`/test_*_dbg*.bas`）未覆盖 `test_debug.bas`/`test_simple.bas` 等新命名
- **建议**: 清理 + .gitignore 扩展（`/test_debug*.bas` `/test_cvi*.bas` `/test_write*.bas` 等）

#### MEDIUM-2: test_on_error.expected 未跟踪

- `tests/bas/test_on_error.expected` 新增但 ?? 状态 → 提交时不会包含 → ON ERROR 测试无期望输出对比

#### MEDIUM-3: 修复未提交

- **当前 HEAD (35e9de1) 的 gweval.c 仍是损坏状态**（比较/除零缺失）— 工作区修复一旦丢失，v89 回归重现
- **建议尽快提交**（v89 CRITICAL 修复是高优先提交）

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | dskcom.c/gwbasic.h 各 +1 行未抽查（FIELDS 相关？）|
| 2 | 7 个 .expected 更新为行为变化（数字空格）— 需确认与 GW-BASIC 语义一致（抽查 read_data " 10 20 30" ✅）|

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 比较运算符恢复（-1/0 语义）| ✅ 正确 — GW-BASIC 核心语义 |
| 除零 set_error(11) 恢复 | ✅ 正确 — ON ERROR 陷阱 |
| MID$/UCASE$/LCASE$ 移入 bistrs.c | ✅ 模块化重构 |
| 字符串拼接链式增强 | ✅ 正确 |
| expected 同步更新 | ✅ 行为变化同步 |
| 根目录调试文件 | ❌ 模式再现（MEDIUM-1）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1086/1086 |
| 比较运算符（cond）| ✅ PASS |
| 除零 + ON ERROR | ✅ ERR= 11 handler |
| 字符串全函数 | ✅（LEN/LEFT$/RIGHT$/MID$/UCASE$/LCASE$/STR$/VAL/CVI）|
| DEF FN | ✅ |
| expected 覆盖 | ⚠️ on_error.expected 未跟踪 |

**测试全面性评分: A-** — 全绿 + 回归恢复；跟踪问题待清理。

---

## 四、改进建议

### 短期 (当前 sprint) — 提交前

1. **🟨 MEDIUM-3**: 提交 v89 CRITICAL 修复（比较/除零）— 高优先（HEAD 仍是损坏态）
2. **🟨 MEDIUM-2**: `git add tests/bas/test_on_error.expected`
3. **🟨 MEDIUM-1**: 清理根目录 20+ 调试 .bas + .gitignore 扩展（防第 3 次）

### 中期 (下个 sprint)

4. 路线剩余: USING 格式集扩充（`+` `-` `*` `$` `,`）
5. pre-commit 工具（根目录 .bas 审计）

### 长期

6. 工作区状态盯控（修复长期滞留在工作区不提交 — 风险管理）

---

## 五、总结

**总体评价**: 工作区未提交变更**修复了 v89 的 2 个 CRITICAL**（比较运算符 -1/0 恢复 + 除零 set_error(11) 恢复），并重构字符串函数（MID$/UCASE$/LCASE$ 移入 bistrs.c）+ 增强字符串拼接。**实测 v89 全部回归恢复**（cond PASS / handler ERR=11 / put_get 无乱码 / read_data 空格）。1086/1086 通过。**主要残留**：修复在工作区未提交（HEAD 35e9de1 仍是损坏态）、根目录 20+ 调试文件再现（第 2 次）、test_on_error.expected 未跟踪。

**关键发现**:
- 🟥 CRITICAL: 0（v89 已修）
- 🟧 HIGH: 0
- 🟨 MEDIUM: 3（调试文件再现、expected 未跟踪、修复未提交）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1086/1086 单元测试
- ✅ 比较运算符（cond "TRUE = -1 / PASS"）
- ✅ 除零 + ON ERROR（"Error handler, ERR= 11"）
- ✅ 字符串全函数 + DEF FN + 回归全绿
- ⚠️ 根目录 20+ 调试文件未跟踪

**建议**: **优先提交 v89 CRITICAL 修复**（当前 HEAD 仍损坏 — 工作区修复是唯一良好状态），然后清理调试文件 + 跟踪 expected。路线: USING 格式集扩充。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-09-03
**审查版本**: v90 — 工作区修复审查（v89 CRITICAL 修复确认 + 字符串重构）