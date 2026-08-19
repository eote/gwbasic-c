# GW-BASIC C Port - Code Review v64

**日期**: 2026-08-20
**审查范围**: 工作区状态确认（src 还原至 v60 基线 33c0f1d）+ 遗留测试资产审查
**测试状态**: 1084/1084 单元测试通过 ✅（干净重建）
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B (良好) — 测试套件健康；遗留资产需决策去留

---

## 一、概述

本次审查覆盖**代码还原后的状态**：v61-v63 期间的源码功能开发（WHILE/WEND、比较运算符、数学函数、多维数组修复，约 800+ 行）已被还原，src/ 恢复至 v60 基线（`33c0f1d`）。

**审查期间用户正在还原文件** — 初始 git status 显示 test_biprtu_biptrg.c / test_fiveo_ext.c / test_runner.c 为 modified，审查过程中被还原（干净重建后确认）。

**当前真实状态:**

| 区域 | 状态 |
|------|------|
| `src/` | ✅ 与 HEAD (33c0f1d) 完全一致，无未提交变更 |
| `tests/test_*.c` | ✅ 已还原（biprtu/fiveo/runner） |
| `tests/bas/test_arrays.bas` | ⚠️ modified（+PASS 输出，无条件打印） |
| `tests/bas/test_while_wend.bas` | ⚠️ modified（-1 删除注释） |
| 37 个 .bas 调试测试 | ❓ 未跟踪（v61-v63 功能测试，依赖已还原功能） |
| `.reviewer.md` + `AGENTS.md` | ❓ 未跟踪（审查契约文档 — 建议入库） |

**验证（干净重建）:**
- ✅ 1084/1084 单元测试通过 — **之前的链接错误是陈旧 .o 产物**（v63 编译的 test_biprtu_biptrg.o 引用已还原的 `biptrg_dim_array_multi` 等符号），clean 重建后消失
- ✅ 解释器基线功能正常：test_print_file (PASS)、test_for (5 次)、test_goto_gosub（完整）、test_hello/print_bare/empty_stmt
- ❌ test_while_wend.bas: "Syntax error: WEND"（WHILE 已还原 — 预期失败）
- ❌ test_arrays.bas: "A(1) = -3.86422e-32"（数组损坏回归 v60 基线 — 预期，v63 修复被还原）

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 说明 |
|---|------|------|------|
| 1 | 37 个未跟踪 .bas 调试测试资产 — 状态未决策 | `tests/bas/` | 依赖已还原功能，当前解释器运行全部失败 |

**详情:**

#### HIGH-1: 37 个 .bas 测试资产归属未决策

工作区有 37 个未跟踪 .bas 文件（v61-v63 期间为验证 WHILE/数组/字符串/LEFT$/LEN/PRINT USING 等功能编写的调试测试）：

| 类别 | 文件示例 | 依赖功能 | 当前状态 |
|------|----------|----------|----------|
| WHILE | test_while_debug.bas | WHILE/WEND | ❌ 已还原 |
| 数组 | test_array_debug/simple/zero/scalar.bas | 数组修复 | ❌ 已还原（v60 数组损坏）|
| 赋值/表达式 | test_assign/cross_assign/expr*.bas | 赋值/表达式 | ⚠️ 部分可用 |
| 字符串 | test_left*/test_len*/test_string_*.bas | 字符串函数 | ❌ 未实现（v60 无）|
| PRINT USING | test_using_*.bas | USING 格式 | ❌ 未实现 |
| 基础 | test_basic.bas / test_print_hello.bas | 基础 PRINT | ✅ 可用 |

**影响/建议（三选一，需用户决策）:**
1. **跟踪保留**：`git add tests/bas/*.bas` — 作为未来功能重做的验收资产（2026-08-16 先例：tests/bas 26 文件曾先入库后实现）
2. **部分跟踪**：仅保留与当前基线功能匹配的（test_basic/print_hello 等）
3. **忽略**：加入 .gitignore 或删除 — 若不再计划重做这些功能

> ⚠️ 注意：这些 .bas 大多**无断言**（只 PRINT 结果）— 即使未来功能实现，也需要 test_print_file.bas 的断言模式改造。

---

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | test_arrays.bas 的 "PASS" 无条件打印（无断言逻辑 — 数组错时也 PASS）| `tests/bas/test_arrays.bas` |
| 2 | test_arrays.bas / test_while_wend.bas 修改与当前 src 不匹配（引用已还原功能）| `tests/bas/` |
| 3 | 数组数据损坏在 v60 基线存在（A(1) = -3.86422e-32）— 曾有修复（v63）被还原 | `src/biptrg.c`/`src/gweval.c` |
| 4 | 工作区大量未跟踪文件（39 个 .bas + 2 个 md）— .gitignore 未覆盖 | 根目录 |

**详情：**

#### MEDIUM-1: test_arrays.bas 无条件 PASS

```basic
70 NEXT I
80 PRINT "PASS"      ← 无条件执行（无论 A(I) 值正确与否）
90 END
```

本次修改新增 "PASS" 输出，但**无校验** — 数组输出错误（-3.86e-32 垃圾值）时也打印 PASS，误导测试结果。应改为断言模式（test_print_file.bas 的 LINE INPUT 回读比对先例）：

