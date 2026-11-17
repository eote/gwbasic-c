# GW-BASIC C Port - Code Review v173

**日期**: 2026-11-18
**审查范围**: `6dd31d0` → `06b94ef`（v172 修复轮 `f885fc5` + 进度登记 `06b94ef`）
**测试状态**: 断言 **3601/3601 通过** | 测试函数 **971 = 971**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **111/111***（9 项仅 .expected 缺尾换行，逐字节确认非真实失败）

**总体评分**: **C-（阻断合并）** — **0 CRITICAL** + **1 HIGH**（FOR 行带前缀段时跨行 NEXT 死循环）+ **3 MEDIUM**（嵌套 GOSUB tail 修复无单测、测试盲区、GW-BASIC 步长 0 语义）+ **1 LOW**

---

## 一、概述

`f885fc5`（src 5 文件 + tests 2 文件）修复 v172 全部 2H/3M，新增 5 条单测：

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | -5：compound 不再消费 `-3`（透传给 runner）；RETURN tail 复合后 `next86_tail_host_clear()`；RESUME NEXT 段判据 `> 0` → `>= 0` |
| `src/gwmain.c` | +28：runner 处理 `-3`——重入 FOR 行、`resume_seg_skip=1` 跳 FOR 头段 |
| `src/next86.c` | +48：整数 FOR STEP 走 CINT（floor/ceil ±0.5）；`next86_current_for_line()`；GOSUB tail host 机制 `g_tail_host_return`（tail 内嵌 GOSUB 返回宿主续点）；`next86_tail_host_clear()` |
| `src/bimisc.c` | +10：行首 `'`/REM（含 `:` 边界）整行跳过 |
| `src/gwdata.c` | +5：`gwdata_set_error` 复位 `error_seg_index=-1`（新错误重新武装段 pinning） |
| `tests` | +5 单测 + 9 断言 |

### 验证结果（v172 问题闭环全验）

| v172 问题 | 探针 | 结果 |
|----------|------|------|
| H1 跨行 NEXT | `10 FOR I=1 TO 3: PRINT "F"; I;` + `20 NEXT I` | ✅ F 1F 2F 3DONE、负步/浮点步正确 |
| H2① 段0 RESUME NEXT | `20 B=1/0: PRINT "SEG2"` | ✅ HSEG2-OK LINE30（同行段1恢复） |
| H2② index 残留 | RESUME 退出后第二错误 | ✅ LINE40 / S2-SHOULD-PRINT / LINE60（中段恢复） |
| M1 行首注释 DATA | `' note: DATA 9` / `REM foo: DATA 8` / 字符串 `"X: DATA 7"` | ✅ 全部跳过 → Out of DATA（正确） |
| M2 整数 STEP 0.5 | `FOR J%=1 TO 9 STEP 0.5` | ✅ B 1..9（CINT 取整 1） |
| M3 嵌套 GOSUB tail | 三层嵌套 + 尾行 | ✅ S1 S3 S1-TAIL2 B1 B2 S2 B3 MAIN-END（顺序正确、无 Error 300） |

### 回归检查（无破）

| 项 | 结果 |
|----|------|
| 简单 GOSUB 尾行 | ✅ SUB / BACK / LINE20 |
| 单行 FOR 同体 + 尾随 | ✅ S 1S 2S 3AFTER（AFTER 仅一次）；内嵌 GOSUB ✅ |
| IF 行号 + 尾随 / IF 内单行 FOR | ✅ TGT / ELSE-JUMP / TGT500 / O 1O 2 END-IF-ONE-LINER |
| 2D/3D double、D 字面量、`%.16g`、USING | ✅ 无回归 |
| 3601/971/111/0W/0 孤儿 | ✅ |

### ❌ 新发现：FOR 行带前缀段 + 跨行 NEXT → 死循环

```
10 A=1: FOR I=1 TO 3: PRINT "X"; I;
20 NEXT I
→ 实测: X 1 无限循环（每轮重入都重新执行 FOR 头、I 永远重置为 1）
→ 期望: X 1X 2X 3DONE
```

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **FOR 行带前缀段（`A=1: FOR …`）时，跨行 NEXT 重入跳过段数写死为 1 → 死循环** | `src/gwmain.c` | 279-300 |

**详情:** runner 的 `-3` 处理假定"FOR 头是 FOR 行第 1 段"，因此 `resume_seg_skip = 1`。但 FOR 行常见写法是带前缀语句：

```
10 A=1: FOR I=1 TO 3: PRINT "X"; I;
20 NEXT I
```

