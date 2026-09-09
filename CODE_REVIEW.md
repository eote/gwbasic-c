# GW-BASIC C Port - Code Review v100

**日期**: 2026-09-10
**审查范围**: commits `d22de54` + `891a03d`（v99 全部 MEDIUM + 部分 LOW 修复；docs 入库 v99）独立验证
**测试状态**: 单元测试 **1745/1745 通过**（709 注册，**0 孤儿**，0 失败）✅ | .bas 集成 **65/67**（0 失败 + 2 预期跳过）✅
**编译状态**: 干净重建成功（exit 0）| 警告 **107 条**（生产 52 / 测试 55，连续 4 轮持平）
**工作区状态**: clean ✅
**总体评分**: A (优秀) — 孤儿归零里程碑达成、EOF LF/断言/注释简化全部兑现、声称-实际偏差收敛至 1 处轻微数字不一致；剩余为缩进治理与零警告冲刺两个长期项

---

## 一、概述

`d22de54` 覆盖 v99 报告的全部 MEDIUM（1/2/3/4/5）与 LOW（1/3）。经逐项实测验收：**5 项 MEDIUM 中 4 项完全兑现、1 项如实标注延期**；孤儿计数 4→0（自 v95 的 42 个累计清零）。`891a03d` 入库 v99。

### 验收表

| v99 项 | 声称 | 实测结果 | 结论 |
|--------|------|----------|------|
| MEDIUM-1 scnedt 真断言 | 加 buf_len 断言 | ✅ 2 个断言实际运行并 PASS（`RUN: scnedt_inlin0_pinlin/inlin ... PASS: Buffer length is 0 after init`）；⚠️ 语义漂移见 MEDIUM-2 | **已修复** |
| MEDIUM-2 EOF LF | 4 文件补 LF | ✅ 实测 4 文件末字节均为 LF（0x0A） | **已修复** |
| MEDIUM-2 注释简化 | 删 3 遍重复 | ✅ diff 确认：函数头 3 行→2 行、函数体内 2 条冗余注释删除 | **已修复** |
| MEDIUM-3 声称-实际偏差 | 提交前自检 | ✅ **显著收敛** — 本轮 8 项声称中 7 项与实测完全一致，仅 1 处数字偏差（`710 registered` vs 实测 709，+1）；"107 warnings unchanged (scope for next sprint)" 如实标注延期 ✅ | **改善** |
| MEDIUM-4 孤儿清零 | 4→0 | ✅ 实测 registered 709 / definitions 709 / **ORPHANS 0**（`string_heap_impl` static 化 + scnedt inlin0 ×2 注册 + gwlist_init 注册） | **已修复** |
| MEDIUM-5 遗留项 | 仅声明不动（下轮） | ⚠️ 107 警告 / 死变量 / bimisc 声明 / 缩进均未处理 — 与声明一致，视为延期而非变更缺失 | **如实延期** |
| LOW-1/3 EOF LF | 附带修复 | ✅ 同 MEDIUM-2 兑现 | **已修复** |

### 指标里程碑（v95 → v100）

