# GW-BASIC C Port - Code Review v86

**日期**: 2026-09-01
**审查范围**: commits `6e3413d` → `ddabca9` → `4058bcb` → `717550f` (HEAD) — 完整代码审查
**测试状态**: 1086/1086 单元测试通过 ✅ | .bas: ON ERROR 基本功能通过但 RESUME NEXT 损坏 ⚠️
**编译状态**: 无错误、零警告 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B- (良好但有关键缺陷) — ON ERROR 架构方向正确但 RESUME NEXT 语义损坏

---

## 一、概述

3 个提交（v85 后）：

| 提交 | 内容 |
|------|------|
| `ddabca9` | 删除 5 个误提交调试文件 ✅ v85 MEDIUM-1 清理 |
| `4058bcb` | .gitignore 增加调试文件模式 ✅ 防再犯 |
| `717550f` | **ON ERROR / RESUME / ERROR / ERR 变量 + 除零错误检测** — Phase 5 |

**717550f 代码审查（完整源码阅读 — 5 源文件 + 8 .bas）:**

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 位置 |
|---|------|------|------|
| 1 | **RESUME NEXT 语义损坏 — 不跳到错误行的下一行，而是跳到 RESUME 语句的下一行** | `src/gwmain.c` / `src/gwsts.c` | gwmain:262 / gwsts:860 |

**详情:**

#### HIGH-1: RESUME NEXT 损坏（error_line 在 handler 前被清除）

**根因链:**
```
错误发生 → gwdata_set_error(11, line=20, ...) → error_line=20
gwmain.c:262: gwdata_clear_error() → error_line=0 ← 清除了！
gwmain.c:263: gwdata_enter_error_handler()
gwmain.c:264: 跳转到 handler (line 100)
handler 执行 → RESUME NEXT
gwsts.c:860: err_line = gwdata_get_error_line() → 0 ← 已清除！
gwsts.c:863: gwlist_find_line(0) → NULL
gwsts.c:864: curr && curr->next → false → "No next line"
gwsts.c:870: result = SUCCESS (0) → 返回 0
gwmain.c: rc=0 → curr = curr->next → 跳到 RESUME 语句的下一行
```

**实测复现:**
```basic
10 ON ERROR GOTO 100
20 A = 1/0
30 PRINT "After error line"   ← 应执行但被跳过
40 END
100 PRINT "Handler"
110 RESUME NEXT
```
- **期望**: "Handler\nAfter error line"
- **实测**: "Handler"（仅 handler 输出 — "After error line" 从未执行）

**现有测试通过的原因:** test_on_error.bas 的 PRINT "After RESUME NEXT" 紧跟 RESUME 语句 → `curr->next` 碰巧是正确的行 → **假通过**。

**修复建议:**
```c
/* gwmain.c — 不要在 handler 前清除 error_line */
/* 当前（错误）*/
gwdata_clear_error();  /* 清除了 error_code + error_line + error_text_ptr */
gwdata_enter_error_handler();

/* 修复：只清除 error_code，保留 error_line/error_text_ptr 供 RESUME 使用 */
g_state->error_code = 0;  /* 或新增 gwdata_clear_error_code() 只清错误码 */
gwdata_enter_error_handler();
```
或：将 error_line/error_text_ptr 存储到单独的 resume_line/resume_text_ptr 字段，在 `gwdata_clear_error` 中不清除。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 位置 |
|---|------|------|------|
| 1 | **`gwdata_set_error` 误设 `on_error_active = true`** — 应仅由 ON ERROR GOTO 语句控制 | `src/gwdata.c` | 142 |
| 2 | 8 个根目录 .bas 测试未纳入 run_bas_tests.sh / TEST_CATEGORIES → "57/57"声称不含它们 | 根目录 | — |
| 3 | gwmain.c 两处错误处理块代码重复（表达式层 + 语句层各一份 ON ERROR 检查）| `src/gwmain.c` | 252-281 / 290-301 |

**详情:**

#### MEDIUM-1: set_error 误设 on_error_active

```c
/* gwdata.c:142 */
void gwdata_set_error(int code, uint16_t line, int text_ptr) {
    ...
    g_state->on_error_active = true;  /* ← 错误！应仅由 set_on_error_goto 控制 */
}
```

- **当前碰巧不影响行为**: `handler_line = gwdata_get_on_error_line()` 返回 0（被 clear_on_error 清除）→ `handler_line > 0` 失败 → 落入"无 handler"路径
- **但如果 handler_line 检查逻辑改变** → 可能重新启用已禁用的陷阱
- **修复**: 移除 `g_state->on_error_active = true;`（on_error_active 只由 set_on_error_goto/clear_on_error 控制）

#### MEDIUM-2: 根目录 .bas 未纳入

8 个 .bas 在根目录（test_debug/div/div2/err2/error_stmt/onerr_off/onerr_simple/resume_next）— run_bas_tests.sh 和 TEST_CATEGORIES.md 均不含 → "57/57"声称不含它们 → 覆盖缺口。

