# GW-BASIC C Port - Code Review v123

**日期**: 2026-09-30
**审查范围**: commit `b01128f`（CODE_REVIEW v122 的 4 LOW 修复）→ 工作区
**测试状态**: 单元测试 **2989/2989 通过**（0 失败）✅ | .bas 集成 **66/68**（0 失败 + 2 预期跳过）✅ | test_renum.bas **输出校验 PASS** ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: modified: CODE_REVIEW.md（v122 报告未入库 → 本版更新为 v123）
**总体评分**: A（优秀）— 0 CRITICAL / 0 HIGH / 0 MEDIUM，1 个 LOW（文档入库,流程项）

---

## 一、概述

本次为 v123 复核，范围是开发者对 v122 报告 4 个 LOW 的修复提交 `b01128f`。

### v122 4 LOW 修复确认（全部验证通过）

| # | v122 问题 | 修复（b01128f） | 独立验证结果 |
|---|-----------|----------------|--------------|
| LOW-1 | test_renum.bas 未收录 run_bas_tests.sh | `IMPLEMENTED_TESTS` 列表尾部添加 `"test_renum.bas"` | ✅ 套件现为 68 项，test_renum 自动运行 |
| LOW-2 | 无 expected 输出基线 | 新增 `tests/bas/test_renum.expected`（10 行全量输出） | ✅ 命名符合 `compare_output` 约定（`${test_name}.expected`），内容与实测输出逐行一致，套件显示 **输出校验 PASS**（非 ⚠️ 未校验） |
| LOW-3 | v121 草稿数字 2998≠2989 | 草稿已被 v122 报告覆盖；DEVELOPMENT_PROGRESS.md 统一 test_runner 口径 | ✅ 实测 2989/2989，与提交记录一致 |
| LOW-4 | v122 过程日志噪音 | 删除 build_v122/test_output_v122/test_run_v122/full_build.log | ✅ 工作区已无相关未跟踪文件 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译 82 个 .c） | ✅ exit 0 |
| 编译警告（生产 + 测试） | ✅ **0 条** |
| 单元测试（tests/test_gwbasic.exe） | ✅ **2989/2989 PASS**（Total tests run = 2989） |
| .bas 集成套件（run_bas_tests.sh，68 项） | ✅ **66/68 PASS**（0 失败 + 2 预期跳过：test_on_goto/test_onerr_off） |
| test_renum.bas | ✅ **PASS** — 已自动收录 + 10 行 expected 输出校验通过 |
| 孤儿检测（唯一函数名口径） | ✅ **754 定义 = 754 注册，0 孤儿 / 0 多余** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **CODE_REVIEW.md 报告未入库** — HEAD 中 CODE_REVIEW.md 仍为 v106（2026-09-13），v107–v122 各轮报告仅存在于提交消息与工作区，`b01128f` 修复了 v122 的 4 LOW 但未提交报告文件本身 | `CODE_REVIEW.md` |

**详情**:

1. **LOW-1 — 报告文档与提交记录脱节**：`b01128f` 提交消息为 "fix: CODE_REVIEW v122 4 LOW"，但 `git show b01128f --stat` 仅含 `run_bas_tests.sh`、`tests/bas/test_renum.expected`、`DEVELOPMENT_PROGRESS.md` 三个文件，CODE_REVIEW.md 未随提交更新（工作区 v122 报告至今未入库）。git log 显示 v107 之后所有 review 结论均以提交消息承载，CODE_REVIEW.md 内容停在 v106。**影响**：文档与代码不同步，未来回溯 review 历史需翻提交消息。**建议**：本版 v123 报告（涵盖 v107–v123 连续结论）随下一提交一并入库，恢复文档单一事实源。

---

## 三、架构评估

### 修复质量评估（b01128f）

| 决策 | 评价 |
|------|------|
| IMPLEMENTED_TESTS 尾部追加 test_renum.bas | ✅ 正确 — 与其他条目格式一致，无重复、无错位 |
| expected 命名 `test_renum.expected`（非 `.bas.expected`） | ✅ 正确 — 与 `compare_output` 的 `$TEST_DIR/${test_name}.expected` 约定吻合（DEVELOPMENT_PROGRESS.md 已记录此约定） |
| expected 内容 10 行 = 实际输出 | ✅ 逐行一致（含 TEST 1-4 场景标题、PASS 断言、ALL DONE） |
| compare_output 行为复查 | ✅ `$(cat | sed)` 规范化（CRLF→LF）+ 精确字符串比较；无 expected 文件时降级 rc-only（return 2 → ⚠️） |
| v122 日志清理 | ✅ 工作区仅剩 CODE_REVIEW.md 修改，无残留过程噪音 |

### 关键代码复核（无源码变更）

