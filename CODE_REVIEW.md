# GW-BASIC C Port - Code Review v82

**日期**: 2026-08-27
**审查范围**: commits `44afbde` → `d5b467a` → `35b9f20` → `b3ac158` (HEAD)
**测试状态**: 1085/1085 单元测试通过 ✅ | .bas 部分验证 ⚠️
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: C+ (及格) — READ/DATA 数值路径成功；随机文件 I/O（FIELD）实测失败 + 字符串 DATA 损坏，commit 声称 50/50 不实

---

## 一、概述

三个提交（v81 后）：

| 提交 | 内容 | 验证 |
|------|------|------|
| `d5b467a` | test_math.bas 补换行 | ✅ v81 LOW-1 修复 |
| `35b9f20` | **READ/DATA/RESTORE**（bimisc.c +133）| ⚠️ 数值✅ 字符串损坏 |
| `b3ac158` | **FIELD/LSET/RSET/PUT/GET + MKI$/MKS$/CVI/CVS + OPEN LEN**（dskcom +178）| ❌ FIELD 实测失败 |

**实测验证（clean rebuild）:**

| 测试 | 结果 | 说明 |
|------|------|------|
| 单元测试 | ✅ 1085/1085 | 零警告 |
| test_read_data.bas | ✅ "First read: 10 20 30 / After RESTORE: 10 20 30" | READ/DATA/RESTORE 数值正常 |
| **test_string_arrays.bas** | ⚠️ **"1ARRA\x03"**（期望 "1Apple"）| **字符串 DATA 值损坏**（HIGH）|
| **test_put_get.bas** | ❌ **"Error at line 20"** exit -1 | **FIELD 语句失败**（HIGH）|
| **test_file_rand.bas** | ❌ **"Error at line 20"** exit -1 | 同 FIELD 失败 |
| test_file_seq/line_input/string/math | ✅ 保持 | 回归正常 |

**⚠️ commit 声称与实际不符**：b3ac158 message "50/50 Implemented PASS" — 但随机文件测试（put_get/file_rand 在 Implemented 的 File I/O - Random access）实测 **失败**。

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 位置 |
|---|------|------|------|
| 1 | **FIELD 语句执行报错** — 随机文件 I/O 全链路（FIELD→LSET→PUT→GET）无法工作 | `src/gwsts.c`/`src/dskcom.c` | gwsts_handle_field / dskcom_field |
| 2 | **字符串数组 READ/DATA 值损坏** — `DATA "Apple"` 读回 `"ARRA\x03"`（含控制字符）| `src/bimisc.c` | parse_data_value / read 赋值 |

**详情:**

#### HIGH-1: FIELD 语句报错（随机文件 I/O 阻塞）

```basic
' test_put_get.bas
10 OPEN "RNDTEST.DAT" FOR RANDOM AS #1 LEN = 32
20 FIELD #1, 4 AS ID$, 28 AS NAME$   ← Error at line 20
```

- `gwsts_handle_field`: filenum 解析（`#1` → 1）看似正确；`dskcom_field(1, "4 AS ID$, 28 AS NAME$")` 报错
- **影响**：LSET/RSET/PUT/GET（依赖 `fd->field_buffer`）全部无法到达 — b3ac158 的随机文件功能实际不可用
- commit 声称 "50/50 PASS" 与实测不符
- 根因方向：`dskcom_field` 解析 `width AS var$` 失败，或 OPEN RANDOM 的 mode/LEN 未正确设置（`fd->mode == FM_RANDOM` 检查）

#### HIGH-2: 字符串数组 DATA 值损坏

```basic
' test_string_arrays.bas: DATA "Apple","Banana","Cherry","Date","Elderberry"
' READ S$(I) 后 PRINT I; S$(I) → 期望 "1Apple"，实测 "1ARRA\x03"
```

- `parse_data_value` 对引号字符串：`memcpy(value, start, len)` — **len 包含两端引号**（`"Apple"` 7 字节），复制后未剥引号；且实测值 "ARRA\x03" 与 `"Apple"` 不符 — 说明**字符串值在 READ→变量存储链路存在字节偏移/长度前缀问题**（GW-BASIC 字符串堆 len+data 布局）
- 影响：字符串 DATA（含字符串数组）全部损坏

