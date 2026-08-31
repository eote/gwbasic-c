# GW-BASIC C Port - Code Review v87

**日期**: 2026-09-01
**审查范围**: commit `717550f` → `85ea7ee` — fix: RESUME NEXT 修复 - HIGH-1 (v86)
**测试状态**: 1086/1086 单元测试通过 ✅ | .bas 全链路验证 ✅
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: A (优秀) — v86 HIGH-1/MEDIUM-1 精准修复，复现场景验证通过

---

## 一、概述

`85ea7ee` 精准修复 v86 报告的 HIGH-1（RESUME NEXT 损坏）和 MEDIUM-1（on_error_active 误设）：

| v86 问题 | 状态 | 修复方式 |
|----------|------|----------|
| 🟧 HIGH-1: RESUME NEXT 损坏（error_line 在 handler 前被清除）| ✅ **已修复** | `gwdata_clear_error()` 仅清除 error_code，**保留 error_line/error_text_ptr** 供 RESUME 用 |
| 🟨 MEDIUM-1: set_error 误设 on_error_active | ✅ **已修复** | 移除 `g_state->on_error_active = true`（注释说明仅由 ON ERROR GOTO 控制）|
| 扩展 | ✅ | test_error.bas 重构 + expected 更新 |

**修复核心（gwdata.c）:**
```c
/* 修复前：清除全部（错误）*/
void gwdata_clear_error(void) {
    g_state->error_code = 0;
    g_state->error_line = 0;        /* ← 清除了 RESUME 所需 */
    g_state->error_text_ptr = 0;    /* ← 清除了 RESUME 所需 */
}

/* 修复后：仅清除 error_code，保留位置供 RESUME */
/* Clear only the error code, preserving error_line/error_text_ptr for RESUME */
void gwdata_clear_error(void) {
    g_state->error_code = 0;
    /* Preserve error_line and error_text_ptr for RESUME NEXT/RESUME line */
}
```

**实测验证（关键 — v86 复现场景重现）:**

| 场景 | v86 | v87 | 结论 |
|------|-----|-----|------|
| **RESUME NEXT 间距**（错误行 20 + handler 100）| ❌ 仅 "Handler"（"After error line" 跳过）| ✅ **"Handler\nAfter error line\n"** | **HIGH-1 修复确认** |
| test_error.bas | — | "After error line - executed via RESUME NEXT" | ✅ |
| test_on_error.bas | ✅ | "After error (should not print)"… + RESUME 行为 | ✅ |
| test_onerr_off.bas | -1（设计性）| -1（保持 — ON ERROR OFF 后未处理错误）| ✅ |
| test_onerr_simple / resume_next / error_stmt / err2 | ✅ | ✅ | 保持 |
| 除零 → ERR=11 → RESUME NEXT | ✅ | ✅ | 保持 |
| 回归（using/read_data）| ✅ | ✅ | 保持 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（v86 HIGH-1 已修复并复测确认）

### 🟨 MEDIUM

| # | 描述 | 状态 |
|---|------|------|
| 1 | **8 个根目录 .bas 测试未纳入**（test_debug/div/div2/err2/error_stmt/onerr_off/onerr_simple/resume_next — 在根目录非 tests/bas/，run_bas_tests.sh/TEST_CATEGORIES 不含）| ❌ v86 MEDIUM-2 残留 |
| 2 | gwmain.c 两处错误处理块代码重复（表达式层 + 语句层）| ❌ v86 MEDIUM-3 残留 |

---

### 🟩 LOW

| # | 描述 | 状态 |
|---|------|------|
| 1 | RESUME（不带 NEXT）/RESUME line 形式的间距测试未覆盖（v86 只验证了 NEXT）| ⚠️ 建议补充 |
| 2 | 其他运行时错误码（溢出/下标越界）待扩展 | 残留 |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| clear_error 仅清 error_code（保留 error_line/text_ptr）| ✅ **精准修复** — 符合 GW-BASIC 语义（ERL/ERRLIN 在 RESUME 前保留）|
| 移除 set_error 中 on_error_active 赋值 | ✅ 语义修正 — 陷阱状态仅由 ON ERROR 语句控制 |
| test_error.bas 重构（验证 RESUME NEXT 到错误行下一行）| ✅ 测试增强 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1086/1086 |
| RESUME NEXT 间距（复现场景）| ✅ **已修复**（"Handler\nAfter error line"）|
| ON ERROR 全链路 | ✅ |
| 回归 | ✅ 全绿 |
| RESUME / RESUME line 变体 | ⚠️ 未专项覆盖（LOW-1）|

**测试全面性评分: A** — 复现场景验证修复 + 全绿。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟨 MEDIUM-1**: 8 个根目录 .bas 移入 tests/bas/ + 纳入脚本/分类（v86 残留）
2. **🟩 LOW-1**: 补 RESUME（无 NEXT）和 RESUME line 形式的间距测试

### 中期 (下个 sprint)

3. **🟨 MEDIUM-2**: gwmain 错误处理代码去重
4. 路线: DEF FN → USING 格式集扩充 → 更多运行时错误码

### 长期

5. pre-commit 工具（.bas 位置、调试文件、expected 校验）

---

## 五、总结

**总体评价**: `85ea7ee` 是精准的修复提交 — v86 的 HIGH-1（RESUME NEXT）和 MEDIUM-1（on_error_active）**根因修复**（不是打补丁）：`clear_error` 保留 error_line/text_ptr 供 RESUME 使用、移除 set_error 对陷阱状态的误设。**复现场景实测确认**（错误行 20 + handler 100 → RESUME NEXT 正确回到 line 30）。1086/1086 + 全绿。残留 v86 MEDIUM-2/3（根目录 .bas 纳入、代码去重）。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 0
- 🟨 MEDIUM: 2（残留 — 根目录 .bas、代码重复）
- 🟩 LOW: 2

**编译与测试**:
- ✅ 1086/1086 单元测试 + 零警告
- ✅ RESUME NEXT 间距复现 → "Handler\nAfter error line"（v86 仅 "Handler"）
- ✅ ON ERROR 全链路保持
- ✅ 回归全绿

**建议**: 本批可合并。下批：迁移 8 个根目录 .bas 入 tests/bas/（一行级），继续 DEF FN/USING 路线。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-09-01
**审查版本**: v87 — fix: RESUME NEXT 修复 - HIGH-1 (v86)