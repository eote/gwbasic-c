# GW-BASIC C Port - Code Review v176

**日期**: 2026-11-19
**审查范围**: `5b22dc0` → `9f54670`（v175 修复轮 `9edcd8d` + 进度登记 `9f54670`）
**测试状态**: 断言 **3615/3615 通过** | 测试函数 **973 = 973**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **113/113***（`test_on_error` 1 项仍仅 .expected 缺尾换行，其余 112 项严格字节全等）

**总体评分**: **B（可合并）** — **0 CRITICAL + 0 HIGH + 1 MEDIUM**（gosub 测试隐式依赖前序测试的 960/961 行）+ **2 LOW**

---

## 一、概述

`9edcd8d`（tests 1 文件 + .expected 8 文件）修复 v175 的 MEDIUM-1、LOW-1，并修复 8 项 .expected 尾换行：

| 文件 | 变更 |
|------|------|
| `tests/test_str_semantics.c` | +49：`test_gosub_tail_host_clear` 沿 RETURN→take_tail→tail→clear 链路补 4 断言（RETURN rc=961、tail body B=7、post-tail GOSUB/RETURN 正常、gosub 栈深度 0、行生命周期清理）；for_prefix 注释补 LOW-1 说明 |
| `tests/bas/*.expected` ×8 | 补 `\n` 尾换行（test_def_fn/test_delete/test_delete_exec/test_line_input/test_read_data/test_renum/test_resume_line/test_string_funcs） |
| `DEVELOPMENT_PROGRESS.md` / `CODE_REVIEW.md` | 登记 |

**src 无行为变更**（9edcd8d 仅测试与注释）。

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ 0 警告 |
| 单测 | ✅ **3615/3615**（v175: 3611，+4）；**973=973**，0 孤儿 |
| .bas 全量 | ✅ **113/113**：112 项严格字节全等；`test_on_error` 1 项仍为 expected 缺尾换行（见 MEDIUM/LOW） |
| 回归（src 零行为变更 + .bas 全量） | ✅ 无回归 |

### v175 问题闭环

| v175 | 状态 |
|------|------|
| M1 gosub 断言打空 | ✅ **真闭环**：RETURN→take_tail→tail 复合→clear 全链路驱动；断言①`RETURN` rc=961（宿主续点，host 换算）；断言②`B==7`（tail body 确实执行）；断言③`next86_get_gosub_depth()==0`（栈清理）；下两断言（post-tail GOSUB/RETURN）验证 marker 清除后行为正常 |
| L1 for_prefix 注释 | ✅ 已标注"runner 路径由 T1 .bas 锁定" |
| 9 项 expected 尾换行 | ⚠️ 实修 **8 项**——`test_on_error.expected` **未包含**在提交中，承诺"strict 0 fail"与实测不符（仍 1 项假失败） |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **`test_gosub_tail_host_clear` 隐式依赖前序测试注册的行 960/961：`g_curlin=960` 假设 `test_line_start_comment_data` 先注册了 960（`' note: DATA 9`）与 961（`REM foo: DATA 8`）** | `tests/test_str_semantics.c` | 687-706 |

**详情:** 该测试用 `g_curlin = 960` 模拟 runner（"the GOSUB lives on line 960 (added by an earlier test)"），RETURN 目标断言 `961` 依赖 line 960 存在且 `->next == 961`。当前顺序（test_runner.c 中 `test_line_start_comment_data` 在前）能通过，但：

- 若注册顺序调整（如把 gosub 测试移到 comment 测试之前）→ `gwlist_find_line(960)` 为 NULL → RETURN 目标不再是 961 → **断言脆断**；
- 若 comment 测试被精简/改行号 → 静默改变宿主续点语义，断言可能"假绿"（961 恰好等于期望）或误报。

