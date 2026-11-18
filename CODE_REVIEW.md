# GW-BASIC C Port - Code Review v175

**日期**: 2026-11-19
**审查范围**: `afebac3` → `5b22dc0`（v174 修复轮 `b20b3ec` + 进度登记 `5b22dc0`）
**测试状态**: 断言 **3611/3611 通过** | 测试函数 **973 = 973**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **113/113***（9 项仅 .expected 缺尾换行，逐字节确认非真实失败）

**总体评分**: **B-（可合并，建议顺手修 MEDIUM-1）** — **0 CRITICAL + 0 HIGH + 1 MEDIUM**（T3 gosub 断言打空 + 测试状态泄漏）+ **2 LOW**

---

## 一、概述

`b20b3ec`（tests +37、gwbasic.h 注释 +6、run_bas_tests.sh 注释 +6）修复 v174 的 1M/2L。**src 无行为变更**，全部为测试断言补强与注释：

| 文件 | 变更 |
|------|------|
| `tests/test_str_semantics.c` | +37：`test_for_prefix_seg_skip` 补 6 断言（A=8/I=4 终值 + -3 续环驱动）；`test_gosub_tail_host_clear` 补 2 断言 |
| `src/gwbasic.h` | 注释：`for_seg_index` 仅 compound 路径可信（v174 LOW-1） |
| `run_bas_tests.sh` | 头注释：`STEP 0/0.49` 永不终止属 GW-BASIC 语义（ASM NT0STP，v174 LOW-2） |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ 0 警告 |
| 单测 | ✅ **3611/3611**（v174: 3602，+9 断言）；**973=973**，0 孤儿 |
| .bas 全量 | ✅ **113/113**（9 项仅 expected 尾换行） |
| 回归（src 无行为变化，.bas 全量确认） | ✅ 无回归 |

### v174 问题闭环

| v174 | 状态 |
|------|------|
| M1 T3 空转单测 | ⚠️ **部分闭环**：`for_prefix_seg_skip` 断言有效（能捕获 FOR 头重推类回归）；`gosub_tail_host_clear` 断言**打空**（详见 MEDIUM-1） |
| L1 for_seg_index 注释 | ✅ 已补（compound 路径可信说明） |
| L2 STEP 0 语义记录 | ✅ 已写入 run_bas_tests.sh 头注释 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **`test_gosub_tail_host_clear` 的两条断言打空：未执行 RETURN → `g_tail_host_return` 从未被设置/清除，断言在 host 标识泄漏时也会通过；且两次 `GOSUB 970` 压栈未 pop、`gwlist_add_line(970)` 泄漏到后续测试** | `tests/test_str_semantics.c` | `test_gosub_tail_host_clear` |

**详情（逐点取证）:**

```c
gwlist_add_line(970, "RETURN");
TEST_ASSERT_EQ_INT(gwsts_exec_compound("GOSUB 970: B = 7"), 970, "...");  /* 断言① */
TEST_ASSERT_EQ_INT(gwsts_exec_compound("GOSUB 970"), 970, "...");          /* 断言② */
```

1. **断言①** 验证的是"GOSUB 段返回目标行号 rc=970"——`next86_gosub_to_line` 的返回码与 `g_tail_host_return`（宿主续点）**无关**；且 tail `B = 7` 虽被 stash，但**测试从未执行 RETURN**，`next86_gosub_take_tail()`（`g_tail_host_return = e->text_ptr` 的唯一设置点，next86.c:565）从未被调用 → marker 全程为 0。
2. **断言②** `"GOSUB 970"` 同理由——即使实现存在"host marker 泄漏为残留值"的回归，`next86_gosub_to_line` 会改用 `return_line = g_tail_host_return` 压栈，但**返回码 rc 仍是 970**，断言照样通过。**两条断言都无法失败于 host 生命周期 bug**，与 v174 MEDIUM-1 的意图（锁定 `g_tail_host_clear` 机制）不符。
3. **状态泄漏**：两次 GOSUB 各自压栈（`g_gosub_sp` +2）而测试内**无 RETURN/clear**；`gwlist_add_line(970, "RETURN")` 向程序列表注入 970 行也未清理。若后续测试（如 `gwsts_on_gosub_syntax` 等）对 gosub 栈/程序列表状态敏感，会受影响——当前靠每个测试开头的 `gwdata_init()` 式重置掩盖（需确认 `gwinit_init()` 是否清 gosub 栈与程序列表）。

