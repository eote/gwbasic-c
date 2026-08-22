# GW-BASIC C Port - Code Review v76

**日期**: 2026-08-23
**审查范围**: HEAD `59a199d` → working tree（递归下降表达式解析器 + EOF API + KILL 增强）
**测试状态**: 1085/1085 单元测试通过 ✅ | .bas: **38/40** ✅
**编译状态**: 无错误、无警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: A- (优秀) — 核心求值器重构零回归，KILL 测试体系就绪

---

## 一、概述

工作区重大变更 — **表达式求值器完整重构**为递归下降解析器：

| 文件 | 变更 | 内容 |
|------|------|------|
| `src/gweval.c` | +257/-231 | **递归下降解析器**（parser_t; parse_expression/term/factor; 完整优先级、一元 +/-、括号、函数调用）|
| `src/dskcom.c`/`h` | +16/+3 | **`dskcom_eof()`** — EOF 检测独立 API |
| `src/gwram.c` | +9 | `gwram_delete_file` 移入（v75 MEDIUM-3 ✅）|
| `src/gwsts.c` | +125 | 语句调整 |
| `tests/test_runner.c` | -20 | 删除旧解析器内部函数测试（scan_var/frqint/chrgt2/synchr — 重构后无对象，正当）|
| `tests/bas/` | +3 新 | test_kill_simple/temp/minimal + test_while_simple 入库 ✅（v75 MEDIUM-2）|

**实测（clean rebuild + 全量）:**

| 项 | 结果 |
|----|------|
| 单元测试 | ✅ 1085/1085（-6：旧解析器测试移除）+ 零警告 |
| **.bas 全量回归** | ✅ **38/40** — 表达式重构**零回归**（math/cond/expr/arrays/multi_array/字符串/WHILE/文件 I/O 全部保持）|
| test_while_simple | ✅ 1..5 Done |
| test_kill_temp（OPEN→PRINT#→CLOSE→KILL）| ✅ 通过（正确模式）|
| test_minimal（`10 END`）| ✅ 通过 |
| **test_kill_simple** | ❌ "Error at line 10"（设计缺陷 — 见 MEDIUM-1）|
| test_read_data | ❌ 预期（READ/DATA 未实现）|

**重构质量评估:** 递归下降解析器替代旧朴素求值 — parse_factor 处理括号/一元/数字/变量/函数，parse_term/expression 处理 * / + - 优先级。**全量 .bas 回归零破坏** — 重构成功的强证据。

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **test_kill_simple.bas 设计缺陷** — KILL 一个从不存在的文件必然失败（GW-BASIC 语义正确，但测试期望 exit 0 不成立）| `tests/bas/test_kill_simple.bas` |
| 2 | **READ/DATA 仍未实现** — test_read_data 卡在 line 10（路线下一项）| `src/gwsts.c` |
| 3 | 3 个新 .bas（kill_simple/kill_temp/minimal）未列入 TEST_CATEGORIES.md / run_bas_tests.sh（归类未同步）| `tests/bas/` `run_bas_tests.sh` |

**详情:**

#### MEDIUM-1: test_kill_simple 测试设计缺陷

```basic
' test_kill_simple.bas
10 KILL "TEST.KILL"     ← 文件从未创建 → remove() 必然失败
20 END
```

- KILL 不存在的文件 → "Error at line 10"（**GW-BASIC 语义正确** — 文件不存在应报错）
- 但测试设计期望验证 KILL 成功？— 前提不成立
- **对比**：test_kill_temp.bas 是正确的模式（先 OPEN 创建 → PRINT# → CLOSE → KILL 已存在文件）

**修复建议（二选一）:**
1. 删除 test_kill_simple（kill_temp 已覆盖成功路径）
2. 或改造成错误路径验证（预期报错 — 放在 Not Implemented/failure 类）

#### MEDIUM-2: READ/DATA 待实现（路线第三项）

test_read_data 仍 "Error at line 10"（`READ A`）— READ/DATA/RESTORE 是 Not Implemented 主类，实现后解锁 read_data/string_arrays。

#### MEDIUM-3: 新测试归类未同步

