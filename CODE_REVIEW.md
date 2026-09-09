# GW-BASIC C Port - Code Review v98

**日期**: 2026-09-09
**审查范围**: commit `936a1ce`（v97 HIGH-1' synchr ASM 语义 bug + HIGH-2 多维数组/DIM 注册）独立验证
**测试状态**: 单元测试 **1742/1742 通过**（706 注册函数，0 失败）✅ | .bas 集成 **65/67**（0 失败 + 2 预期跳过）✅
**编译状态**: 干净重建成功（exit 0）| 警告 **107 条**（生产 52 / 测试 55，与上轮持平）
**总体评分**: A- (优秀) — v97 全部 HIGH 项闭环，断言 +13 且多维数组/DIM 覆盖恢复；剩余仅 6 个低价值孤儿与风格/门槛项

---

## 一、概述

`936a1ce` 精确修复 v97 报告的两个 HIGH 项并补齐暴露测试。全部实测通过，提交信息与实际一致（首次），无新引入问题。

### 修复验收总表

| v97 项 | 声称 | 实测结果 | 结论 |
|--------|------|----------|------|
| HIGH-1' synchr 跳空格条件 | 改查匹配后字符 `**text < ':'` | ✅ 与 `BIMISC.ASM:322` (`MOV AL,[BX]; CMP AL,":"; JB SYNCON`) 逐行一致；`'= 42'` 用例断言指针落在 `'4'`（实际运行输出确认 PASS） | **已修复** |
| HIGH-2 高价值孤儿注册 | 4 个（biptrg multi ×3 + gwsts dim_then_set） | ✅ 全部注册并**实际运行**（RUN 输出 4/4 PASS）：多维数组 DIM 2D/3D、multi set/get、DIM 语句解析+数组赋值均恢复覆盖 | **已修复** |

### 实测指标（vs v97）

| 指标 | v97 | v98 |
|------|-----|-----|
| 注册测试函数 | 701 | **706**（+5：4 孤儿 + synchr_operator_eq） |
| 断言（全部通过） | 1729 | **1742**（+13：multi 3 + dim_then_set + operator_eq） |
| 孤儿（定义未注册） | 10 | **6**（全部低价值） |
| .bas 集成 | 65/67 | **65/67** ✅ |
| 警告 | 107 | 107 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（v97 的 2 个 HIGH 均已闭环，本提交无新 HIGH。）

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **main() 缩进混乱恶化** — gweval 块新增的 3 行注册用 16 空格缩进（同块 `gweval_synchr_match` 为 8 空格，函数体基准为 4 空格），v95 起连续 4 轮指出未收敛；本次 diff 中 901-904 行缩进漂移 8→16 造成块内不一致 | `tests/test_runner.c` | 901-904 |
| 2 | **6 个低价值孤儿待清理决策** — ① `test_gwdata_string_heap_impl`/`wrapper`（与已注册 `string_heap` 重复的中间版本，删）；② `test_gwlist_init`（已声明未注册，被 `init_twice` 替代，删或并入）；③ `test_math_debug`（与 `test_math.c:192` 重复，删）；④ `test_scnedt_inlin0_pinlin`/`inlin0_inlin`（空壳：仅调用函数后 `TEST_PASS` 无断言，若保留需补断言） | `tests/test_*.c` | — |
| 3 | **test 错误状态泄漏持续**（v97 MEDIUM-5 未处理）— `test_gweval_synchr_eos` 空串场景设置 error 2 后无清理，残留到后续测试；当前无雷，但缺跑测试隔离机制 | `tests/test_gweval_helpers.c` | 96-103 |
| 4 | **根 Makefile TEST_SRCS 死变量（第 4 轮）** / **gwbasic.h 缺 bimisc trap API 声明（第 3 轮）** — 均未处理；`test_gweval_helpers.c` 重复 ×2 仍在 | `Makefile`、`src/gwbasic.h` | 52-53 |
| 5 | **107 条警告继续超零警告门槛 2 倍** — 生产 52（oem.c 31 未用函数/参数/恒真比较、gwsts 7、ibmres 2 sign-compare 等）、测试 55（next86 18 + call86 8 + gweval_helpers 7 const 指针、bimisc 6、framework 6） | 多处 | — |

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | `test_gweval_helpers.c` 末尾无换行符 + 新测试注释冗余（14 行描述重复 3 遍同一逻辑）；建议简化注释并补文件末换行 | `tests/test_gweval_helpers.c` 126-139 |
| 2 | 新增测试名 `test_gweval_synchr_operator_eq` 与注册名 `gweval_synchr_operator_eq` 前缀规则一致 ✅；但测试内容集中验证 `'='` 单字符，可补 `':'`/`'<'` 参数化变体 | `tests/test_gweval_helpers.c` |
| 3 | synchr 修复中 `**text && **text < ':'` 对 TAB（0x09 < ':' 且非空格）走分支但不消费（while 只认 `' '`）；与 ASM CHRCON 的 LOWCHR 路径（TAB→LOWCHR）有差异，当前无调用者无影响，可记录为已知边界 | `src/gweval.c` 774-780 |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| synchr 修复采用 `**text < ':'` | ✅ 严格对齐 ASM（注释引用 BIMISC.ASM:322），修复最小侵入（2 行） |
| 暴露用例 `"= 42"` 验证运算符场景 | ✅ 类型正确：精确覆盖 v97 指出的 `expected >= ':'` 分支（`=` 0x3D），断言指向 `'4'` 而非空格 |
| 孤儿注册顺序（声明+注册+运行验证） | ✅ 声明区、注册区、RUN 输出三处一致，`gwsts_dim_then_set` 注册名与函数名匹配 |
| 提交信息口径 | ✅ 首次与实测完全一致（1742/1742、65/67 均验证属实） |

