# GW-BASIC C Port - Code Review v129

**日期**: 2026-10-06
**审查范围**: commit `0497172`（fix: CODE_REVIEW v128 — CRITICAL-1 RESEQ 泄漏 + LOW-1 版本计数器）→ 工作区
**测试状态**: 单元测试 **3066/3066 通过**（0 失败）✅ | .bas 集成 **68/70**（0 失败 + 2 预期跳过）✅ | probe 复测全绿 ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean（v128 报告已随 0497172 入库）
**总体评分**: A（优秀）— v128 的 1 CRITICAL + 1 LOW 全部修复验证通过，**0 新问题**；M1 正式关闭

---

## 一、概述

本次审查开发者对 v128 问题的修复提交 `0497172`（7 文件，+171/-116）。开发者**超额完成**：不仅修复泄漏与版本语义，还落地了 reviewer 建议的分配/释放计数器与泄漏回归测试。

### v128 修复确认（全部验证通过）

| v128 问题 | 修复方式（0497172） | 独立验证 |
|-----------|--------------------|----------|
| 🔴 CRITICAL-1 RESEQ 成功路径泄漏 | `return SUCCESS` 前补 `gwram_free(nodes)`（L105）+ **gwram 分配/释放计数器**（g_alloc_count/g_free_count + getter，init 重置，alloc/realloc/calloc/free 全路径递增）+ `test_gwlist_reseq_no_leak`（100×RESEQ×300 行 alloc==free 平衡断言） | ✅ 代码复核 + 单测 3066 内 balance 断言通过 |
| 🟩 LOW-1 版本计数器未覆盖 reseq/clear | `gwlist_reseq` 成功路径与 `gwlist_clear` 补 `g_link_version++`，与 v127 提交消息"delete/add/reseq/clear/init 递增"口径一致 | ✅ 代码复核 + probe（运行中 RENUM）无回归 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3066/3066 PASS**（776 → 777，+1 test_gwlist_reseq_no_leak） |
| .bas 集成套件（70 项） | ✅ **68/70**（0 失败 + 2 预期跳过），test_renum/test_delete/test_delete_exec 均输出校验 PASS |
| 孤儿检测（唯一函数名） | ✅ **777 定义 = 777 注册，0 孤儿 / 0 多余** |
| probe：删除紧邻下一行 | ✅ `20 DELETE 30` → `start / after`（行 30 不执行） |
| probe：运行中 RENUM（reseq 递增版本后 fast path） | ✅ `20 RENUM 100,10,10` → `start / after renum`（程序正确继续） |

### gwram 计数器实现复核（关键点全通过）

| 路径 | 计数 | 正确性 |
|------|------|--------|
| `gwram_alloc(size)` 成功 | alloc++ | ✅ fail 不计 |
| `gwram_realloc(NULL, size)` | 委托 alloc | ✅ NULL 分支正确 |
| `gwram_realloc(ptr, 0)` | 走 free | ✅ 0 大小分支正确 |
| `gwram_realloc(ptr, size)` 成功 | alloc++ + free++（realloc = free+alloc） | ✅ realloc 失败（p==NULL）不计数（旧块保留） |
| `gwram_calloc` 成功 | alloc++ | ✅ 溢出检查在计数前 |
| `gwram_free(ptr)` 非 NULL | free++ | ✅ NULL 不计 |
| `gwram_init()` | 双计数重置 | ✅ 测试隔离（每测试自 init） |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无。

本版为修复确认轮：v128 的 1 CRITICAL + 1 LOW 已关闭，无新增问题。

**备注（不计问题）**：`gwlist_reseq` 的 FCERR 分支（`new_num > 65529` 中途返回 ERROR）未递增 `g_link_version` —— 但该分支此后语句返回 ERROR，执行循环中断（不再走 fast path），无实际影响；若未来改变错误处理方式可再评估。

---

## 三、架构评估

### 修复质量（0497172）

