# GW-BASIC C Port - Code Review v157

**日期**: 2026-10-29
**审查范围**: commit `e92c91e`（fix: v156 MEDIUM-1 — SCREEN 0 图形 gate 补 `gwdata_set_error(5, line)`，6 处 gate + 单测 + 集成 `test_gfx_screen0_resume`）→ 工作区
**测试状态**: 单元测试 **3334/3334 通过**（0 失败，0 警告）✅ | .bas **84/84 全量直接运行**（输出与 expected 一致）✅ | 注册/定义 **893=893**（0 孤儿）✅
**工作区状态**: clean（仅 `.reposman` 外部标记）
**总体评分**: A- — v156 MEDIUM-1 修复确认（6 处 gate 的 ERR=5 + RESUME NEXT 全闭环，probe 实证）；无阻塞问题，6 观察项

---

## 一、概述

本次审查 v156 MEDIUM-1 修复提交 `e92c91e`（6 文件，+125/-6）。

### v156 MEDIUM-1 修复确认（全部验证通过）

| 验证层 | 内容 | 结果 |
|--------|------|------|
| 代码 | **6 处 gate**（GET/PUT 入口 + PSET/PRESET/LINE/CIRCLE/PAINT）统一 `gwdata_set_error(5, current_line, 0)` 后 `result = ERROR` | ✅ 与 ASM FCERR(Illegal function call=5) 一致 |
| 单测 | `test_gwsts_gfx2_screen0_resume_next`（PSET：rc≠0 + error==5）、`test_gwsts_gfx2_screen0_err_line`（CIRCLE：error==5） | ✅ 2 个新测试通过 |
| 集成 | `test_gfx_screen0_resume.bas`：`ERR= 5` → `PSET-FAIL` → `ERR= 5` → `CIRCLE-FAIL` → `GATE-DONE` | ✅ 与 expected 一致 |
| probe（6 语句逐一） | PSET/LINE/CIRCLE/PAINT/GET/PUT 全部 `ERR= 5` → RESUME NEXT → 各自 `*-GATED` → `GATE-DONE` | ✅ **全 6 处闭环**（v156 时仅 `ERR AT 0` 后终止） |
| 回归 | `test_gfx_screen0`（S0-BLOCKED/DONE） | ✅ |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3334/3334 PASS**（3331 → 3334，+3 断言） |
| 编译警告 | ✅ **0 条** |
| **.bas 全量（84 个，分 6 批直接运行）** | ✅ **84/84 输出与 expected 一致**（含新增 screen0_resume；terminating 用例符合预期） |
| 注册/定义一致性 | ✅ **893 = 893**（0 孤儿） |
| gate probe（6 语句） | ✅ ERR=5 + RESUME NEXT 全部恢复 |
| 集成 + 回归 | ✅ screen0_resume / screen0 均一致 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无（v156 MEDIUM-1 已修复确认）。

### 🟩 LOW

无（阻塞）。观察项：

1. **集成仅覆盖 6 处 gate 中的 2 处**（PSET/CIRCLE）；LINE/PAINT/GET/PUT 的 `set_error(5)` 仅在代码层一致，无对应单测/集成。建议补 4 处（各自 1 条错误行 + RESUME NEXT）。
2. **`test_gwsts_gfx2_screen0_err_line` 结束未 `gwdata_clear_error()`**（与 resume_next 测试不一致）— 测试卫生：错误码 5 残留，后续测试若依赖 error 状态可能受影响。建议统一 teardown。
3. **gate 叠加错误时错误码覆盖**：SCREEN 0 下坐标解析失败（result=ERROR，可能已设码）→ 外层 gate 再设 error=5 覆盖；边缘（mode 0 优先合理，但与"首个错误"语义存差异）。
4. **POINT 在 SCREEN 0 未 gate**（返回 -1，v156 观察）— 待 ASM 对照。
5. **obs-4 交互 RUN 仍无自动化测试**（v156 观察；dev 手动实证依赖 stdin 行间 sleep）。
6. 其余开放：filespec string heap 消耗（v152）、PUT 容量校验 / CIRCLE radius（v145）。

---

## 三、架构评估

### 修复质量（e92c91e）

| 决策 | 评价 |
|------|------|
| 6 处 gate 统一 `set_error(5, current_line)` | ✅ 正确 — ERR/ERL 与 RESUME NEXT 闭环（probe 6/6） |
| 行号取 `g_state->current_line`（执行前由 gwmain 设置） | ✅ 正确（.bas 与 RUN 统一执行器路径） |
| 单测（error==5 断言）+ 集成（ERR 行 + FAIL 行 + GATE-DONE） | ✅ 质量好；⚠️ 仅 2/6 gate 被测试覆盖（观察项 1） |
| `run_bas_tests.sh` 收录 + DEVELOPMENT_PROGRESS 更新 | ✅ 流程完整 |
| 测试 teardown 一致性 | ⚠️ err_line 测试缺 clear_error（观察项 2） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| PSET / CIRCLE gate 错误码 + RESUME NEXT | ✅ 单测 + 集成 + probe |
| LINE / PAINT / GET / PUT gate 错误码 | ⚠️ 仅 probe（仓库测试未覆盖，观察项 1） |
| gate 正常路径（SCREEN 1） | ✅ 全量既有测试 |
| 全量回归 | ✅ 84/84 |
| 注册一致性 | ✅ 893=893 |

---

## 四、改进建议

### 短期（可选）
1. 补 LINE/PAINT/GET/PUT 的 gate 错误码测试（观察项 1）
2. 统一测试 teardown（`clear_error`）（观察项 2）
3. POINT 在 SCREEN 0 的 ASM 对照（观察项 4）

### 中期
4. 交互 RUN main 级自动化 smoke（观察项 5）
5. v145 图形剩余（PUT 容量校验、CIRCLE radius）
6. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
7. ASAN 构建
8. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v156 MEDIUM-1 精准修复 — 6 处 SCREEN 0 gate 统一设置错误码 5 与行号，`ON ERROR + RESUME NEXT` 全语句闭环（probe：PSET/LINE/CIRCLE/PAINT/GET/PUT 均 `ERR=5` → 恢复 → `GATE-DONE`）；新增单测 + 集成；单测 3334/3334、84/84 全量、0 警告、893=893。剩余为测试覆盖均匀性（2/6 gate）与既有观察项。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**（6 观察项）。
**建议**: 补齐剩余 4 处 gate 测试与 teardown 一致性；POINT/交互 RUN 等观察项随批次处理。

---

## 六、指标追踪

| 指标 | v156 | v157 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3331 | **3334** | ✅ +3（+2 测试函数） |
| 定义数 / 注册数 | 891 / 891 | **893 / 893** | +2 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 83/83 | **84/84 全量直接运行一致** | ✅ +1（screen0_resume） |
| SCREEN 0 gate ERR/ERL/RESUME | ❌ MEDIUM-1 | ✅ **修复**（6/6 probe 闭环） | 关闭 |
| 观察项 | 5 | **6**（新增 gate 覆盖均匀性/teardown） | 更新 |
| M2 进度 | 2/3 + 观察项批次 | **2/3 + 观察项批次修复** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3334 → 84 个 .bas 分 6 批直接运行（expected 对比）→ 6 语句 gate probe + 集成/回归 + 注册核对
- 全程未使用脚本（分批命令链 + 内置工具）；Reviewer 角色约束保持（仅更新 CODE_REVIEW.md）
- **M2 状态**: 观察项批次闭环；剩余观察项（POINT/交互 RUN/图形边角）待后续