| 指标 | v95 | v96 | v97 | v98 | v99 | v100 |
|------|-----|-----|-----|-----|-----|------|
| 注册函数 | 669 | 686 | 701 | 706 | 706 | **709** |
| 断言（全过） | 1143 | 1618 | 1729 | 1742 | 1742 | **1745** |
| 孤儿 | 42 | 25 | 10 | 6 | 4 | **0** 🎉 |
| .bas | 61/67 | 61/67 | 65/67 | 65/67 | 65/67 | 65/67 |
| 警告 | 141 | 109 | 107 | 107 | 107 | 107 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（连续第 3 轮 0 HIGH。）

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **scnedt inlin0 测试语义漂移** — 补断言时**移除了对被测函数的调用**（原 `scnedt_pinlin()/scnedt_inlin()` 被删除），函数退化为"init + buf_len==0 断言"，测试名 `inlin0_pinlin/inlin` 与实际内容（仅测 init 状态）不符；且与已注册的 `test_scnedt_init` 覆盖重叠。建议：改名 `scnedt_init_state_*` 或恢复 `pinlin/inlin` 的实际断言（若能安全调用） | `tests/test_scnedt.c` | 102-116 |
| 2 | **main() 注册块缩进严重失控（第 5 轮，本次恶化）** — d22de54 将 SCNEDT 块 12 行从 32 空格重排为 **64 空格**，gwlist 块 4→8 空格；main() 内缩进现已出现 4/8/16/32/64 五种层级，任何新增注册都无规则可循。建议一次性 clang-format 或手动归一（4 空格基准） | `tests/test_runner.c` | 1193-1206, 1375-1378 |
| 3 | **107 条警告（零警告门槛，延期项）** — 生产 52：oem.c 31（未用函数/参数、恒真比较×2）、gwsts 7、ibmres 2；测试 55：next86 18 + call86 8 + gweval_helpers 7（const 指针）、bimisc 6（隐式声明）、framework 6 | 多处 | — |
| 4 | **根 Makefile TEST_SRCS 死变量（第 5 轮）/ gwbasic.h 缺 bimisc trap API 声明（第 4 轮）** — 均未处理；死变量仍含 `test_gweval_helpers.c` 重复 ×2 | `Makefile`、`src/gwbasic.h` | 52-53 |

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | 声称数字偏差残留：`1745/1745 unit tests pass (710 registered)` — 实测注册 709（710 可能混入 static 化的 impl 计数）；孤儿清零结论正确，仅数字口径 | 提交信息 |
| 2 | `string_heap_impl` static 化后仍由 `string_heap` wrapper 转发调用 — 建议直接合并入 `string_heap`（减少一层间接） | `tests/test_gwdata.c` 97-127 |
| 3 | docs 提交与代码修复分离模式已建立 ✅（00d05d5 / 891a03d）— 建议保持 | — |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| static 化消除孤儿（string_heap_impl） | ✅ 合理：单编译单元内部辅助函数，static 符合封装语义 |
| scnedt 空壳"真断言"方案 | ⚠️ 断言合格但丢了被测调用 — 从"空壳"变成了"测 init"，测试意图漂移（MEDIUM-1） |
| 注册 gwlist_init | ✅ 合法（was declared-but-unregistered），断言"Program list is empty after init"实际运行通过 |
| 声称-实际自检 | ✅ 大幅改善（8 项 7 项一致）；本次已把"未做项"明确标注延期而非隐瞒 |

### 测试覆盖分析（实测）

| 功能 | 状态 |
|------|------|
| 全部测试函数（709） | ✅ **孤儿归零** — 定义↔注册 100% 一致（v95 时仅 94%） |
| scnedt inlin0（重命名前） | ⚠️ 覆盖"init 状态"而非"pinlin/inlin 行为"（MEDIUM-1） |
| 多维数组/DIM/字符串堆/gweval synchr | ✅ 全覆盖并运行 |
| .bas 集成 65/67 | ✅ 连续 5 轮稳定 |

**测试全面性评分: A** — 断言 1745、.bas 65/67、孤儿 0；测试基础设施缺口（v95 起 42 孤儿）全部闭环，进入"全覆盖注册"状态。

---

## 四、改进建议

### 短期（当前 sprint）
1. 修复 scnedt inlin0 语义漂移：改名 `test_scnedt_init_state_*`（与内容相符）或恢复被测函数调用 + 真实断言。
2. main() 注册块**全局格式化一次**（clang-format，4 空格基准），终结第 5 轮缩进漂移。
3. 合并 `string_heap_impl` 入 `string_heap`；修正注册数字口径。

### 中期（下个 sprint）
4. **零警告冲刺**（107 → ≤50 → 0）— 优先级：测试侧 const 指针 35 条（next86/call86/gweval_helpers）> test_framework 格式串 6 条 > bimisc 隐式声明（补 gwbasic.h 声明）> 生产侧 oem 存根标注。
5. 删除根 Makefile TEST_SRCS 死变量（或改为 include tests/Makefile）。

### 长期
6. "定义↔注册一致性"自动校验入 CI（第 4 轮建议 — 人工检测已连续 6 轮，孤儿问题可视为已根除但需防回归）。
7. run_test 包装内全局状态重置（错误码/变量表），从机制上消灭跨测试泄漏。

---

## 五、总结

**总体评价**: 里程碑轮 — **孤儿归零**（v95 起 42→0）、EOF LF/注释/断言全部兑现、声称-实际偏差收敛至 1 处数字口径；测试 1745/1745、.bas 65/67、警告 107 三线稳定。
**关键发现**: 0 CRITICAL / 0 HIGH / 4 MEDIUM / 3 LOW。测试基础设施问题彻底收官，剩余均为工程治理项（缩进、零警告、死代码、文档数字口径）。
**建议**: 短期修 scnedt 测试语义与缩进；中期零警告冲刺（目标 ≤50）；长期 CI 自动校验 + 测试隔离。下轮关注：警告 ≤50、缩进归一、满月轮（v100 后）可考虑收敛审查节奏。