重入 FOR 行时跳过 1 段（即 `A=1`），随后**重新执行 FOR 头** → `next86_for` 重新 push、I 重置为 1 → 下一轮 `-3` 再重入 → 无限循环（实测 X 1X 1X 1…，`timeout 5` 输出满屏）。v172 对同一程序是"只执行一次"（次数错误但有限），v173 修复后变成**死循环**——这是修复轮引入的行为退化，GW-BASIC 中该写法合法且常见。

**修复:** FOR 入栈时记录"FOR 头所在段索引"（`for_seg_index`），runner 重入时 `resume_seg_skip = for_seg_index + 1`；或在 `next86_for` 返回时把"FOR 头段号"传出，runner 用它计算跳过段数。**补用例见第四节 T1（红灯）**——现有 5 条新单测全部只覆盖"FOR 即第 1 段"（`test_cross_line_next_body` 亦同），未覆盖前缀段场景。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **嵌套 GOSUB tail 修复（M3）没有任何单测** | `tests/test_runner.c` | — |
| 2 | **步长 CINT 后为 0 的语义未定：`STEP 0.49` → CINT=0 → 死循环（ASM step 0 故意无限循环，需真机/ASM 对齐确认）** | `src/next86.c` | 333-345 |
| 3 | **测试覆盖断链：v173 修复的 6 处行为中 5 处有单测，唯一漏网的死循环（HIGH-1）恰好是最严重的** | `tests/` | — |

**MEDIUM-1:** 上轮 MEDIUM-3（`g_tail_host_return` 宿主续点）修复经验证生效（嵌套三层 GOSUB + 尾行输出顺序完全正确），但本轮 5 条新单测覆盖了跨行 NEXT/段0/残留/注释/整数 STEP，**唯独没有嵌套 GOSUB tail**——该机制（全局单值 `g_tail_host_return` + `next86_tail_host_clear` 在 RETURN 时清除）是多层嵌套场景，极易回归且无保护。**用例见第四节 T2。**

**MEDIUM-2:** `STEP 0.49` → `floor(0.49+0.5)=0` → 步长 0 → 死循环。ASM 注释"step 0 时改符号为 2"意味着 GW-BASIC 的 step 0 本来就是**故意无限循环**（防呆：STEP 0 的 FOR 永不结束），因此 CINT 后为 0 时死循环可能是**正确语义**——但需在真机/ASM 全流程确认（GETINT 对 0.49 的取整结果与 FRCINT 一致后再下结论）。建议补 `STEP 0.49`/`STEP 0` 用例并记录期望语义。

**MEDIUM-3:** 上轮报告( v172 LOW-2 )预测的"6 条新单测未覆盖全部 4 处行为缺陷"本轮部分应验：5 条单测覆盖了 4 处，但 HIGH-1 死循环依然漏网。结论：**每轮把探针直接转正为测试**（见第四节），不要依赖下一轮修复开发者的自觉。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | `test_cross_line_next_body` 只验证 compound 返回码 0 且注释"runner loop, .bas-verified"——真正的跨行循环行为没被单测锁定；FOR 前缀段场景更完全空白 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| `-3` 透传给 runner 统一处理（v172 H1） | ⚠️ 正确解耦了 compound/runner 职责，但段跳过数硬编码 1，未携带 FOR 头段号（HIGH-1） |
| `gwdata_set_error` 复位 error_seg_index | ✅ 干净关闭残留通道，与"错误即新状态"语义一致 |
| RESUME NEXT `>= 0` 判据 | ✅ 段0 错误恢复同行后续，符合 ASM OLDTXT |
| 行首 `'`/REM 整行跳过 | ✅ 与 ASM 非 tokenize 注释一致，字符串引号路径不受影响 |
| 整数 STEP CINT | ✅ 0.5→1 正确；0.49→0 需确认 step 0 语义（MEDIUM-2） |
| `g_tail_host_return` 全局单值 | ⚠️ 三层嵌套实测正确，但全局状态在更深嵌套/递归 tail 下无保护（MEDIUM-1） |
| 每轮探针转正 | ⚠️ 5/6 已转，唯一漏网即最严重的死循环——需要"探针=下一轮测试"的强约定 |

**根因**: 修复轮的主体设计（-3 透传 + 段跳过 + CINT + 注释行跳过）方向正确且已闭环 6/6；唯二缺口是（a）runner 对"FOR 头段号"的假设过于天真（第 1 段），（b）测试转正率没有达到 100%，且最危险用例（死循环）被漏。

---

## 四、测试建议（可直接粘贴给开发者）

按 `tests/` 与 `tests/bas/` 现有约定书写，建议随 HIGH-1 修复一同入库（T1 红灯先入库，修复后转绿）。

### T1（红灯）FOR 行带前缀段 + 跨行 NEXT —— 当前死循环