| 决策 | 评价 |
|------|------|
| `gwram_free(nodes)` 补于成功路径（错误路径原已正确） | ✅ CRITICAL-1 关闭 |
| **gwram 分配/释放计数器 + 平衡断言**（超出建议） | ✅ 优秀 — 为全项目提供可复用的泄漏回归机制 |
| `test_gwlist_reseq_no_leak`（100×RESEQ，alloc==free + 300 行保持） | ✅ 高质量 — 可捕获泄漏回归；gwdata/gwram/gwlist 自 init 隔离正确 |
| reseq/clear 补 `g_link_version++` | ✅ LOW-1 关闭，版本语义单一化 |
| 计数器全路径复核（alloc/realloc/calloc/free × NULL/0/失败分支） | ✅ 实现正确（见上表） |
| CODE_REVIEW.md / DEVELOPMENT_PROGRESS.md 随提交入库 | ✅ 流程闭合，工作区 clean |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| RESEQ 语义（默认/起点/MM/INC=0/溢出/空/大程序/内容） | ✅ 8+ 用例 |
| **RESEQ 内存平衡（成功路径 + 100 次循环）** | ✅ **test_gwlist_reseq_no_leak（新）** |
| 版本计数器语义（delete/add/reseq/clear/init 全覆盖） | ✅ 提交后实现与口径一致 |
| 执行指针（删除/RENUM fast path 与 walk） | ✅ probe 复测无回归 |
| 注册一致性 | ✅ 777=777，0 孤儿 0 多余 |
| 测试盲区 | ⚠️ 65 个 .bas 仍为 ⚠️ rc-only（无 expected）；**M2 建议项** |

---

## 四、改进建议

### 短期（可选）
1. （备注项）FCERR 分支版本递增 —— 无实际影响，可随 M2 顺手处理

### 中期（M2 规划）
2. **65 个 .bas 补充 expected 基线**（当前 ⚠️ rc-only 校验，输出未断言；含 test_error/test_err2/test_onerr 等错误场景基线）
3. **CI 门禁落地**：孤儿检测 + 零警告 + fresh clone 构建校验 + .bas 集成自动回归 + 计数器平衡断言纳入标准测试
4. **AddressSanitizer 构建**（泄漏/UAF 双保险，与 gwram 计数器互补）

### 长期
5. ON ERROR/GOTO 语句分支（test_on_goto/test_onerr_off 两个预期跳过项 → 实现后转 PASS）
6. 定期运行 .bas 集成

---

## 五、总结

**总体评价**: v128 的 1 CRITICAL（RESEQ 泄漏）与 1 LOW（版本语义）全部修复且超额落地 —— gwram 分配/释放计数器为全项目提供可复用泄漏回归测试（100×RESEQ alloc==free），版本计数器语义闭环；单测 3066/3066、套件 68/70、0 警告、0 孤儿，probe 复测无回归。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**。无阻塞性缺陷。
**M1 状态**: **正式关闭**（RANDOMIZE / RESEQ / DELETE / RENUM / UAF 修复 / nodes[256] 动态化 / 泄漏关闭，7 项全部完成）。
**建议**: M1 关闭，进入 M2 规划（expected 基线补齐为最大收益项；ON ERROR/GOTO 分支为下一个功能里程碑候选）。

---

## 六、指标追踪

| 指标 | v128 | v129 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3064 | **3066** | ✅ +2（+1 测试函数） |
| 定义数（唯一函数） | 776 | **777** | +1 |
| 注册数（run_test 调用） | 776 | **777** | +1 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 68/70 | **68/70** | 保持 |
| RESEQ 泄漏 | 存在（CRITICAL-1） | **关闭**（+计数器回归防护） | 修复 |
| 版本计数器语义 | ⚠️ 缺 reseq/clear | **闭环** | 修复 |
| HEAD 独立构建 | ✅ | ✅ | 保持 |
| M1 状态 | 功能完成 | **正式关闭**（7/7） | 收尾 |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3066 → 套件 68/70 → 孤儿 777=777 → 2 项 probe（已清理）
- gwram 计数器为全项目通用能力（gwram_get_alloc_count/free_count），可支持未来所有模块的泄漏回归测试
- **下一步**: M2 规划（expected 基线补齐 → ON ERROR/GOTO 分支 → CI 门禁）