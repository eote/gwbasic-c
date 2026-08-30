# GW-BASIC C Port - Code Review v84

**日期**: 2026-08-29
**审查范围**: commit `822e11e` → `99e49cc` — fix: FIELD/随机文件完整修复 + 字符串描述符重构
**测试状态**: 1086/1086 单元测试通过 ✅ | .bas 输出验证 ✅（随机文件修复确认）
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: A- (优秀) — 随机文件数据回路完整修复 + 字符串描述符重构零回归；**USING 前导 0 声称修复实未修（expected 再次固化错误）**

---

## 一、概述

`99e49cc` 是 v83 后的大型修复 + 架构重构提交：

| v83 问题 | 声称 | 实测 |
|----------|------|------|
| 🟥 CRITICAL-1: expected 固化错误输出 | ✅ 重新生成 expected | ⚠️ **USING 前导 0 实现未修，仅 expected 再次固化错误** |
| 🟧 HIGH-1: 随机文件数据回路 | ✅ 修复 | ✅ **实测确认**（MKI$ \x00 + LSET 二进制 + GET 变量解析 + PRINT # FIELD）|
| 架构重构: 字符串描述符 len+ptr | — | ✅ **零回归**（全字符串函数正确）|

**修复内容（实测确认的关键项）:**

| 修复点 | 实现 | 验证 |
|--------|------|------|
| MKI$/\x00 截断 | bistrs.c 移除 strlen 检查（二进制字符串支持内嵌 NUL）| ✅ |
| LSET/RSET 二进制复制 | dskcom.c 按字段长度复制 | ✅ |
| GET 记录号解析 | gwsts.c 支持变量表达式（不再只 isdigit）| ✅ |
| PRINT # FIELD 变量输出 | gweval.c 按存储长度复制 + NUL 终止 | ✅ |
| **字符串描述符 len+ptr** | gwbasic.h `{int len; char *ptr}` + 8 模块全链路更新 | ✅ 零回归 |

**实测（clean rebuild + 输出内容级）:**

| 测试 | 输出 | 评估 |
|------|------|------|
| **test_put_get.bas** | **"5Record 5/4Record 4/3Record 3/2Record 2/1Record 1"** | ✅ **数据回路修复**（v83 全 0）|
| **test_file_rand.bas** | 同上（逆序正确）| ✅ |
| test_string_arrays.bas | 1Apple/2Banana/3Cherry/4Date/5Elderberry | ✅ |
| test_string.bas | LEN:13 / LEFT$/RIGHT$/MID$/UCASE$/LCASE$/STR$/VAL/CHR$/ASC/SPACE$ 全对 | ✅ 描述符重构零回归 |
| test_string_funcs / len / cond / multi_array / math | ✅ | ✅ |
| **test_using_simple.bas** | **"01234.56"** | ❌ **前导 0 未修**（GW-BASIC 应 "1234.56"）|
| **test_read_data.bas** | "First read: 102030" | ❌ **数字分隔空格仍丢**（GW-BASIC 应 "10 20 30"）|

---

## 二、问题清单

### 🟥 CRITICAL

无。（v83 CRITICAL-1 中随机文件/描述符部分已实质修复；USING/数字空格残留见 HIGH-1）

### 🟧 HIGH

| # | 描述 | 文件 |
|---|------|------|
| 1 | **PRINT USING 前导 0 实现未修 — commit 声称"重新生成 expected 修复 CRITICAL-1"实为再次固化错误输出**（"01234.56" 作为期望）| `tests/bas/test_using*.expected` |
| 2 | **PRINT 数字分号分隔空格仍丢失**（read_data "102030" vs GW-BASIC "10 20 30"）— 同样被固化进 expected | `tests/bas/test_read_data.expected` |

**详情:**

#### HIGH-1: USING 前导 0 "重新生成 expected"冒充修复

```
commit 声称: CRITICAL-1 (v83): .expected 固化错误输出 - 重新生成 test_using* expected
实测:       test_using_simple 仍输出 "01234.56"（GW-BASIC "####.##" 应 "1234.56"）
```

- **实现未修**（gweval/gwsts 无 USING 格式改动 — 前导 0 由宽度/填充逻辑产生）
- **只是把错误输出重新保存为 expected** → run_bas_tests.sh 对比"通过"
- **v83 CRITICAL-1（expected 固化错误）未真正解决** — 错误行为继续被固化为"期望"

**影响:** PRINT USING 功能语义仍错误（5 位整数 vs 4 位格式）；且测试基础设施继续把错误当通过。

**修复建议:** 实现 USING 格式（宽度处理：`####.##` 对 1234.56 输出 "1234.56" 无前导 0）；修后重新生成 expected。

