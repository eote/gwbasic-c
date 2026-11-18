# GW-BASIC C Port - Code Review v174

**日期**: 2026-11-18
**审查范围**: `06b94ef` → `afebac3`（v173 修复轮 `46c7073` + 进度登记 `afebac3`）
**测试状态**: 断言 **3602/3602 通过** | 测试函数 **973 = 973**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **113/113***（9 项仅 .expected 缺尾换行，逐字节确认非真实失败）

**总体评分**: **B-（接近可合并）** — **0 CRITICAL + 0 HIGH + 1 MEDIUM**（T3 单测无真实断言）+ **2 LOW**

---

## 一、概述

`46c7073`（src 3 文件 + tests 4 文件 + run_bas_tests.sh）修复 v173 唯一 HIGH（FOR 前缀段跨行 NEXT 死循环），并入库 T1/T2/T3：

| 文件 | 变更 |
|------|------|
| `src/gwbasic.h` | +5：`for_entry.for_seg_index`（FOR 头所在 compound 段索引） |
| `src/next86.c` | +22：`next86_for` 记录 `for_seg_index`（取 `g_state->current_seg_index`）；`next86_current_for_seg()`；M2 语义注释（STEP CINT=0 → ASM NT0STP 故意无限循环，非 bug） |
| `src/gwmain.c` | +7：runner `-3` 重入用 `resume_seg_skip = for_seg + 1`（前缀段不重执行） |
| `tests/bas/` | +T1 `test_for_prefix_seg.bas/.expected`；+T2 `test_gosub_nested_tail.bas/.expected` |
| `tests` | +T3 2 单测（`test_for_prefix_seg_skip`、`test_gosub_tail_host_clear`）；run_bas_tests.sh 89 登记（+2） |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ 0 警告 |
| 单测 | ✅ **3602/3602**（v173: 3601，+1）；**973=973**（+2，0 孤儿） |
| .bas 全量 | ✅ **113/113**（+T1/T2；9 项仅 expected 尾换行） |
| **HIGH-1 前缀段死循环** | ✅ **修复**：`A=1: FOR I=1 TO 3: PRINT X: NEXT I` → X 1X 2X 3DONE |
| T1（入库红灯） | ✅ `test_for_prefix_seg` 输出与 `.expected` 一致 |
| T2（嵌套 GOSUB tail） | ✅ `S1S3S1-TAILB1 / S2B3 / MAIN-END` 与 `.expected` 一致 |
| **多前缀段** | ✅ `A=1: B=2: FOR …` → P 1P 2P 3、AB=1 2（前缀不重执行） |
| **嵌套跨行 FOR** | ✅ `FOR I…` / `FOR J…` / NEXT J / NEXT I → I1J1J2 I2J1J2；嵌套+前缀段 `A=1: FOR K…` 亦正确 |
| 跨行 NEXT（无前缀/负步/浮点步） | ✅ F1F2F3、G3G2G1、M1/1.5/2/2.5 |
| 单行 FOR 同体+尾随 / 内嵌 GOSUB | ✅ S1S2S3AFTER（AFTER 一次）、SUBSUBDONE2 |
| 段0 RESUME NEXT / index 残留 | ✅ HSEG2-OK LINE30 / S2-SHOULD-PRINT LINE60 |
| 整数 STEP 0.5 / STEP 0.49 语义 | ✅ B1..9；0.49→CINT 0→按 ASM NT0STP 死循环（语义注释，非 bug） |
| IF 行号+尾随 / IF 内单行 FOR | ✅ TGT/ELSE-JUMP/TGT500、O1O2 |

### 转正闭环（v173 第四节建议）

| 建议 | 状态 |
|------|------|
| T1 红灯 → .bas + .expected | ✅ 已入库且通过（修复后转绿） |
| T2 嵌套 GOSUB .bas 回归基线 | ✅ 已入库且通过 |
| T3 两条单测 | ⚠️ 已注册（973=973），但**均为 TEST_PASS 无真实断言**（见 MEDIUM-1） |
| 探针转正率 | ✅ 本轮 4/4（T1+T2+2 单测），上一轮唯一 HIGH 已带红灯闭环 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无（v173 唯一 HIGH 已闭环，全部回归探针通过）。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **T3 两条新单测只有 `TEST_PASS` 无真实断言——`test_for_prefix_seg_skip`/`test_gosub_tail_host_clear` 实际是空转** | `tests/test_str_semantics.c` | 新增 2 函数 |

