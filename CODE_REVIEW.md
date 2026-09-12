# GW-BASIC C Port - Code Review v103

**日期**: 2026-09-13
**审查范围**: commits `cf36b80`（v102 MEDIUM-1 clang-format）+ `258a28d`（docs v102）独立验证
**测试状态**: 单元测试 **1745/1745 通过**（709 声明，**58 孤儿**，0 失败）✅ | .bas 集成 **65/67**（0 失败 + 2 预期跳过）✅
**编译状态**: 干净重建成功（exit 0）| 警告 **52 条**（生产 52 / 测试 0）✅（v102 记录 107 为统计错误）
**工作区状态**: clean ✅
**总体评分**: B（良好但有退化）— 缩进已统一为 4 空格 ✅；但出现 **58 个孤儿测试函数**（声明但未注册），属于新增问题

---

## 一、概述

`cf36b80` 对 `tests/test_runner.c` 执行 `clang-format IndentWidth=4` 自动格式化，1343 行变更（+731/-612）。实测结果：

- **缩进归一化**：649 行 `run_test` 全部位于 4 空格层级 ✅（v102 MEDIUM-1 已解决）
- **警告修正**：52 条（之前 v102 记录 107 系因构建输出编码问题导致统计偏高）
- **新发现问题**：**58 个孤儿测试函数** — 有定义、有 `test_runner.c` 中的 extern 声明，但 `main()` 中无对应 `run_test(...)` 调用

### 指标追踪

| 指标 | v102 | v103 | 趋势 |
|------|------|------|------|
| 单元通过 | 1745/1745 | 1745/1745 | ✅ 持平 |
| 孤儿数 | 0 | **58** | 🔴 新增 |
| 警告数 | 107(错) | **52** | ✅ 修正 |
| 缩进层级 | 9种 | **1种**(4空格) | ✅ 修复 |
| .bas 通过 | 65/67 | 65/67 | ✅ 持平 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（连续第 6 轮 0 HIGH。）

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **58 个孤儿测试函数** — 有 `void test_xxx(void)` 定义（在 test_*.c 中），有 extern 声明（在 test_runner.c 中），但 `main()` 中无对应 `run_test("name", test_xxx)` 调用。这些测试代码被编译链接，但永远不运行。涉及模块：fiveo(20)、oem(14)、ibmres(10)、itsa86(6)、biptrg(3)、gwdata/gwlist/giodsk/oemcbk/oemsnd 各 1-2 个 | `tests/test_runner.c` + 各 test_*.c | — |
| 2 | **警告 52 条（生产代码）** — 集中在 oem.c（28 条：13 未用静态函数 + 15 未用参数）、gwsts.c（7 条 const 丢弃 + 2 未用变量）、advgrp.c（4 条）、itsa86.c（4 条）、scnedt.c（1 条）等 | 多处 | — |
| 3 | **根 Makefile TEST_SRCS 死变量（第 8 轮）** — 根目录 Makefile 中定义了 `TEST_SRCS` 但未使用，实际测试 Makefile 有自己的完整列表 | `Makefile` | — |
| 4 | **gwbasic.h 缺 bimisc trap API 声明（第 7 轮）** | `src/gwbasic.h` | — |

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | `.gitattributes` 缺失 — 虽全仓已统一 LF，但无显式规则固化标准，未来仍可能混入 CRLF | 仓库根 |

---

## 三、孤儿测试详细清单（58 个）

