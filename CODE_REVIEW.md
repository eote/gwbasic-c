# GW-BASIC C Port - Code Review v167

**日期**: 2026-11-11
**审查范围**: `898b864` → `ef7b6ca`（修复轮 `328c568`：v166 的 2 HIGH + M1）
**测试状态**: 断言 **3495**（**3494 通过 / 1 失败**）| 测试函数 **940 = 940** | 编译警告 **1** | 已登记 .bas **86/86 通过**
**失败用例**: `tests/test_biprtu_out.c:95` — string array swapped（期望 `twoone`）
**总体评分**: **D（阻断合并）** — 2 CRITICAL（SWAP use-after-free、CLEAR/RUN 后 intern 悬空）+ 2 HIGH（IF OOM 仍静默、既有类型后缀）

---

## 一、概述

`328c568`（4 文件，+100/-16）：字面量 intern 改为可增长表（去掉 32×64 上限）、IF 字符串比较增加 `cv=-3` OOM 传播、循环测试不再逐次断言、新增“长字面量 500 次/40 个不同字面量”2 条测试。

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ⚠️ 默认 `/myplace`/`/mnt/d` argv0 使 gcc 找不到 `cc1`；用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 构建成功 |
| 单测 | ❌ **3494/3495**，唯一失败 `test_biprtu_out.c:95` 字符串数组 SWAP |
| 警告 | ❌ **1**：`tests/test_str_semantics.c:147` `f1` 未使用（零警告门禁破坏） |
| 断言计数 | ✅ 从 11488 回落到 **3495**（循环内断言已移除） |
| 长字面量/40 字面量单测 | ✅ 均通过 |
| 86 .bas | ✅ **86/86** |
| 手工 SWAP 字符串数组 | ❌ `[two][<二进制垃圾>]`（实测） |
| 手工 CLEAR 后重复字面量 | ❌ 第二次 `PRINT "LIT"` 输出 `Z` |
| 手工 IF 内 OOM | ❌ 输出 `DONE`，无 `ERR=7` |

### v166 问题闭环

| v166 | 状态 |
|------|------|
| H1 长字面量/32 项上限 | ✅ 可增长表；2 条测试通过 |
| H2 IF 中 OOM 静默 | ❌ **仍静默**（实测 `DONE`、无 ERR） |
| M1 断言膨胀 | ✅ 3495 |
| 新问题 | ❌ SWAP UAF（测试失败）、CLEAR/RUN 缓存悬空 |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **字符串数组 SWAP 的 use-after-free（唯一失败单测）** | `src/gwsts.c` / `src/biptrg.c` | 1209-1227 / 455 |
| 2 | **CLEAR/RUN/NEW 重置字符串堆但不重置字面量缓存，intern 指针悬空** | `src/bimisc.c` | 69、117、149 |

**CRITICAL-1 详情:** SWAP 先取两个元素指针，再写回：

```c
tmp1 = biptrg_get_array_string(bn1, idx1[0]);   /* 指向 data1 */
tmp2 = biptrg_get_array_string(bn2, idx2[0]);   /* 指向 data2 */
result = biptrg_set_array_string(bn1, idx1[0], tmp2); /* free(data1)! tmp1 悬空 */
result = biptrg_set_array_string(bn2, idx2[0], tmp1); /* 复制已释放内存 */
```

`biptrg_set_array_string` 在 `biptrg.c:455` `gwram_free(data[index])`。本轮 heap 布局变化后分配器复用 `data1` 块，第二次 set 从已释放内存复制：

```basic
10 DIM A$(2)
20 A$(1)="one"
30 A$(2)="two"
40 SWAP A$(1),A$(2)
50 PRINT "[";A$(1);"][";A$(2);"]"
```

实测：`[two][\xE0\x92\x57\xBE\x06\x02]`（第二元素为二进制垃圾）；单测断言 `twoone` 失败。该缺陷自 v164 数组字符串 SWAP 起存在，此前靠分配器“不立刻复用”偶然通过。

**修复**: 写入前把 tmp1/tmp2 复制到独立缓冲，或直接交换两个 `char*` 槽并管理所有权；禁止第二次 set 引用第一次 set 已释放的指针。

**CRITICAL-2 详情:** intern 表项 `ptr` 指向 `str_heap.data`，但 `bimisc_new()` / `bimisc_clear_with_args()` / `bimisc_run_setup()` 只执行 `str_heap.used = 0`，未调用 `gwdata_lit_cache_reset()`（仅 `gwdata_init` 调用）：

```basic
10 PRINT "LIT"
20 CLEAR
30 A$=STRING$(100,"Z")
40 PRINT "LIT"
```

实测：第二次输出为 `Z`（缓存 ptr 指向已被 A$ 覆盖的堆区），而不是 `LIT`。`RUN`、`NEW` 同样可触发；属字面量常量被改写的数据损坏。

**修复**: 所有 `str_heap.used=0` 处统一调用 `gwdata_lit_cache_reset()`（或封装 `gwdata_reset_string_heap()`）；补 CLEAR/RUN 后重复字面量测试。

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **IF 字符串比较的 OOM 仍静默：v167 `cv=-3` 修复未生效** | `src/gwsts.c` | 2414-2440 |
| 2 | 数值类型后缀未实现（既有）：`#` 为 single、`%` 无整数溢出 | `src/gwsts.c` | 3099 |

**HIGH-1 详情:** 将字符串堆填满后执行首次出现的新字面量比较：

```basic
10 ON ERROR GOTO 2000
20 S$=""
30 FOR I=1 TO 10000
40 S$=S$+STRING$(100,"A")
50 NEXT
500 IF C$="BRANDNEWLITERAL" THEN PRINT "MATCH"
510 PRINT "DONE"
2000 R=R+1
2010 IF R=1 THEN RESUME 500
2020 PRINT ERR
```