3 个新 .bas 未加入 TEST_CATEGORIES.md 分类表和 run_bas_tests.sh 数组 — 脚本运行不会包含它们。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | 删除旧解析器测试的 commit 应注明理由（v65"删测试"教训 — 本次有正当理由但未记录）| `tests/test_runner.c` |
| 2 | KILL 错误信息无细节（"Error at line 10" 不含"File not found"）| `src/gwsts.c` |
| 3 | gwram_delete_file / dskcom_eof 无单元测试（.bas 间接覆盖）| `tests/` |

---

## 三、架构评估

### 文件组织

6 源文件 + 4 测试文件变更（587+/325-）。

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 递归下降解析器重构 | ✅ **优秀** — 完整优先级语义；全量回归零破坏 |
| dskcom_eof() 独立 API | ✅ 正确 — 语句层与文件层解耦 |
| gwram_delete_file 移入 gwram.c | ✅ 模块归属修正（v75 MEDIUM-3）|
| 删除旧解析器测试 | ✅ 正当（函数已删 — 无测试对象）|
| kill_temp/minimal 测试 | ✅ 设计正确 |
| kill_simple 测试 | ❌ 设计缺陷（MEDIUM-1）|
| 新测试归类 | ⚠️ 未同步（MEDIUM-3）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1085/1085 |
| .bas 全量回归 | ✅ 38/40（重构零回归）|
| KILL 成功路径 | ✅ kill_temp |
| KILL 错误路径 | ⚠️ 未覆盖（kill_simple 误设计）|
| READ/DATA | ❌ 未实现 |

**测试全面性评分: A-** — 重构验证充分；新测试管理细节待完善。

---

## 四、改进建议

### 短期 (当前 sprint) — 提交前

1. **🟨 MEDIUM-1**: 删 test_kill_simple 或改错误路径预期
2. **🟨 MEDIUM-3**: 3 个新测试加入 TEST_CATEGORIES/run_bas_tests.sh（kill_temp/minimal → Implemented；kill_simple 处理见上）
3. **🟩 LOW-1**: commit message 注明删除旧解析器测试的理由

### 中期 (下个 sprint)

4. **🟨 MEDIUM-2**: 实现 **READ/DATA/RESTORE** → 解锁 read_data/string_arrays
5. 路线: FIELD（随机文件）→ PRINT USING（前导 0）→ ON ERROR/GOTO
6. dskcom_eof/gwram_delete_file 补单元测试

### 长期

7. run_bas_tests.sh 支持 expected-error 类别（kill_simple 类缺陷测试）
8. 新测试入库 pre-commit 检查

---

## 五、总结

**总体评价**: 递归下降表达式解析器重构是高质量核心升级 — 完整运算符优先级，38 个 .bas 全量回归**零破坏**，1085/1085 单测 + 零警告。同期完成 EOF API 独立、gwram_delete_file 归属修正、KILL 测试体系（kill_temp 正确模式/minimal）。**遗留 3 个 MEDIUM**：kill_simple 测试设计缺陷（KILL 不存在文件必然报错）、READ/DATA 待实现、新测试归类未同步。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 0
- 🟨 MEDIUM: 3（kill_simple 缺陷、READ/DATA、归类同步）
- 🟩 LOW: 3

**编译与测试**:
- ✅ 1085/1085 单元测试 + 零警告
- ✅ **38/40 .bas**（表达式重构零回归 — 核心验证）
- ✅ kill_temp / minimal / while_simple
- ❌ kill_simple（设计缺陷 — KILL 不存在文件）
- ❌ read_data（READ 未实现 — 预期）

**v75→v76 追踪**:

| 问题 | v75 | v76 |
|------|-----|-----|
| 表达式求值器 | 朴素 | ✅ **递归下降重构（零回归）** |
| gwram_delete_file 归属 | MEDIUM-3 | ✅ 移入 gwram.c |
| while_simple 入库 | MEDIUM-2 | ✅ 已 add |
| EOF 检测 | gwsts 内联 | ✅ dskcom_eof() API |
| kill_simple 测试 | — | 🟨 设计缺陷（新）|
| READ/DATA | 路线 #3 | ❌ 未实现 |

**建议**: 处理 3 个 MEDIUM（kill_simple 修正、归类同步、删测试理由注明）后提交本批 — 解析器重构是大价值变更。下批：**READ/DATA → FIELD → PRINT USING（修前导 0）**。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-23
**审查版本**: v76 — 递归下降表达式解析器 + EOF API + KILL 测试（工作区未提交）