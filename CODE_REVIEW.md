# GW-BASIC C Port - Code Review v88

**日期**: 2026-09-01
**审查范围**: commits `85ea7ee` → `de6f047` → `8505968` (HEAD)
**测试状态**: 1086/1086 单元测试通过 ✅ | .bas 迁移测试全绿 ✅（test_resume 例外 ⚠️）
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B (良好) — v86/v87 全部修复兑现 + 测试迁移，但 test_resume 死循环测试和 177 万行垃圾 expected 入库

---

## 一、概述

2 个提交（v87 后）：

| 提交 | 内容 | 对应 |
|------|------|------|
| `de6f047` | **gwdata_handle_error() 提取**（gwmain 去重）+ 8 个 .bas 迁移 tests/bas/ + .expected | ✅ v86 MEDIUM-2/3 |
| `8505968` | ON ERROR 完整 + test_resume_line 新增 + 测试迁移收尾 | ✅ v87 MEDIUM-1 |

**验证（clean rebuild + 输出内容级）:**

| 测试 | 输出 | 评估 |
|------|------|------|
| 单元测试 | ✅ 1086/1086 | 零警告 |
| test_resume_next.bas | "After error (should not print)" — RESUME NEXT 跳过错误行 | ✅ |
| **test_resume_line.bas** | "After RESUME line - X is now 1" — RESUME 指定行跳转 | ✅ 新增变体正确 |
| test_resume.bas | ❌ **无限循环（5s 超时未退出）** | ⚠️ 见 HIGH-1 |
| test_error/onerr_simple/error_stmt/err2/div | ✅ ERR=11 全链路 | ✅ |
| 回归（using/read_data/put_get/string）| ✅ | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 |
|---|------|------|
| 1 | **test_resume.expected 提交了 177 万行（10.6 MB）死循环垃圾输出** — git 仓库污染 | `tests/bas/test_resume.expected` |
| 2 | **test_resume.bas 设计缺陷 — 无限死循环**（RESUME 重试无法修复的错误行）| `tests/bas/test_resume.bas` |

**详情:**

#### HIGH-1: 177 万行垃圾 expected 入库

```bash
$ python ... test_resume.expected
size: 10,674,176 bytes (10.6 MB), lines: 1,776,710
```

- `test_resume.bas`：`RESUME`（不带 NEXT）重试错误行 30（`X = 1 / 0`）— handler 设 `X = 1` 但 **1/0 永远除零失败** → 无限循环
- 开发者生成 expected 时跑死循环程序 → 输出被无限写入 test_resume.expected → **177 万行未检查就提交**
- **影响**: git 仓库被 10.6 MB 无效文件污染（含历史 — 每次 clone 都携带）；run_bas_tests.sh 不含 test_resume（幸免脚本运行），但文件本身是垃圾包袱
- **修复**: 删除 test_resume.bas/.expected（run_bas_tests.sh 已不含 — 确认设计意图：RESUME 重试语义由 test_resume_line/next 覆盖）

#### HIGH-2: test_resume.bas 死循环

```
30 X = 1 / 0      ← 错误源（永远失败）
...
120 RESUME        ← 重试 line 30 → 再除零 → handler → RESUME → ∞
```

- RESUME（无 NEXT/无 line）语义正确重试错误行 — 但该测试的"错误行"**不可修复** → 测试本身无法终止
- **真实 GW-BASIC 行为**: 无限循环（GW-BASIC 同样会 loop）— 解释器行为正确，**测试设计错误**
- **修复建议**: 删除 test_resume.bas；RESUME 重试用**可修复错误**（如文件读取重试、变量条件错误）

---

### 🟨 MEDIUM

| # | 描述 |
|---|------|
| 1 | commit message 声称 "64/64" 但 run_bas_tests.sh 含 test_resume 吗？实测**不含**（仅 resume_next/resume_line）→ 声称应注明 test_resume 未纳入/已删除 |
| 2 | 死循环测试无保护 — 未来若将 test_resume 加入脚本 → run_bas_tests.sh 永久挂起（无 timeout 机制）|

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | test_onerr_off.bas 移入 NOT_IMPLEMENTED（3 SKIP：DEF FN/ON GOTO/ON ERR OFF）— 合理标注 |
| 2 | expected 生成流程仍无输出大小/终止检查（177 万行教训 — 建议生成时设 timeout + 大小上限）|

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| gwdata_handle_error() 提取（gwmain 去重）| ✅ 正确 — 消除两处重复 ON ERROR 检查 |
| 8 个 .bas 迁移 tests/bas/ + .expected | ✅ v87 MEDIUM-1 兑现 |
| test_resume_line.bas（RESUME line 变体）| ✅ 新增正确 |
| test_onerr_off → NOT_IMPLEMENTED | ✅ 合理（设计性 FAIL）|
| test_resume.bas/.expected（死循环产物）| ❌ HIGH-1/2 — 未检查提交 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1086/1086 |
| RESUME NEXT | ✅ |
| RESUME line | ✅ 新增 |
| ERROR/ERR/除零 | ✅ |
| test_resume.expected 大小 | ❌ 10.6 MB / 177 万行（垃圾）|
| .bas 迁移 | ✅ 8/8 |

**测试全面性评分: B** — 功能验证充分；test_resume 死循环/垃圾 expected 为质量缺口。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟧 HIGH-1/2**: 删除 `test_resume.bas` + `test_resume.expected`（run_bas_tests.sh 已不含 — RESUME 语义已由 next/line 变体覆盖）
2. **🟨 MEDIUM-1**: commit message 注明 64/64 不含 test_resume（已删/未纳入）

### 中期 (下个 sprint)

3. run_bas_tests.sh 加**超时保护**（timeout 命令 — 防死循环测试挂起）
4. expected 生成工具加**输出大小/终止检查**（防 177 万行再次发生）
5. 路线: DEF FN → USING 格式集扩充

### 长期

6. pre-commit 检查：文件大小上限（防 >1MB 文本入库）、未跟踪文件审计

---

## 五、总结

**总体评价**: `de6f047` + `8505968` 兑现了 v86/v87 的全部修复（REsUME NEXT/LINE 正确、M.EDIUM-1/2/3 完成、8 个 .bas 迁移入 tests/bas/），实测迁移测试与回归全绿。**但存在 2 个 HIGH**：test_resume.bas 是**无效死循环测试**（RESUME 重试永远失败的 `1/0`），其生成的 **177 万行（10.6MB）垃圾 expected 被提交**污染 git 仓库 — 提交前未检查输出大小。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 2（177 万行垃圾 expected、死循环测试设计）
- 🟨 MEDIUM: 2（声称不含 test_resume、无超时保护）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1086/1086 单元测试 + 零警告
- ✅ RESUME NEXT/LINE、ERROR/ERR、除零全链路
- ✅ 回归全绿（using/read_data/put_get/string）
- ❌ test_resume.bas 死循环（5s 超时）
- ❌ test_resume.expected 10.6MB / 177 万行（垃圾入库）

**建议**: 删除 test_resume.bas/.expected（一行级）+ run_bas_tests.sh 加超时保护后可合并。路线: DEF FN → USING 格式集。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-09-01
**审查版本**: v88 — ON ERROR 完整 + 测试迁移 + 死循环测试/垃圾 expected