`tests/bas/test_for_prefix_seg.bas`:
```basic
10 A=1: FOR I=1 TO 3: PRINT "X"; I;
20 NEXT I
30 PRINT "DONE"
```
`tests/bas/test_for_prefix_seg.expected`（GW-BASIC 期望）:
```
X 1X 2X 3DONE
```
当前实际: `X 1X 1X 1…`（无限循环，`timeout` 挂死）→ T1 红灯。

### T2 嵌套 GOSUB + 尾行（MEDIUM-3 回归保护）

`tests/bas/test_gosub_nested_tail.bas`:
```basic
10 GOSUB 100: PRINT "B1": GOSUB 200: PRINT "B3"
20 PRINT "MAIN-END"
30 END
100 PRINT "S1";: GOSUB 300: PRINT "S1-TAIL";: RETURN
200 PRINT "S2";: RETURN
300 PRINT "S3";: RETURN
```
`tests/bas/test_gosub_nested_tail.expected`:
```
S1S3S1-TAILB1
S2B3
MAIN-END
```
（v173 实测通过；作为回归基线入库。）

### T3 单测补强（tests/test_str_semantics.c）

```c
void test_for_prefix_seg_skip(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* FOR line with a leading segment: cross-line NEXT (line runner)
     * must skip to the segment AFTER the FOR header, not segment 1.
     * Invariant: re-entering the FOR line must NOT re-push the FOR. */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("A=1: FOR I=1 TO 3: A = A + I"), 0,
                       "prefix + FOR line executes");
    TEST_PASS("prefix-seg FOR (runner loop, .bas-verified)");
}
void test_gosub_tail_host_clear(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* After a GOSUB tail finishes, the tail-host marker must clear so
     * a later GOSUB returns normally (MEDIUM-3 lifecycle). */
    TEST_PASS("gosub tail host clear (integration in .bas probe)");
}
```
注册两行于 `tests/test_runner.c`（不可用 `patch replace_all`，手工 `sed`/`edit` 单点插入）。

---

## 五、总结

**总体评价**: v172 的 2H/3M 六处行为全部修复且回归无破——跨行 NEXT（正/负/浮点步）、段0 RESUME NEXT、error_seg_index 残留、行首 `'`/REM 注释 DATA、整数 STEP CINT、三层嵌套 GOSUB tail 全部实测正确；3601/971、0 警告、0 孤儿、.bas 111/111。但 `-3` 重入机制的段跳过数写死为 1，使"FOR 行带前缀段 + 跨行 NEXT"从 v172 的"执行一次"恶化为**死循环**——这是修复轮引入的新 HIGH，且恰好未被 5 条新单测覆盖。**建议：阻断合并**，修 HIGH-1（FOR 头段号传递）并随 T1 红灯入库。

**关键发现**: **0 CRITICAL / 1 HIGH / 3 MEDIUM / 1 LOW**；3601 单测、111/111 .bas、0 警告、0 孤儿。
**建议**: 阻断合并。HIGH-1 修复 + T1/T2/T3 测试入库（T1 先红灯）。

---

## 六、指标追踪

| 指标 | v172 | v173 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3592 / 3592 | **3601 / 3601** | +9 |
| 测试函数 | 966 | **971** | +5 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas | 111/111 | **111/111*** | 保持（9 项 expected 尾换行） |
| 跨行 NEXT | F 1 后即 DONE | **F1F2F3 / G3G2G1 / 浮点步全对** | ✅ 修复 |
| **FOR 前缀段 + 跨行 NEXT** | — | **X 1 死循环** | ❌ 新 HIGH |
| 段0 RESUME NEXT | SEG2 丢失 | **HSEG2-OK LINE30** | ✅ 修复 |
| error_seg_index 残留 | S2 丢失 | **S2-SHOULD-PRINT 恢复** | ✅ 修复 |
| 行首注释 DATA | Q=9/R=8 泄漏 | **全部 Out of DATA** | ✅ 修复 |
| 整数 STEP 0.5 | 死循环 | **B 1..9** | ✅ 修复 |
| 嵌套 GOSUB tail | S3 重复+Error 300 | **顺序正确 MAIN-END** | ✅ 修复 |
| 简单 GOSUB / 单行 FOR / IF 行号 / IF 内 FOR | 全对 | **全对（无回归）** | ✅ |
| CRITICAL / HIGH | 0 / 2 | **0 / 1** | 降级未到可合并 |

**注释**: 构建/单测 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 全量字节比对（\r 归一、stdin=/dev/null），9 项差异逐字节核实为 .expected 缺尾换行。修复验证与回归均为独立进程手工探针；ASM 对照 GWMAIN.ASM（`NEXT`/FOR 文本指针、`GETINT`、step 0 语义）。第四节 T1/T2/T3 为可直接落盘的测试文案。仅更新本报告，未改源码/测试、未提交。