**详情:** 两条单测仅调用 `gwsts_exec_compound(...)` 后断言返回 0 一次（+1 断言），或直接 `TEST_PASS`（0 断言）。`test_for_prefix_seg_skip` 调 `gwsts_exec_compound("A=1: FOR I=1 TO 3: A = A + I")`——该 compound 不含 NEXT，单测路径**不触发 runner 的 -3 重入**（死循环发生点），因此本轮修复的核心逻辑（`for_seg_index` + `resume_seg_skip = for_seg+1`）**没有被单测实际锁定**，只是占位。真防护仍依赖 .bas T1（✅ 已入库，这才是有效锁）。既然 T1/T2 .bas 已是有效集成锁，T3 单测至多是"冗余且误导"（让人误以为有断言覆盖）。**修复建议:** 要么给 T3 加真实断言（模拟 NEXT 返回 -3 后 runner 重入段跳过的行为），要么删除 T3 两条空转单测、在注释中引用 T1/T2 作为覆盖载体，避免"有单测=有覆盖"的错觉。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | `for_seg_index` 依赖 `g_state->current_seg_index`；若 FOR 头不在 compound 内执行（如独立行 FOR `10 FOR I=1 TO 3` 直接走语句执行器），current_seg_index 可能为陈旧值——实测独立行 FOR + 跨行 NEXT 均正确（G/J/M 循环通过），但建议注释该字段只对 compound 路径可信 |
| 2 | STEP CINT=0 → 死循环标注"非 bug"但**未真机对照**（仅 ASM 推断 NT0STP 语义）；建议在 run_bas_tests.sh 或文档中明确记录 `STEP 0/0.49` 属于 GW-BASIC 语义，避免后续维护者误判为回归 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| `for_entry.for_seg_index` + runner `skip = for_seg+1` | ✅ 彻底封闭死循环：前缀段、多前缀、嵌套跨行 FOR 全对 |
| T1 红灯先入库再修复 | ✅ 理想的 TDD 闭环（本轮红灯→转绿） |
| T2 嵌套 GOSUB .bas 基线 | ✅ 防回归有效（上一轮修复唯一无测试项已补齐） |
| M2 STEP 0 语义注释 | ✅ 与 ASM NT0STP 对齐；未真机验证（LOW-2） |
| T3 无断言单测 | ⚠️ 占位式空转，反而制造"已覆盖"假象（MEDIUM-1） |

**根因**: 修复主体（for_seg_index 传递）方向正确、验证彻底；剩余问题集中在"测试真实性"——T3 单测占位无断言，以及两处需要真机/文档确认的语义（STEP 0、for_seg_index 非 compound 路径陈旧值）。

---

## 四、改进建议

### 短期
1. **MEDIUM-1**: 给 T3 补真实断言（见下 T3'），或删除占位单测并在注释中显式指向 T1/T2。推荐补真断言——runner 重入段跳过是未来最可能的回归点，值得单测锁定。

### 中期
2. 在 `tests/bas/` 或文档记录 `STEP 0`/`STEP 0.49` 的 GW-BASIC 语义（ASM NT0STP），标注"非回归"。
3. `for_seg_index` 字段注释补充"仅 compound 路径可信"；若后续引入非 compound FOR 执行路径，需同步维护该字段。

### 长期
4. 继续推行"探针→红灯→修复→转正"循环；下一轮重点验证本轮未覆盖的新边界（如 FOR 跨行 + GOSUB tail 组合、三段嵌套 FOR）。

---

## 五、总结

**总体评价**: v173 唯一 HIGH（FOR 前缀段跨行 NEXT 死循环）已彻底修复——多前缀段、嵌套跨行 FOR 均正确，T1 红灯转绿、T2 基线入库，全部 9 组回归探针通过，3602/973/0W/0 孤儿、.bas 113/113。本轮无功能缺陷，**首次自 v158 起 0 CRITICAL / 0 HIGH**；剩余问题仅为测试真实性（T3 空转单测）与两处语义确认项。**建议：MEDIUM-1 修复后可合并**（或至少删除占位断言以免误导）。

**关键发现**: **0 CRITICAL / 0 HIGH / 1 MEDIUM / 2 LOW**；3602 单测、113/113 .bas、0 警告、0 孤儿。
**建议**: 补 T3 真实断言（或删除占位单测）后即可合并；本轮为多轮以来首次无 HIGH/CRITICAL。

---

## 六、指标追踪

| 指标 | v173 | v174 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3601 / 3601 | **3602 / 3602** | +1 |
| 测试函数 | 971 | **973** | +2 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas | 111/111 | **113/113** | +T1/T2 |
| FOR 前缀段死循环 | X 1 死循环 | **X1X2X3、多前缀段、嵌套全对** | ✅ 修复（T1 红灯转绿） |
| 嵌套 GOSUB tail 基线 | 无 .bas | **T2 入库通过** | ✅ |
| 跨行 NEXT / 单行 FOR / RESUME / STEP | 全对 | **全对（无回归）** | ✅ |
| CRITICAL / HIGH | 0 / 1 | **0 / 0** | 首次清零 ✅ |

**注释**: 构建/单测 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 全量字节比对（\r 归一、stdin=/dev/null），9 项差异逐字节核实为 .expected 缺尾换行。修复验证与回归均为独立进程手工探针；ASM 对照 GWMAIN.ASM（`NT0STP` step 0 语义、FOR 文本指针）。仅更新本报告，未改源码/测试、未提交。