| 模块 | 状态 |
|------|------|
| `gwsts.c` RENUM 语句分支（L564-596） | ✅ 存在 — `starts_with("RENUM")` 解析 NN[,MM[,INC]] |
| `gwbasic.h` API 声明 | ✅ `gwlist_reseq` / `gwlist_delete_range` 已声明 |
| INC=0 显式错误 | ✅ 符合 ASM L3140-3142（test_gwsts_renum_inc_zero 覆盖） |
| DELETE 语句分支（gwsts.c 接入 gwlist_delete_range） | ⏳ 仍未实现 — M1 待办（开发者已声明为下一步） |
| M1 模块进度 | 3/N（RANDOMIZE ✓ / RESEQ+DELETE range API ✓ / RENUM 语句 ✓ / RENUM .bas+expected ✓） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| 单测（test_runner） | ✅ 2989/2989，0 失败 |
| 注册一致性 | ✅ 754=754，0 孤儿 0 多余 |
| .bas 集成（68 项） | ✅ 66/68（0 失败 + 2 预期跳过） |
| test_renum.bas 自动化 | ✅ **已收录** + expected 输出校验（v122 LOW-1/2 关闭） |
| 测试盲区 | ⚠️ DELETE 语句分支无语句级测试（待语句实现后补充，与 M1 计划一致） |

---

## 四、改进建议

### 短期（当前 sprint）
1. **CODE_REVIEW.md 入库**：提交 v123 报告时一并提交（LOW-1，本次唯一遗留项）
2. **DELETE 语句分支**（gwsts.c 接入 `gwlist_delete_range`，同 RENUM 模式）+ 语句级 .bas 集成测试（含 expected）
3. 为其余 65 个 .bas 测试补充 expected 基线（当前多数仍为 ⚠️ rc-only 校验，输出未断言）

### 中期
4. 验证 nodes[256] 数组大小是否足够（ASM 64K 程序上限）
5. 增加 AddressSanitizer 构建

### 长期
6. CI 自动化门禁：孤儿检测 + 零警告校验 + 缩进检查 + .bas 集成自动回归
7. 定期运行 .bas 集成测试

---

## 五、总结

**总体评价**: v122 的 4 个 LOW 全部修复且独立验证通过 —— 单测 2989/2989、.bas 套件 66/68（test_renum 已自动化并通过输出校验）、0 警告、0 孤儿。修复正确、无回归。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM，**1 个 LOW**（CODE_REVIEW.md 报告未入库，流程/文档项）。无阻塞性缺陷。
**建议**: 提交 v123 报告时一并入库 CODE_REVIEW.md；随后推进 DELETE 语句分支。

---

## 六、指标追踪

| 指标 | v120 | v121 | v122 | v123 | 变化 |
|------|------|------|------|------|------|
| 单元测试通过（test_runner） | (2998*¹) | 2989 | 2989 | **2989** | 保持 |
| 定义数（唯一函数） | 754 | 754 | 754 | **754** | 保持 |
| 注册数（run_test 调用） | (755*²) | 754 | 754 | **754** | 保持 |
| 孤儿数 | 0 | 0 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | 0 | 0 | **0** | 保持 |
| 生产警告 | 0 | 0 | 0 | **0** | 保持 |
| 测试警告 | 0 | 0 | 0 | **0** | 保持 |
| .bas 通过 | 65/67 | 65/67 | 65/67 | **66/68** | ✅ +1（test_renum 收录） |
| test_renum.bas | — | 手动 4/4 | 手动 4/4 | **套件自动 + 输出校验** | ✅ 自动化 |
| M0 模块完成 | 4/4 | 4/4 | 4/4 | **4/4** | 保持 |
| M1 模块完成 | 3/N | 3/N | 3/N | **3/N** | 保持 |
| RENUM 语句分支 | ✅ | ✅ | ✅ | **✅** | 保持 |

*¹ v120 草稿记录的 2998 为 grep 统计口径漂移，自 v122 起统一采用 test_runner TEST SUMMARY 口径（2989）。
*² v120 记录的 755 注册为 cmd `findstr` 误计（含 `int run_test(` 定义行），按唯一函数名精确复核为 754。

**注释**:
- 本轮变更仅涉及测试基础设施（run_bas_tests.sh +1 行、test_renum.expected +10 行、DEVELOPMENT_PROGRESS.md），无 src/ 源码改动
- 独立验证全程在当前 checkout 重跑：干净重建 → 单测 → .bas 套件 → 孤儿检测
- **M1 进度**: 3/N，下一步为 DELETE 语句分支（gwsts.c 接入 gwlist_delete_range，做语句级测试）
- 待办: CODE_REVIEW.md 报告入库（v107 起各轮报告仅存于提交消息，建议 v123 一并恢复文件单一事实源）