```basic
80 IF A(1) = 2 AND A(2) = 4 THEN PRINT "PASS" ELSE PRINT "FAIL": END
```

#### MEDIUM-3: 数组损坏（v60 基线遗留 + 曾被修复后还原）

- v60 基线：数组损坏（v61 报告 HIGH-1：A(1)=1.35426e+07）
- v61-v63 修复：calloc 清零 + multi 接口（实测正确）
- 现状态：修复被还原，回到损坏（-3.86422e-32）
- **功能重做时需重新实施** — 这是已记录在案的已知问题

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | .reviewer.md / AGENTS.md 审查契约未跟踪（建议入库，供后续审查者遵循）| 根目录 |
| 2 | test_arrays.bas & test_while_wend.bas 均无末尾换行（`\ No newline` — 模式持续）| `tests/bas/` |
| 3 | tests/bas 现有 26 个已跟踪 .bas 中 test_while_wend.bas 将永远失败（WHILE 已还原）— 应标记"待重做"或移除跟踪 | `tests/bas/` |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| src 还原至 v60 基线 | ✅ 用户有意为之（功能开发重新组织/重做）|
| tests/ 同步还原 | ✅ 测试套件恢复可构建（1084 通过）|
| 保留 37 个 .bas 调试测试未跟踪 | ⚠️ 状态未决 — 建议决策 |
| 新增 .reviewer.md / AGENTS.md | ✅ 审查契约 — 规范后续协作（含 Git 规则：未经"批准合并"不得 commit/restore）|

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1084/1084（干净重建）|
| 解释器基线功能 | ✅ PRINT#/FOR/GOTO/GOSUB/裸 PRINT/空语句（实测）|
| 数组 | ❌ 损坏（v60 基线，曾修复后还原）|
| WHILE | ❌ 已还原（test_while_wend.bas 报 Syntax error）|
| 字符串/数学函数/比较 | ❌ 已还原 |

**测试全面性评分: B** — 基线测试健康；还原的功能无测试（等待重做）。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟧 HIGH-1 决策**: 37 个 .bas 的去留（跟踪 / 部分跟踪 / 忽略）— 需用户明确
2. **🟨 MEDIUM-1**: test_arrays.bas 改断言模式（无条件 PASS → 条件 PASS/FAIL）
3. **🟩 LOW-1**: AGENTS.md + .reviewer.md 入库跟踪（审查契约价值高）
4. **🟩 LOW-2**: 补测试文件末尾换行

### 中期 (下个 sprint)

5. **功能重做路线（还原内容重新实施）**: 数组（calloc 清零）→ WHILE/WEND → 比较运算符（-1/0 布尔）→ 数学函数 → 字符串函数链路（LEN=0 问题 + PRINT 泄漏同源）→ PRINT USING
6. 2026-08-16 的先例模式：先测试资产入库 + TEST_PLAN 分阶段，后实现

### 长期

7. .bas 断言机制标准化（PASS/FAIL 模式推广到所有测试）
8. pre-commit 工具（换行/行尾符/误提交 — 多轮遗留）
9. 明确"还原"策略的文档化（避免 v51/v64 这类状态反复）

---

## 五、总结

**总体评价**: 当前工作区是干净的 v60 基线 + 测试资产遗留状态。单元测试套件 1084/1084 健康（还原后干净重建验证）；解释器基线功能（PRINT#/FOR/GOTO/PRINT 变体）实测正常。**待决策项**：37 个未跟踪 .bas 调试测试的归属，以及审查契约文档（AGENTS.md/.reviewer.md）是否入库。之前的链接错误为陈旧 .o 产物，已随 clean 重建消失。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 1（37 个 .bas 状态未决）
- 🟨 MEDIUM: 4（PASS 无条件、测试与 src 不匹配、数组损坏遗留、未跟踪泛滥）
- 🟩 LOW: 3

**编译与测试**:
- ✅ 1084/1084 单元测试通过（clean 重建）
- ✅ 基线 .bas: print_file/for/goto_gosub/hello/print_bare/empty_stmt 全部正确
- ❌ test_while_wend.bas: "Syntax error: WEND"（预期）
- ❌ test_arrays.bas: 数组垃圾值 -3.86e-32（v60 基线遗留，曾修复后还原）

**跨版本状态追踪（v60→v64）**:

| 版本 | 状态 |
|------|------|
| v60 | 基线（33c0f1d 提交）|
| v61-v63 | 功能开发（未提交，WHILE/数组/数学）|
| **v64** | **功能还原回 v60 基线；测试资产遗留** |

**已知问题清单（重做路线参考）**:
1. 数组损坏（原修复：calloc + multi 接口）
2. WHILE/WEND 缺失
3. 布尔 1/0（应 -1/0）
4. 字符串函数链路 LEN=0（与 PRINT 字符串泄漏同源）
5. PRINT USING 格式
6. 未实现语句：KILL/FIELD/READ/DATA/INPUT/ON GOTO

**建议**: 请决策 37 个 .bas 和 2 个 md 文件归属；随后按"测试资产先入库 → 按 TEST_PLAN 分阶段重做功能"的既定模式推进。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-20
**审查版本**: v64 — 代码还原后状态确认 + 遗留测试资产审查