实测输出 `DONE`，无 `ERR=7`；说明 `cv=-3` 分支后的回退路径仍把条件当 false 或清除了错误。需继续定位 IF 条件求值回退，并补该场景测试（应 ERR=7，而非 DONE）。

**HIGH-2:** `A#=1.23456789012345#` → ` 1.23457`；`A%=40000` → `40000`；`ERL` 仍恒 0（`ERR=11, ERL=0`）。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 零警告门禁破坏：`float f1` 未使用 | `tests/test_str_semantics.c` | 147 |
| 2 | 测试缺口：无 CLEAR/RUN 字面量、无 IF OOM 传播；40 字面量测试未断言 `vf1` | `tests/` | — |
| 3 | 控制台打印区仍走 `isatty` fallback | `src/gwsts.c` | 2138 |
| 4 | intern 的 `text` malloc 失败发生在 `gwdata_alloc_string` 之后，泄漏 heap 指针且无缓存项 | `src/gwdata.c` | 153-170 |

**MEDIUM-2:** `test_many_distinct_literals` 中 `f1 = vf1 ? ... : 0.0f`，若 `F1` 未建立则 `drop` 为负、断言恒过；`test_long_literal_loop_heap` 未使用 `f1`。IF OOM 的 `cv=-3` 分支无测试，实测仍静默。

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | intern 查找 O(n)、表项/文本 malloc 无上限（大程序性能/内存） |
| 2 | `g_assign_allow_raw` 全局布尔，非可重入 |
| 3 | `g_line_start` 未跟踪 CLS/LOCATE/INPUT |
| 4 | `parse_var_ref` >8 下标静默截断 |
| 5 | 字符串数组每元素单独 `gwram_alloc(1)`；`D` 指数按 single |
| 6 | 环境：默认 `gcc` 经 `/myplace` 调用无法定位 `cc1`；需 `CC=D:/...` |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| intern 可增长表（无 32×64 上限） | ✅ 方向正确；2 条测试通过 |
| IF OOM `cv=-3` 传播 | ❌ 实测仍静默（HIGH-1） |
| 循环测试不逐次断言 | ✅ 计数回落 3495 |
| SWAP 字符串数组“get 指针 → set 复制” | ❌ UAF（CRITICAL-1） |
| 字符串堆重置分散在 bimisc | ❌ 与 intern 缓存生命周期脱节（CRITICAL-2） |
| 新增测试 | ⚠️ +2，但有未使用变量/弱断言 |

**测试覆盖缺口**: CLEAR/RUN 后字面量、IF OOM 传播、SWAP 数组 ASAN；`.bas` 已 86/86。

---

## 四、改进建议

### 短期（阻断合并）
1. **CRITICAL-1**：SWAP 字符串数组先复制 tmp1/tmp2（或交换槽指针），补输出断言 + ASAN。
2. **CRITICAL-2**：`bimisc_new/clear/run_setup` 统一 `gwdata_lit_cache_reset()`；补 CLEAR/RUN 重复字面量用例。
3. **HIGH-1**：修复 IF 条件回退路径，确保 OOM 后整条 IF 失败并保留 ERR=7；补堆满 + 新字面量测试。
4. 移除未使用 `f1`，恢复零警告。

### 中期
5. 数值类型后缀按 ASM 实现。
6. 控制台输出列统一计数。
7. intern malloc 失败先分配 text 再占 heap，避免泄漏。
8. 40 字面量测试断言 `vf1/vf2`，避免弱断言。

### 长期
9. ASAN/UBSAN + fuzz（SWAP 数组、CLEAR/RUN、OOM、长字面量）。
10. CI：零失败/零警告；FRE 稳定性与 .bas 纳入。

---

## 五、总结

**总体评价**: intern 无上限与循环断言计数已修正，长字面量/40 字面量测试通过，86 .bas 全绿；但出现 **1 个失败单测**（字符串数组 SWAP 的 use-after-free，实测第二元素为二进制垃圾），并发现 **CLEAR/RUN/NEW 重置堆却不重置 intern 缓存**（实测 `LIT`→`Z`）的数据损坏路径；IF 内 OOM 的修复实测仍未生效。另有 1 个编译警告。

**关键发现**: **2 CRITICAL / 2 HIGH / 4 MEDIUM / 6 LOW**；测试 3494/3495，1 警告。
**建议**: **阻断合并**。先修 SWAP UAF 与缓存生命周期，再修 IF OOM 传播，恢复测试全绿/零警告。

---

## 六、指标追踪

| 指标 | v166 | v167 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 11488/11488 | **3495 / 3494** | 计数修正；1 失败 |
| 测试函数 | 938 | **940** | +2 |
| 警告 | 0 | **1** | ❌ 零警告破坏 |
| 86 .bas | 86/86 | **86/86** | 保持 |
| 长字面量 intern | FRE -16318 | **单测通过** | ✅ |
| IF OOM 传播 | 静默 | **仍静默（DONE）** | ❌ 未修复 |
| SWAP 数组字符串 | 通过（侥幸） | **二进制垃圾，测试失败** | ❌ 新暴露 |
| CLEAR 后字面量 | 未测 | **LIT→Z（缓存悬空）** | ❌ 新 CRITICAL |
| 类型后缀/ERL | 未修 | **未修** | ❌ |
| CRITICAL / HIGH | 0 / 3 | **2 / 2** | ❌ 阻断 |

**注释**: 单测与 86 .bas 均在磁盘恢复后重新执行；SWAP/CLEAR/IF-OOM 为独立进程手工探针。构建使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe`（默认 `/mnt/d` argv0 找不到 cc1）。未改源码，仅更新本报告。