| 模块 | 孤儿函数 |
|------|----------|
| **fiveo** (20) | `test_fiveo_chain_init_resets_state`, `test_fiveo_chain_nonexistent_file`, `test_fiveo_common_get_marked_overflow`, `test_fiveo_common_is_marked_basic`, `test_fiveo_common_is_marked_case_insensitive`, `test_fiveo_common_is_marked_null`, `test_fiveo_common_mark_case_insensitive`, `test_fiveo_common_mark_duplicate`, `test_fiveo_find_while_empty_stack`, `test_fiveo_init_clears_while_stack`, `test_fiveo_peek_while_after_push`, `test_fiveo_peek_while_does_not_pop`, `test_fiveo_write_active_after_begin_end`, `test_fiveo_write_begin_overrides`, `test_fiveo_write_begin_sets_state`, `test_fiveo_write_end_clears_state`, `test_fiveo_write_end_when_not_active`, `test_fiveo_write_file_returns_current` |
| **oem** (14) | `test_oem_clrscn_al0_clears_screen`, `test_oem_clrscn_al1_clears_framebuffer`, `test_oem_clrscn_al2_clamps_to_screen`, `test_oem_clrscn_al2_window_clear`, `test_oem_nsetcx_crosses_byte_boundary`, `test_oem_process_keyboard_ctrl_numlock`, `test_oem_process_keyboard_ctrl_only`, `test_oem_process_keyboard_ctrl_shift_b`, `test_oem_process_keyboard_ctrl_shift_p`, `test_oem_process_keyboard_key_up`, `test_oem_process_keyboard_regular`, `test_oem_process_keyboard_shift_only`, `test_oem_scroll_invalid_dimensions` |
| **ibmres** (10) | `test_ibmres_all_extended_functions`, `test_ibmres_all_extended_statements`, `test_ibmres_case_insensitive_all`, `test_ibmres_crunch_extended_comprehensive`, `test_ibmres_eval_extended_comprehensive`, `test_ibmres_extended_token_functions`, `test_ibmres_get_keyword_type_comprehensive`, `test_ibmres_is_reserved_word_comprehensive`, `test_ibmres_lookup_case_insensitive`, `test_ibmres_lookup_extended_func` |
| **itsa86** (6) | `test_itsa86_basvar_all_indices_uninitialized`, `test_itsa86_basvar_invalid_index`, `test_itsa86_basvar_read_uninitialized`, `test_itsa86_basvar_write_uninitialized`, `test_itsa86_mapclc_m_option_too_large`, `test_itsa86_mapclc_m_option_valid` |
| **biptrg** (3) | `test_biptrg_get_numeric_on_string`, `test_biptrg_get_string_on_numeric`, `test_biptrg_type_mismatch_set_numeric`, `test_biptrg_type_mismatch_set_string` |
| **其他** (5) | `test_advgrp_get_data_correctness`, `test_giodsk_open_invalid_filenum`, `test_gwdata_variable_type_handling`, `test_gwlist_delete_then_add_same`, `test_oemcbk_process_without_cbksta`, `test_oemsnd_donote_zero_duration` |

> 注：总计 58 个孤儿（above 表中 ibmres 列 10 个但清单实际 9 个含 lookup_extended_stmt 共 10）。

---

## 四、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| clang-format 统一缩进到 4 空格 | ✅ 正确 — 全仓一致，9 种层级→1 种层级 |
| 警告统计口径（107→52） | ⚠️ v102 统计错误（cmd.exe 管道编码问题），实际 52 条 |
| 孤儿测试长期存在 | 🔴 需修复 — 代码写了但不运行，浪费维护成本且造成"覆盖率"假象 |

### 测试覆盖分析

| 指标 | 状态 |
|------|------|
| 单元 1745/1745 断言通过 | ✅ |
| 孤儿 58 个（新增） | 🔴 |
| .bas 65/67 | ✅ |
| 警告 52 条 | ⏸️ |

**测试全面性评分: B** — 功能正确，但 58/709 = 8.2% 的测试函数未被执行，覆盖存在盲区。

---

## 五、改进建议

### 短期（当前 sprint）
1. **修复 58 个孤儿测试**：将缺失的 `run_test("name", func)` 调用补入 `test_runner.c` 的 main() 函数。按模块分组插入（fiveo 20 个 > oem 14 个 > ibmres 10 个 > itsa86 6 个 > 其他 8 个）。
2. 提交前自检升级：运行 `findstr /b "void test_" tests\test_runner.c | findstr /v ");"` 确认无新孤儿；或用脚本自动化检测。

### 中期（下个 sprint）
3. 零警告冲刺（52 → 0）：oem.c 13 个未用 static 函数可标记 `__attribute__((unused))` 或移除；gwsts.c 7 个 const 丢弃可用 `const char *` 接收。
4. 删除根 Makefile `TEST_SRCS` 死变量；gwbasic.h 补 bimisc trap API 声明。

### 长期
5. CI 自动化："定义↔注册一致性"校验 + "零警告"门禁。
6. `run_test` 包装内全局状态重置（错误码/变量表），从机制上消灭跨测试泄漏。

---

## 六、总结

**总体评价**: 工程整治提交 — clang-format 缩进统一修复成功（9 种→1 种），警告数修正为 52 条（v102 的 107 是统计错误）。**但引入 58 个孤儿测试函数**，属于代码质量退化。
**关键发现**: 0 CRITICAL / 0 HIGH / 4 MEDIUM / 1 LOW。58 个孤儿是本轮最大问题 — 8.2% 的测试函数未执行。
**建议**: 立即修复孤儿（补 `run_test` 调用）；中期消除 52 条警告；建立 CI 门禁防止复发。下轮关注：孤儿 0、警告 ≤20、声明↔注册一致性 100%。