#### MEDIUM-3: gwmain 代码重复

两处 ON ERROR 检查（表达式层 gwdata_get_error + 语句层 rc<0）逻辑几乎相同 — 可提取公共函数。

---

### 🟩 LOW

| # | 描述 | 位置 |
|---|------|------|
| 1 | gwdata.c 末尾无换行（`\ No newline`）| gwdata.c EOF |
| 2 | `error_text_ptr` 存储 raw 指针 cast 到 int（64 位截断 — 与 v54 指针截断同模式）| gweval.c:283 |
| 3 | gweval.c parse_factor 的 ERR 变量检查缩进异常（12 空格 vs 8 空格）| gweval.c:288-294 |
| 4 | RESUME（不带 NEXT）同样受 HIGH-1 影响 — error_line=0 → 跳到行 0 | gwsts.c:873 |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 运行时状态集中在 g_state（on_error_active/line/error_code/error_line/in_handler）| ✅ 方向正确 — 对齐 ASM 字段 |
| ERR 变量自动填充 + 持久化 | ✅ GW-BASIC 语义 |
| 除零 → set_error(11) | ✅ 正确错误码 |
| RESUME 返回行号给 main 跳转 | ✅ 与 GOTO 对称 |
| **clear_error 在 handler 前调用** | ❌ HIGH-1 — 清除了 RESUME 需要的 error_line |
| **set_error 设 on_error_active** | ⚠️ MEDIUM-1 — 语义错误（碰巧不影响行为）|
| ON ERROR GOTO 排除关键字 | ✅ 继承 v82 修复 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1086/1086 |
| ON ERROR GOTO / GOTO 0 / ERROR / ERR | ✅ 实测通过 |
| **RESUME NEXT（错误行与 handler 有间距）** | ❌ **HIGH-1 — "After error line" 未执行** |
| 除零 → ERR=11 | ✅ |
| 回归（using/read_data/put_get/string/cond/multi_array）| ✅ 全绿 |
| 根目录 .bas 纳入 run_bas_tests.sh | ❌ 0/8 |

**测试全面性评分: B-** — ON ERROR 基本功能验证但 RESUME NEXT 语义损坏未被现有测试捕获（假通过）。

---

## 四、改进建议

### 短期 (当前 sprint) — 必须修复

1. **🟧 HIGH-1**: 修复 RESUME NEXT — `gwdata_clear_error()` 不应在 handler 前清除 `error_line`/`error_text_ptr`（只清 `error_code`）；或新增 `gwdata_clear_error_code()` 函数
2. **🟨 MEDIUM-1**: 移除 `gwdata_set_error` 中的 `on_error_active = true`
3. 补充 RESUME NEXT 间距测试（错误行与 handler 分离 — 暴露 HIGH-1）

### 中期 (下个 sprint)

4. **🟨 MEDIUM-2**: 8 个根目录 .bas 移入 tests/bas/ + 纳入脚本
5. **🟨 MEDIUM-3**: gwmain 提取公共错误处理函数
6. 路线: DEF FN → USING 格式集扩充

### 长期

7. 其他运行时错误码扩展（溢出/下标越界/文件不存在 → GW-BASIC 错误码）
8. RESUME 嵌套深度测试

---

## 五、总结

**总体评价**: `717550f` 实现 ON ERROR/RESUME/ERROR/ERR 的架构方向正确（g_state 集中状态 + gwmain 主循环检测 + ERR 自动填充 + handler 跳转），ON ERROR GOTO / GOTO 0 / ERROR / ERR / 除零检测均实测通过。**但 RESUME NEXT 语义损坏**（HIGH-1）— `gwdata_clear_error()` 在 handler 执行前清除了 `error_line`，导致 RESUME NEXT 读到 0 → 跳到 RESUME 语句的下一行而非错误行的下一行。现有测试假通过（PRINT 紧跟 RESUME 巧合掩盖）。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 1（**RESUME NEXT 损坏** — error_line 在 handler 前被清除）
- 🟨 MEDIUM: 3（set_error 误设 on_error_active、根目录 .bas 未纳入、gwmain 代码重复）
- 🟩 LOW: 4

**编译与测试**:
- ✅ 1086/1086 单元测试 + 零警告
- ✅ ON ERROR GOTO / GOTO 0 / ERROR / ERR / 除零 → 全部通过
- ❌ **RESUME NEXT 间距测试**（错误行 20 + handler 100 → "After error line" 未执行）
- ✅ 回归全绿

**建议**: 修复 HIGH-1（`clear_error` 不清 `error_line` — 一行核心改动）后补充间距测试 → 可合并。下批：DEF FN → USING 格式集。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-09-01
**审查版本**: v86 — ON ERROR / RESUME / ERROR 完整代码审查（RESUME NEXT 损坏）