---

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **随机文件功能零覆盖** — FIELD 失败使 PUT/GET/LSET/RSET 测试全跳过，b3ac158 声称的实现未经真实路径验证 | `tests/bas/` |
| 2 | commit message 声称 "50/50 PASS" 与实际（2 个随机文件测试失败）不符 | — |
| 3 | test_string_arrays.bas 无断言（输出垃圾值 exit 0 — 又是只查 exit code 盲区）| `tests/bas/` |

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | CODE_REVIEW.md 在 b3ac158 有 287 行变更（-200）— 需确认非行尾符/误替换（我 v81 文档 vs 库内）| `CODE_REVIEW.md` |
| 2 | gwsts_handle_field 的 field_spec 用 strdup_trim 但 dskcom_field 需二次解析逗号 — 结构待验证 | `src/gwsts.c` |

---

## 三、架构评估

### 文件组织

| 提交 | 文件 | 变更 |
|------|------|------|
| 35b9f20 | bimisc.c +133 / gwsts.c +75 / gwmain.c +3 / gwbasic.h +8 | READ/DATA/RESTORE |
| b3ac158 | dskcom.c +178 / gwsts.c +243 / bistrs.c +53 / gweval.c +43 | FIELD/PUT/GET + 打包函数 |

### 设计决策评价

| 决策 | 评价 |
|------|------|
| READ/DATA 用 data_ptr/data_index 游标 + RESTORE | ✅ 方向正确（数值路径实测通过）|
| parse_data_value 逗号/引号切分 | ⚠️ 引号未剥（HIGH-2 根因候选）|
| FIELD 用 dskcom_field 解析 spec | ❌ 实测失败（HIGH-1）|
| 打包函数 MKI$/CVI/... 集中在 bistrs.c | ✅ 模块化正确 |
| OPEN LEN → record_length | ⚠️ 需验证（FIELD 依赖）|

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| READ/DATA 数值 | ✅ test_read_data（PASS）|
| READ/DATA 字符串 | ❌ 损坏（HIGH-2）|
| RESTORE | ✅ |
| FIELD | ❌ 失败（HIGH-1）|
| LSET/RSET/PUT/GET | ❌ 因 FIELD 失败未验证 |
| MKI$/CVI/... | ❌ 未验证（依赖随机文件）|
| 字符串数组 | ❌ 损坏 |

**测试全面性评分: C** — 两个新功能类（随机文件、字符串数组）实际不可用但 commit 声称全绿。

---

## 四、改进建议

### 短期 (当前 sprint) — 修复

1. **🟧 HIGH-1**: 定位 dskcom_field 报错（解析 `width AS var$` 或 OPEN RANDOM mode）— FIELD 通过后才能测 PUT/GET
2. **🟧 HIGH-2**: 修 parse_data_value 剥引号 + 字符串存储链路（READ→S$ 赋值字节偏移）
3. 修复后补字符串数组断言（test_string_arrays 加 PASS/FAIL）
4. 修正 commit message 声称（50/50 不实）

### 中期 (下个 sprint)

5. 随机文件全链路测试（FIELD→LSET→PUT→GET→CVI）修复后验证
6. 路线剩余: PRINT USING（前导 0）→ ON ERROR/GOTO

### 长期

7. .bas 断言推广（字符串/随机文件测试加断言 — 输出级）
8. commit message 与实测结果核对（多次声称不符的教训）

---

## 五、总结

**总体评价**: 35b9f20 的 READ/DATA/RESTORE **数值路径成功**（test_read_data 实测通过）；但 b3ac158 的**随机文件 I/O（FIELD）实测失败**（"Error at line 20"），PUT/GET/LSET/RSET 全链路因 FIELD 阻塞而不可用，且 **commit 声称 "50/50 PASS" 与实测不符**（2 个随机文件测试失败）。另有 **字符串数组 DATA 值损坏**（"Apple"→"ARRA\x03"，含控制符）。两个新功能类（随机文件、字符串数组）实际不可用。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 2（FIELD 失败、字符串 DATA 损坏）
- 🟨 MEDIUM: 3（随机文件零覆盖、声称不符、字符串数组无断言）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1085/1085 单元测试 + 零警告
- ✅ READ/DATA/RESTORE 数值（test_read_data PASS）
- ❌ test_put_get / test_file_rand（FIELD "Error at line 20"）
- ❌ test_string_arrays（"ARRA\x03" — 字符串损坏）
- ✅ 文件顺序/行输入/字符串函数/数学保持

**建议**: 本批**不应视为完全可合并** — 修复 HIGH-1（FIELD）和 HIGH-2（字符串 DATA）后再验证随机文件全链路。commit message 声称需与实际对齐。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-27
**审查版本**: v82 — READ/DATA + 随机文件 I/O（FIELD 失败 + 字符串损坏）