### 测试覆盖分析（实测）

| 功能 | 状态 |
|------|------|
| gweval_synchr：匹配/不匹配(SNERR-eq)/EOS/空指针/运算符`=`跳空格 | ✅ 5 路径全覆盖 |
| 多维数组：DIM 2D/3D、set/get multi | ✅ 覆盖恢复（RUN 3/3 PASS） |
| DIM 语句解析 + 数组赋值（gwsts 集成） | ✅ 覆盖恢复（RUN PASS） |
| 低价值孤儿 6 个 | ⚠️ 无价值增量（重复/空壳），待清理 |

**测试全面性评分: A-** — 断言 1742 全绿、.bas 65/67、孤儿仅剩 6 个低价值；主要功能路径（数组含多维、DIM 解析、字符串、文件 I/O、ON ERROR、def FN）均有运行时验证。

---

## 四、改进建议

### 短期（当前 sprint）
1. 清理 6 个低价值孤儿：`string_heap_impl/wrapper`、`math_debug` 直接删除（重复）；`gwlist_init` 删除或并入 `init_twice`；`scnedt_inlin0_*` 补断言或删除 — 目标孤儿归零。
2. 统一 main() 注册块缩进（建议全局一次性 clang-format，4 空格基准）。
3. 补 `\n` 到 `test_gweval_helpers.c` 文件尾 + 精简冗余注释。

### 中期（下个 sprint）
4. 测试隔离：run_test 包装内重置 g_state（error_code/变量表），杜绝跨测试错误码泄漏。
5. 删除根 Makefile TEST_SRCS 死变量；gwbasic.h 补 bimisc trap API 声明。
6. 零警告冲刺（107 → ≤50 → 0）：先清理测试侧 const 指针（next86/call86/gweval_helpers 35 条）与 framework 格式串。

### 长期
7. "定义↔注册一致性"自动校验入 CI（已建议 2 轮 — 人工核对成本已连续 5 轮）。
8. ASM 直译函数"行为等价用例"制度推广到全部解析原语（本轮 synchr 即受益）。

---

## 五、总结

**总体评价**: 收尾轮 — v97 全部 HIGH 项闭环、提交信息首次与实测完全一致、新的暴露用例（`'= 42'`）精准命中修复点；多维数组与 DIM 语句覆盖恢复，断言 1742 全绿、.bas 65/67 保持。
**关键发现**: 0 CRITICAL / 0 HIGH / 5 MEDIUM / 3 LOW。全项目进入"无 HIGH"状态，剩余均为风格、门槛（零警告/缩进/死代码清理）与 6 个低价值孤儿问题。
**建议**: 短期清孤儿与缩进；中期零警告冲刺 + 死变量/声明补齐 + 测试隔离；长期 CI 自动化一致性校验。下轮关注：孤儿归零、警告 ≤50。