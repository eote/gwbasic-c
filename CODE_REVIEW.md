# GW-BASIC C Port - Code Review v83

**日期**: 2026-08-29
**审查范围**: commit `b3ac158` → `822e11e` — fix: FIELD 语句修复 + 6 测试移入 Implemented
**测试状态**: 1085/1085 单元测试通过 ✅ | .bas "56/56" ⚠️（expected 固化错误输出）
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: C- (及格) — FIELD 执行修复 + 字符串数组修复，但随机文件数据回路缺陷 + expected 固化错误输出

---

## 一、概述

`822e11e` 修复 v82 报告的 HIGH-1（FIELD）+ 声称修复 HIGH-2，并将 6 个测试移入 Implemented：

| v82 问题 | 声称 | 实测 |
|----------|------|------|
| 🟧 HIGH-1: FIELD 语句报错 | ✅ 已修复 | ✅ **FIELD 可执行**（根因：赋值分支误截获 `OPEN ... LEN=32` 的 '=' — 排除关键字修复）|
| 🟧 HIGH-2: 字符串数组 DATA 损坏 | ✅ 声称已修 | ✅ **"1Apple/2Banana/..." 正确**（gweval 修复生效）|
| 6 测试移入 Implemented | 声称 56/56 | ⚠️ **using 前导 0 未修 + put_get 数据全 0 — 错误被固化为 expected** |

**实测（clean rebuild）:**

| 测试 | 输出 | 评估 |
|------|------|------|
| test_put_get.bas | `0\n0\n0\n0\n0\n` | ⚠️ **应有 "5 Record 5/4 Record 4..." — 数据回路坏（CVI=0、NAME$ 空）** |
| test_file_rand.bas | `0`×5 | ⚠️ 同 put_get |
| test_string_arrays.bas | "1Apple/2Banana/3Cherry/4Date/5Elderberry" | ✅ 修复确认 |
| **test_using_simple.bas** | **"01234.56"** | ❌ **前导 0 未修**（GW-BASIC 应 "1234.56"）|
| **test_print_using.bas** | "01234.56/01.23457e+06/0-12.34/0Hello World" | ❌ 前导 0 全家错误 |
| **test_read_data.bas** | "First read: 102030" | ⚠️ 数字分隔空格丢失（GW-BASIC 应 "10 20 30"）|
| 回归（file_seq/string/multi_array/math）| ✅ | 保持 |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 |
|---|------|------|
| 1 | **expected 文件固化错误输出** — 3 处 bug 输出被保存为"期望 → run_bas_tests.sh 报 PASS" | `tests/bas/*.expected` |

**详情:**

#### CRITICAL-1: .expected 固化错误输出（测试基础设施成为 bug 固化器）

| expected 文件 | 内容 | 问题 |
|---------------|------|------|
| test_using_simple.expected | `01234.56` | **PRINT USING 前导 0 错误被固化**（GWBASIC: `"####.##"; 1234.56` → "1234.56"）|
| test_print_using.expected | `01234.56...` | 同上（3 条全错）|
| test_put_get.expected | `0 ×5` | **随机文件数据回路失败的输出被固化** |
| test_read_data.expected | `102030` | **PRINT 数字分隔空格缺失被固化**（GWBASIC "10 20 30"）|

**影响:**
- 822e11e 声称 "56/56 .bas 通过" 是基于**错误 expected 的对比通过** — 这些测试实际上验证了错误行为
- **PRINT USING 基于错误 expected 被移入 Implemented**（v75 的正确做法是移到 Not Implemented 标"待修" — 现在被"固化错误"包办了）
- 系统性风险：**未来修复了 USING/随机文件/数字格式后，expected 需全部手工纠正** — 同时期间任何改动都"通过"（无基线）

**根因:** .expected 由**生成时输出**直接保存（`--update` 模式），未经过 GW-BASIC 语义校验 — 生成 expected 时就把 bug 输出当正确了。

**修复建议:** 逐项人工核对 expected 与 GW-BASIC 语义（using/put_get/read_data 四文件）；修复实现后重新生成。

---

### 🟧 HIGH

| # | 描述 | 文件 |
|---|------|------|
| 1 | **随机文件数据回路缺陷** — MKI$(`\x00`) 截断 + GET 读回缓冲空 → ID$/NAME$ 空、CVI=0 | `src/gwsts.c`/`src/dskcom.c`/`src/bistrs.c` |

**详情:**

#### HIGH-1: PUT/GET 二进制数据损坏（最小复现验证）

```basic
' 临时验证（最小 PUT）
LSET ID$ = MKI$(7)   ' 期望 MKI$ = "\x07\x00"（int16 小端 2 字节）
LSET NAME$ = "TESTDATA"
PUT #1, 1
' 文件内容实测: b'\x07   TESTDATA...'  ← \x07 后应为 \x00，实际是空格！
```