**修复建议:** 测试内自足——开头自行 `gwlist_add_line(960, "' self-contained");`（与 970/980 同类），不依赖前序测试；或不依赖具体行号，改为断言"RETURN rc 与宿主行的 next 行号一致"（先查 960 的 next 再期望）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | **提交声称"9 项 .expected 尾换行修复 (strict 字节比对 0 fail)"，实际 `test_on_error.expected` 未修改（`tail` 无 `\n`）→ 严格比对仍 112/113，承诺与实测不符**。且该行为"最后 1 个假失败"，下一次修复轮顺手补上即可 |
| 2 | `test_on_error.expected` 既缺尾换行、其内容本身为"错误后不应打印的行堆叠"基线——建议在补 `\n` 的同时确认该 .bas 语义（错误后 `PRINT` 不应出现）有测试断言 | 
| 3 | `test_gosub_tail_host_clear` 中 `gwlist_delete_line(970)/(980)` 清理了自建行，但 960/961 属前序测试的生命周期，未清理属预期（进一步说明耦合） |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| gosub tail host 真断言（沿 take_tail→tail→clear） | ✅ 与 v175 建议一致：断言能失败于"宿主续点换算错"与"tail body 未执行"两类回归；`next86_get_gosub_depth()==0` 锁栈清理 |
| `next86_get_gosub_depth` 复用（v148 已有） | ✅ 无新增 API 面 |
| g_curlin 模拟 runner | ⚠️ 方向正确（runner 确实设 g_curlin），但硬编码 960/961 产生前序测试依赖（MEDIUM-1） |
| 8/9 expected 尾换行 | ⚠️ 修复方向正确、逐一 diff 确认 2±1 行变化；漏 `test_on_error`（LOW-1） |

**根因**: MEDIUM-1 从"打空断言"到"真链路断言"完成质变；残留问题集中在测试独立性（g_curlin 依赖前序测试行号）与提交完整性（expected 9 项漏 1）。

---

## 四、改进建议

### 短期
1. **MEDIUM-1**: `test_gosub_tail_host_clear` 开头自足 `gwlist_add_line(960, "' self-contained");`，删除对前序测试的隐式依赖；断言宿主续点改为动态（`gwlist` 查 960 的 next）。
2. **LOW-1**: 补 `test_on_error.expected` 尾换行（与 8 项同样 `\n`），使 strict 比对 113/113 归零假失败。

### 中期
3. 保持"探针→红灯→修复→转正"循环；本轮起断言已能沿机制链路失败（good）。
4. 建议把 `.expected` 尾换行纳入生成工具（若 .bas 套件由脚本生成 expected，应统一 `<<'EOF'` 或 printf 带 `\n`），根治此类假失败。

### 长期
5. 同 v175：考虑导出 `next86_gosub_peek_return_line()` 类内部状态查询，使测试可自我断言而不必模拟 g_curlin。

---

## 五、总结

**总体评价**: v175 的 MEDIUM-1（gosub 断言打空）已**真闭环**——RETURN→take_tail→tail 复合→clear 链路被 4 条断言实际驱动（RETURN rc=961 宿主续点、B=7 tail body 执行、post-tail 正常、栈深度 0），且测试自带行/栈清理；LOW-1 注释到位。8 项 expected 尾换行修复使严格比对从 104/113 提升到 112/113。**0C/0H 连续三轮**。残留：gosub 测试隐式依赖前序测试行号（MEDIUM-1，测试独立性）与 `test_on_error.expected` 遗漏（LOW-1，承诺 9 项实修 8 项）。**建议：MEDIUM-1 顺手加固（自足行号）后即可合并**；无功能缺陷。

**关键发现**: **0 CRITICAL / 0 HIGH / 1 MEDIUM / 2 LOW**；3615 单测、113/113 .bas（1 假失败）、0 警告、0 孤儿。
**建议**: 可合并。补 `test_on_error.expected` 尾换行 + gosub 测试自足后，strict 全绿。

---

## 六、指标追踪

| 指标 | v175 | v176 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3611 / 3611 | **3615 / 3615** | +4 |
| 测试函数 | 973 | **973** | 保持 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas strict 全等 | 104/113（9 假失败） | **112/113**（仅 test_on_error） | +8 |
| .bas 有效通过 | 113/113 | **113/113** | 保持 |
| gosub tail host 断言 | 打空 | **B=7/961/depth=0 真闭环** | ✅ M1 闭环 |
| 测试独立性 | — | **g_curlin=960 依赖前序测试** | ❌ 新 MEDIUM |
| CRITICAL / HIGH | 0 / 0 | **0 / 0** | 保持可合并 |

**注释**: 构建/单测 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 全量字节比对（\r 归一、stdin=/dev/null）；`test_on_error` 差异经 `cat -A` 确认仅缺 `\n`。承诺比对：`git show 9edcd8d --stat` 实含 8 个 .expected。仅更新本报告，未改源码/测试、未提交。