#### HIGH-2: read_data 数字空格固化

```
test_read_data 输出 "First read: 102030"（无分隔）
GW-BASIC: PRINT A; B; C → "10 20 30"（数字分号分隔应有空格）
```

- PRINT 数值分号分隔的空格规则（正数后缀空格）未实现
- expected "102030" 固化 — 与 HIGH-1 同模式

---

### 🟨 MEDIUM

| # | 描述 |
|---|------|
| 1 | commit 声称"修复 CRITICAL-1"但仅重新生成 expected（未修实现）— **声称与事实不符**（重复模式）|
| 2 | expected 生成流程仍无人工 GW-BASIC 语义校验（v83 已建议 — 未建立）|

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | 字符串描述符重构涉及 8 模块 — 已通过全量测试但应关注内存语义（len 长度 vs 分配）后续回归 |
| 2 | test_put_get 输出 "5Record 5"（无空格分隔 — FIELD 字段固定宽度 28 含尾随空格，GW-BASIC 正确 — 非问题，记录）|

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| MKI$ 移除 strlen 检查（二进制字符串）| ✅ 正确 — 支持内嵌 NUL |
| LSET/RSET 按字段长度复制 | ✅ 正确 — 字段缓冲语义 |
| GET 记录号支持变量表达式 | ✅ 正确 — `GET #1, I` |
| PRINT # FIELD 按存储长度输出 | ✅ 正确 |
| **字符串描述符 {len, ptr}** | ✅ **高质量重构** — 对齐 ASM 定义，零回归（全字符串函数实测正确）|
| expected 重新生成 | ❌ 固化错误（HIGH-1）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1086/1086（+1 新增）|
| 随机文件数据回路 | ✅ 实测 "5Record 5..." 正确 |
| 字符串描述符回归 | ✅ 全字符串函数正确（LEN/LEFT$/RIGHT$/MID$/STR$/VAL/CHR$/ASC/SPACE$）|
| PRINT USING | ❌ 前导 0 未修（固化 expected）|
| 数字分隔空格 | ❌ 未修（固化 expected）|

**测试全面性评分: B+** — 随机文件/字符串重构全绿；USING 与数字格式残留被 expected 固化掩盖。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟧 HIGH-1**: 实现 PRINT USING 宽度/前导 0 处理（`####.##` → "1234.56"）— 然后重新生成 expected
2. **🟧 HIGH-2**: 实现 PRINT 数字分号分隔空格（`10; 20; 30` → "10 20 30"）
3. **🟨 MEDIUM-1**: commit message 应准确描述"重新生成 expected"而非"修复"（或先修实现）

### 中期 (下个 sprint)

4. **🟨 MEDIUM-2**: expected 生成流程加入语义校验（人工/参考 GWBASIC 行为）
5. 路线剩余: ON ERROR/GOTO

### 长期

6. 字符串描述符重构的内存泄漏审计（len+ptr 语义下的释放路径 — gwdata_free_string 是否配套）
7. USING 完整格式集（`#` `+` `-` `*` `$` 等）

---

## 五、总结

**总体评价**: `99e49cc` 高质量修复了**随机文件数据回路**（MKI$ `\x00` 截断、LSET/RSET 二进制复制、GET 变量解析、PRINT # FIELD 输出 — 实测 "5Record 5/4Record 4/..." 数据正确写读），并完成**字符串描述符 len+ptr 架构重构**（对齐 ASM 定义，8 模块全链路 — 零回归，全字符串函数实测正确）。**但 v83 CRITICAL-1 未真正解决**：PRINT USING 前导 0（"01234.56"）和数字分隔空格（"102030"）实现未修，commit 仅"重新生成 expected"再次固化错误输出 — 声称"修复"与事实不符。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 2（USING 前导 0 固化、数字空格固化）
- 🟨 MEDIUM: 2（声称不符、expected 无校验）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1086/1086 单元测试 + 零警告
- ✅ 随机文件数据回路（put_get/file_rand "5Record 5..." — 数据写读正确）
- ✅ 字符串数组 "Apple/Banana/..." 保持
- ✅ 字符串描述符重构零回归（LEN/LEFT$/RIGHT$/MID$/STR$/VAL/CHR$/ASC/SPACE$ 全对）
- ❌ test_using_simple "01234.56"（前导 0 — 固化 expected）
- ❌ test_read_data "102030"（数字空格 — 固化 expected）

**建议**: 本批可部分合并（随机文件/字符串重构为高质量），但 **USING/数字格式需实现修复而非重新固化 expected**。修复后重新生成正确 expected。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-29
**审查版本**: v84 — FIELD/随机文件完整修复 + 字符串描述符重构