**双重缺陷:**
1. **MKI$ 的 `\x00` 字节截断**（bistrs.c）— 字符串含 NUL 时按 C 字符串语义存储 → `\x07\x00` 变 `\x07` — **打包函数（MKI$/MKS$/CVI/CVS）二进制数据在字段中损坏**
2. **GET 读回缓冲为空** — test_put_get 输出 `CVI(ID$) = 0 且 NAME$ 空` — GET 未把文件内容填充到 FIELD 缓冲（或直接从空缓冲读）

**影响:** 随机文件记录（含二进制字段）读写全部失效 — 数据全 0/空。test_put_get/file_rand 的 expected "0×5" 正是该缺陷的固化。

**修复方向:** 打包函数返回/字符串存储需支持内嵌 NUL（长度语义）；GET 需将文件记录读入 field_buffer。

---

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **PRINT USING 前导 0 未修复却被移入 Implemented**（v75 曾正确移出标注待修 — 现被错误 expected 包办）| `run_bas_tests.sh` |
| 2 | **PRINT 数字分号分隔空格丢失**（read_data "102030" vs GW-BASIC "10 20 30" — 数字分隔行为）| `src/gwsts.c` |
| 3 | 6 个测试移入的依据是"与 expected 对比通过" — 其中 4 个 expected 固化错误（using×3 + put_get）| `run_bas_tests.sh` |

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | commit message 声称"字符串数组已由之前 gweval.c 修复" — 实际本提交 gweval.c +19 含有修复（表述模糊）|
| 2 | test_file_rand/test_put_get 无内容断言（依赖 expected 对比 — 而 expected 固化错误）|

---

## 三、架构评估

### 修复质量

| 修复 | 评价 |
|------|------|
| HIGH-1 FIELD 根因精确定位（赋值分支 '=' 误截获 OPEN LEN）| ✅ 高质量 — 排除关键字法正确 |
| HIGH-2 字符串数组（gweval 修复）| ✅ 实测 "Apple/Banana/..." 正确 |
| 6 测试移入 Implemented | ❌ 基于错误 expected（CRITICAL-1）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1085/1085 |
| run_bas_tests.sh "56/56" | ⚠️ 错误 expected 对比 — 假通过 |
| 随机文件数据回路 | ❌ MKI$ \x00 截断 + GET 空缓冲 |
| 字符串数组 | ✅ 修复 |
| PRINT USING | ❌ 前导 0（固化在 expected）|

**测试全面性评分: C-** — 基础设施对比逻辑正常但**期望值本身错误** — 测试通过不代表行为正确。

---

## 四、改进建议

### 短期 (当前 sprint) — 必须处理

1. **🟥 CRITICAL-1**: 逐项核对并修正 4 个错误 expected（using×3、put_get、read_data）为 GW-BASIC 正确输出；**expected 生成需人工语义校验**（而非程序直接保存）
2. **🟧 HIGH-1**: 修 MKI$ `\x00` 截断（长度语义字符串）+ GET 读回缓冲填充

### 中期 (下个 sprint)

3. **🟨 MEDIUM-1/2**: PRINT USING 前导 0 修复（移回 Not Implemented 或实现）；PRINT 数字分隔空格
4. 随机文件全链路修复后重新生成 put_get/file_rand expected

### 长期

5. expected 校验流程（生成后人工/自动比对参考 GW-BASIC 语义 — 防固化 bug）
6. 组件级测试（MKI$/CVI 往返、LSET/PUT/GET 环形验证）

---

## 五、总结

**总体评价**: 822e11e 的 FIELD 修复（HIGH-1）质量高（根因"赋值分支误截获 LEN=32 的 '='" 精准定位），字符串数组（HIGH-2）实测修复正确。**但引入严重问题**：4 个 .expected 文件**固化了错误输出**（PRINT USING 前导 0、put_get 数据全 0、read_data 紧凑数字）→ run_bas_tests.sh "56/56 通过"是错误 expected 的假通过。**随机文件数据回路实测损坏**（MKI$ `\x00` 截断 + GET 空缓冲 — 最小复现验证）。

**关键发现**:
- 🟥 CRITICAL: 1（expected 固化错误输出 — 测试基础设施反噬）
- 🟧 HIGH: 1（随机文件数据回路 — MKI$ \x00 + GET 空缓冲）
- 🟨 MEDIUM: 3（USING 误移入、数字空格、移入依据）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1085/1085 单元测试 + 零警告
- ✅ FIELD 执行级修复（Error line 20 消失）
- ✅ 字符串数组 "Apple/Banana/..." 完整
- ❌ put_get/file_rand "0×5"（数据回路坏 — 被固化）
- ❌ using "01234.56"（前导 0 — 被固化）
- ⚠️ read_data "102030"（数字空格 — 被固化）

**建议**: **本批不可合并** — 先修正错误 expected（CRITICAL-1），再修随机文件数据回路（HIGH-1）。承诺的"56/56"必须在 corrected expected 下重新验证。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-29
**审查版本**: v83 — FIELD 修复 + expected 固化错误输出（系统性）