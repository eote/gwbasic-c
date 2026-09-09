# GW-BASIC C Port - Code Review v99

**日期**: 2026-09-10
**审查范围**: commits `0eec19c` + `efc3f3e` + `00d05d5`（v98 MEDIUM 清理 + docs 入库）独立验证
**测试状态**: 单元测试 **1742/1742 通过**（706 注册，0 失败）✅ | .bas 集成 **65/67**（0 失败 + 2 预期跳过）✅
**编译状态**: 干净重建成功（exit 0）| 警告 **107 条**（生产 52 / 测试 55，连续 3 轮持平）
**工作区状态**: clean ✅（CODE_REVIEW.md v98 + PLAN_FRESH.md 首次入库）
**总体评分**: A- (优秀) — 删除与清理决策全部合理、eos 错误泄漏修复正确；但两处声称与实际不符（scnedt 断言、EOF LF），空壳测试仍未真正兑现，孤儿计数报告失实

---

## 一、概述

三个提交整体质量良好：`0eec19c` 修复 v98 MEDIUM-3（eos 错误状态泄漏）；`efc3f3e` 清理 6 个低价值孤儿（删除 2 个重复 + 改造 2 个空壳）；`00d05d5` 首次将 CODE_REVIEW.md 与 PLAN_FRESH.md 入库（工作区干净）。但逐一比对发现**提交信息与实际实现有 3 处偏差**（连续第 4 轮出现，模式稳定）。

### 验收表

| 提交 | 声称 | 实测 | 结论 |
|------|------|------|------|
| 0eec19c | eos 加 `gwdata_clear_error()` | ✅ diff 确认 1 行，正确修复 v98 MEDIUM-3 | **已修复** |
| 0eec19c | 补文件末尾 LF | ❌ **未兑现** — 4 个测试文件（gweval_helpers/gwdata/math/scnedt）末尾字节实测均为 `}`（0x7D），无一以 LF 结束 | 声称不实 |
| 0eec19c | 简化冗余注释（删 3 遍重复） | ❌ **未兑现** — diff 仅 1 行（clear_error），无任何注释变更 | 声称不实 |
| efc3f3e | 删除 string_heap_wrapper | ✅ 合理 — wrapper 仅转发 `string_heap()`，注册入口保留；`string_heap_impl` 经注册的 `string_heap` 间接执行，无覆盖损失 | **删除正确** |
| efc3f3e | 删除 test_math_debug | ✅ 合理 — 其断言（SIN(0)=0/COS(0)=1/SQR(16)=4）与 `test_math_expression_eval`（192-198 行）逐条重复 | **删除正确** |
| efc3f3e | scnedt inlin0 加"minimal assertions" | ❌ **未兑现** — 实际只加 2 行**注释**（`/* PINLIN should set up... */`），断言数仍为 0，仍是空壳 | 声称不实 |
| efc3f3e | Orphan 6→4 (remaining: gwlist_init, bimisc trap API) | ⚠️ 数量对但列项错 — 实测剩余 **4 个**：`string_heap_impl`、`gwlist_init`、`scnedt_inlin0_pinlin/inlin`；"bimisc trap API" **不是孤儿**（是 gwbasic.h 缺声明问题，见 MEDIUM） | 声称部分不实 |
| 00d05d5 | CODE_REVIEW v98 + PLAN_FRESH 入库 | ✅ 工作区 clean，两文件完整入库 | **完成** |

### 实测指标跟踪