**修复建议:** 单测层真正驱动 host 生命周期需要沿 look take_tail→tail 复合→clear 调用链（即必须执行 RETURN 使其 tail 生效）；更实用的是：既然 T2 `.bas`（`test_gosub_nested_tail.bas`）已端到端锁定该机制（v174 已验证通过），**删除 `test_gosub_tail_host_clear` 的两条空断言**，改为注释引 T2——避免"有断言=有覆盖"的错觉。若坚持单测，则需先 `next86_clear_gosub_stack()`、构造"执行 RETURN 完成 tail"的调用序列后再用第二条 GOSUB 断言其 text_ptr（这需要导出 gosub 栈状态，见长期建议）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | `test_for_prefix_seg_skip` 的 -3 驱动是**直接调 `gwsts_exec_compound` 模拟**（body 文本 + NEXT），不经 `run_program_lines` 的 `resume_seg_skip = for_seg+1` 逻辑——能锁"FOR 头不重推/前缀不重执行"（A=8/I=4），但 runner 段的集成仍靠 T1 `.bas`；建议注释明确"单测层不覆盖 runner 路径" |
| 2 | `gwsts_exec_statement("ON ERROR GOTO 0")` 一行在 `test_gosub_tail_host_clear` 中与测试目标无关（残留调试行），且 `gwlist_add_line(970, ...)` 无生命周期管理 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| `for_prefix_seg_skip` 补真断言（A=8、I=4、-3/-3/0 三连） | ✅ 有效：A=1(前缀)+1(首体)+1+2+3=8 与 I=4 能捕获"FOR 头重推→I 重置"类回归 |
| `gosub_tail_host_clear` 补断言 | ❌ 打空：未触发 take_tail/clear 任一设置点，断言与 host 机制正交（MEDIUM-1） |
| src 注释（for_seg_index 可信范围、STEP 0 语义） | ✅ 到位且准确（run_bas_tests.sh 头注释对维护者友好） |
| 本轮"src 零变更 + 纯测试" | ✅ 符合 MEDIUM/LOW 修复的最小化原则，回归面为零 |

**根因**: v174 报告建议被理解为"给 T3 加断言即可"；两处断言中 `for_prefix_seg` 恰好驱动了部分真实状态（FOR entry 的 -3），而 gosub host 机制的唯一入口是 RETURN 执行路径——没有沿路径设计测试，断言自然落空。**教训：补断言必须沿被测机制的调用链设计，纯结果断言在全局状态机制上经常失效。**

---

## 四、改进建议

### 短期
1. **MEDIUM-1**: 删除 `test_gosub_tail_host_clear` 空断言并注释引 T2 `.bas`（推荐）；或沿 take_tail→tail 复合→clear 链路补真断言。加 `next86_clear_gosub_stack()` 防泄漏。
2. LOW-1: `test_for_prefix_seg_skip` 注释注明经 runner 的集成由 T1 `.bas` 承担。

### 中期
3. 保持"探针→红灯→修复→转正"循环；转正时优先选择能驱动目标机制最小状态的单测形态（或直接 .bas 集成），避免打空断言。

### 长期
4. 为 `next86_gosub_to_line`/`return_line` 机制考虑导出 g_gosub_stack 摘要查询（如 `next86_gosub_peek_return_line()`），使单测能断言压栈 text_ptr——避免此类"打空"断言反复出现。

---

## 五、总结

**总体评价**: v174 的 1M/2L 中，LOW-1/LOW-2 以注释精确闭环；MEDIUM-1（T3 空转）**部分闭环**——`for_prefix_seg_skip` 的 6 条断言真实有效（锁定 FOR 头不重推），但 `gosub_tail_host_clear` 的两条新断言**打空**（未触发 take_tail/clear 任何设置点，且断言结果与 host 标识正交，状态还泄漏至后续测试）。src 零行为变更，.bas 113/113、3611 断言、0 孤儿、0 警告。**功能上可合并（0C/0H）**；MEDIUM-1 属测试质量遗留：建议删除空断言改注释引 T2，或在单测中沿 RETURN→tail→clear 链路补真断言。

**关键发现**: **0 CRITICAL / 0 HIGH / 1 MEDIUM / 2 LOW**；3611 单测、113/113 .bas、0 警告、0 孤儿。
**建议**: 可合并；MEDIUM-1 顺手修（删空断言引 T2，或沿机制链路补断言）。

---

## 六、指标追踪

| 指标 | v174 | v175 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3602 / 3602 | **3611 / 3611** | +9 |
| 测试函数 | 973 | **973** | 保持 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas | 113/113 | **113/113** | 保持 |
| src 行为 | — | **零变更** | ✅ 最小化 |
| T3 for_prefix 断言 | 空转 | **A=8/I=4 有效** | ✅ 部分闭环 |
| T3 gosub host 断言 | 空转 | **打空（无关机制）** | ❌ 遗留 MEDIUM-1 |
| LOW-1/2 注释 | — | **已落实** | ✅ |
| CRITICAL / HIGH | 0 / 0 | **0 / 0** | 保持可合并 |

**注释**: 构建/单测 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 全量字节比对（\r 归一、stdin=/dev/null），9 项差异逐字节核实为 .expected 缺尾换行。`g_tail_host_return` 设置点取证自 next86.c:565（take_tail）/清除点 507、gwsts.c:1846。仅更新本报告，未改源码/测试、未提交。