| 指标 | v96 | v97 | v98 | v99 |
|------|-----|-----|-----|-----|
| 注册函数 | 686 | 701 | 706 | 706 |
| 断言（全过） | 1618 | 1729 | 1742 | 1742 |
| 孤儿 | 25 | 10 | 6 | **4** |
| .bas | 61/67 | 65/67 | 65/67 | 65/67 |
| 警告 | 109 | 107 | 107 | 107 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（连续第 2 轮 0 HIGH。）

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **空壳测试仍未兑现断言（3 轮）** — `test_scnedt_inlin0_pinlin/inlin` 仍只有注释 + `TEST_PASS`，无任何行为断言；提交 efc3f3e 声称"added assertions"实为注释。这两函数要么补真实断言（检查 inlin/pinlin 的缓冲/光标副作用），要么删除 | `tests/test_scnedt.c` | 102-113 |
| 2 | **文件末尾 LF 缺失（4 个文件，0eec19c 声称修复但未兑现）** — 实测末字节均为 `}`：`test_gweval_helpers.c`/`test_gwdata.c`/`test_math.c`/`test_scnedt.c`；POSIX 工具链/未来 diff 工具会误报差异 | `tests/test_*.c` | EOF |
| 3 | **提交声称-实际偏差模式（第 4 轮）** — 本轮 3 处（scnedt 断言、EOF LF、注释简化）+ 孤儿列表失实；连续 4 轮出现"声称修复 N 项，实际部分兑现"或"实现与描述不同"。建议提交前自检：`git diff --stat` 与实际改动逐项核对 | 提交信息 | — |
| 4 | **剩余 4 孤儿** — `string_heap_impl`（间接执行，可并入 `string_heap` 或注册，1 行）；`gwlist_init`（已声明未注册，被 `init_twice` 替代 — 删或注册）；`scnedt_inlin0_*`（见 MEDIUM-1） | `tests/` | — |
| 5 | **遗留未闭环（第 2-4 轮）** — 根 Makefile TEST_SRCS 死变量（含重复项）；gwbasic.h 缺 bimisc trap API 声明（test_bimisc.c 6 条隐式声明）；main() 注册块缩进混乱（16 空格块）；107 条警告超零警告门槛 | `Makefile`、`src/gwbasic.h`、`tests/test_runner.c` | — |

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | 多次 efc3f3e 提交删除操作后文件仍无末尾换行（本次删除的 gwdata/math 两文件结尾即为此前未结束的函数体 `}` 直接 EOF） | `tests/test_gwdata.c`、`tests/test_math.c` |
| 2 | 00d05d5 将 CODE_REVIEW 与 PLAN_FRESH 一并入库 — 建议后续将 CODE_REVIEW 变更与代码修复拆开提交，保持 review 文档与代码提交正交 | — |
| 3 | `gwdata_clear_error()` 在 eos 测试内使用 ✅；注意它只是清码，`on_error_active` 状态不受影响 — 建议该模式推广到所有产生错误码的测试 | `tests/test_gweval_helpers.c` |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 删除重复 wrapper（string_heap_wrapper / math_debug） | ✅ 正确去重：注册入口、断言内容均验证过无覆盖损失 |
| scnedt 空壳"改造"方式 | ⚠️ 注释代替断言 — 未达到"不再空壳"的声称效果 |
| eos 错误清理 | ✅ `gwdata_clear_error()` 位置正确（断言后、TEST_PASS 前） |
| docs 提交（CODE_REVIEW+PLAN_FRESH） | ✅ PLAN_FRESH 作为单一事实来源入库符合项目计划文档治理 |

### 测试覆盖分析（实测）

| 功能 | 状态 |
|------|------|
| gweval_synchr 全套（匹配/失配/EOS/空/运算符） | ✅ 5 路径 + 错误清理解耦 |
| 多维数组 / DIM 语句 | ✅ 恢复覆盖（v98 起） |
| 字符串堆（impl 间接） | ✅ 经 `string_heap` 入口运行 |
| 空壳（scnedt inlin0） | ❌ 无断言（MEDIUM-1） |

**测试全面性评分: A-** — 1742 断言全绿、.bas 65/67、孤儿 4 个均为"间接执行或无价值"级别；主要盲区已清零，剩余为空壳断言与常规门槛项。

---

## 四、改进建议

### 短期（当前 sprint）
1. 兑现 scnedt inlin0 断言：如 `TEST_ASSERT(scnedt_get_buf_len() == 0, ...)` 或检查 INIT 后光标/缓冲状态后注册，否则删除两函数（目标孤儿归零）。
2. 4 个文件补 EOF LF（PowerShell `Add-Content` 或单次 write 修复）；顺带兑现注释简化。
3. `string_heap_impl` 并入 `string_heap`（或注册），`gwlist_init` 删除或注册 — 消除 4 孤儿。

### 中期（下个 sprint）
4. **提交前自检流程**：`git diff --stat` 与提交信息逐项核对（连续 4 轮偏差的根治手段）。
5. 删除根 Makefile TEST_SRCS 死变量；gwbasic.h 补 bimisc trap API 声明；main() 注册块统一缩进。
6. 零警告冲刺（107 → ≤50）。

### 长期
7. "定义↔注册一致性"自动校验入 CI（第 3 轮建议，人工核对成本持续累积）。
8. run_test 包装内全局状态重置（错误码/变量表），从机制上消灭跨测试泄漏。

---

## 五、总结

**总体评价**: 质量收敛轮 — 删除决策全部正确、eos 泄漏修复到位、docs 首次入库（工作区干净）；测试/集成/警告三条线连续 3 轮稳定（1742 / 65/67 / 107）。
**关键发现**: 0 CRITICAL / 0 HIGH / 5 MEDIUM / 3 LOW。本轮主要扣分项为**声称-实际偏差连续第 4 轮出现**（scnedt 断言、EOF LF、注释简化 3 项声称仅 1 项兑现）与空壳测试遗留。
**建议**: 短期完成"孤儿归零 + EOF LF + 真断言"三项收尾；中期落实提交前自检与零警告冲刺。下轮关注：孤儿 0、警告 ≤50、声称-实际零偏差。