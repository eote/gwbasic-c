# GW-BASIC C Port - Code Review v168

**日期**: 2026-11-12
**审查范围**: `ef7b6ca` → `58ec586`（修复轮 `c63e84b`：v167 的 2 CRITICAL + 2 HIGH + M2/M4）
**测试状态**: 断言 **3500/3500 通过** | 测试函数 **942 = 942**（0 孤儿）| 编译警告 **1** | 已登记 .bas **86/86 通过**
**总体评分**: **B（本轮可合并）** — 本轮 0 CRITICAL / 0 新增 HIGH；仍有 1 项既有 HIGH（类型后缀）与 4 MEDIUM（SWAP 临时副本泄漏、1 警告、控制台打印区、测试缺口）

---

## 一、概述

`c63e84b`（5 文件，+93/-6）处理 v167 清单：

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | +35：SWAP 字符串数组 get 后立即 `gwram` 副本；IF `cv=-3` 硬错误跳过 THEN/ELSE dispatch |
| `src/bimisc.c` | +3：NEW/CLEAR/RUN 重置堆时统一 `gwdata_lit_cache_reset()` |
| `src/gwdata.c` | +12：intern 先分配 key 再占字符串堆（失败不泄漏） |
| `tests/test_str_semantics.c` | +45：CLEAR 后字面量、IF OOM 传播 2 条测试 |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ⚠️ 默认 gcc 经 `/mnt/d` argv0 找不到 `cc1`；用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 构建成功 |
| 单测 | ✅ **3500/3500**（+5），0 失败 |
| 测试函数/注册 | ✅ **942 = 942**，0 孤儿 |
| 警告 | ❌ **1**：`tests/test_str_semantics.c:147` `f1` 未使用 |
| 86 .bas | ✅ **86/86** |
| 手工 SWAP 字符串数组 | ✅ `[two][one]` |
| 手工 CLEAR 后重复字面量 | ✅ `LIT` / `LIT` |
| 手工 RUN 两次后重复字面量 | ✅ `LIT` / `LIT` |
| 手工 IF 内 OOM（真正填满，FRE<150 + 200B 新字面量） | ✅ **CAUGHT**（ERR=7 被 ON ERROR 捕获） |
| IF 数值 Overflow/除零 | ✅ `ERR=6` / `ERR=11`，AFTER1/AFTER2 正常 |
| 长字面量/40 字面量单测 | ✅ 通过 |

### v167 问题闭环

| v167 | 状态 | 说明 |
|------|------|------|
| C1 SWAP 数组 UAF | ✅ | get 后立即复制到 gwram；实测 `[two][one]` |
| C2 CLEAR/RUN/NEW 缓存悬空 | ✅ | bimisc 三处统一 `gwdata_lit_cache_reset()`；CLEAR/RUN 实测正确 |
| H1 IF 字符串 OOM 静默 | ✅ | 手工真正填满堆后 CAUGHT；`test_if_oom_propagates` 通过 |
| H2 类型后缀 | ❌ | 未处理（既有） |
| M2 测试缺口 | ⚠️ | +2 条（CLEAR/IF OOM）；仍无 SWAP 泄漏/端到端测试 |
| M4 intern key 泄漏 | ✅ | 先 malloc key，再占 str_heap |

> **更正 v167 探针**: v167 的 `DONE` 并非 IF 修复失败——`S$=S$+STRING$(100)` 先触发 “String too long”，且 A$=STRING$ 走非 str_heap 路径，未真正耗尽字符串堆。本轮用 `FRE(0)<150` + 200 字节新字面量强制 OOM，ON ERROR 正常捕获 ERR=7。

### 正面确认

| 功能 | 结果 |
|------|------|
| SWAP 字符串数组正确性 | ✅ `[two][one]` |
| CLEAR/RUN 后字面量缓存 | ✅ `LIT` 保持 |
| IF 字符串 OOM 传播 | ✅ ERR=7，CAUGHT |
| IF 数值错误传播 | ✅ ERR=6/11 |
| 数组 set 复制语义 | ✅ 旧元素安全释放 |
| 长字面量/40 字面量 intern | ✅ 单测通过 |
| 86 .bas / 3500 单测 | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无（v167 的 2 CRITICAL 均已实测闭环）。

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 数值类型后缀未实现（既有）：`#` 为 single、`%` 无整数溢出 | `src/gwsts.c` | 3099 |

`A#=1.23456789012345#` → ` 1.23457`；`A%=40000` → `40000`；`ERL` 仍恒 0（`ERR=11, ERL=0`）。非本轮引入，建议单独立项。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **SWAP 字符串数组每次泄漏 2 个 `gwram_alloc` 副本** | `src/gwsts.c` | 1181-1210 |
| 2 | 零警告门禁破坏：`float f1` 未使用 | `tests/test_str_semantics.c` | 147 |
| 3 | 控制台打印区仍走 `isatty` fallback | `src/gwsts.c` | 2138 |
| 4 | 测试缺口：无 SWAP 副本泄漏/ON ERROR 端到端测试；40 字面量测试弱断言 | `tests/` | — |

**MEDIUM-1 详情:** 为避免 UAF，SWAP 在 get 后复制：

```c
char *t1copy = (char *)gwram_alloc(t1len + 1);
if (t1copy) { memcpy(t1copy, tmp1, t1len + 1); tmp1 = t1copy; }
...
/* t1copy/t2copy 在两次 set 之后没有 gwram_free */
```

`biptrg_set_array_string()` 会再次 `gwram_alloc` 复制入数组，因此 t1copy/t2copy 在写回完成后已成为纯临时对象，却从未释放。每次字符串数组 SWAP 泄漏 2 块（malloc）；长循环（如 100k 次）会持续增长。若副本分配失败，`tmp1` 仍保留原指针，后续 set 仍可能 UAF（OOM 边缘）。建议在两次 set 后 `gwram_free(t1copy)`/`gwram_free(t2copy)`，并记录“是否复制”标志避免释放原指针。

**MEDIUM-2:** `tests/test_str_semantics.c:147` 的 `f1` 未使用，构建输出 1 条 `-Wunused-variable`，破坏项目零警告门禁。

**MEDIUM-3:** `if (!isatty(fileno(stdout)))` 之外的真实控制台仍用 `biprtu_last_output_columns()`；行中尾逗号在控制台仍可能错位。

**MEDIUM-4:** 新增 CLEAR/IF OOM 两条测试（直接 API）质量较好；但缺：

- SWAP 字符串数组副本泄漏测试（可用 alloc/free 计数或长循环 RSS）；
- IF OOM 的 `ON ERROR` 端到端测试（当前仅验证 rc/error）；
- `test_many_distinct_literals` 中 `F1` 未建立时 `drop` 为负、断言仍通过（弱断言）；
- 无 RUN（非 CLEAR）后字面量自动测试（手工已通过）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | intern 查找 O(n)、表项/文本 malloc 无上限（大程序性能/内存） |
| 2 | `g_assign_allow_raw` 全局布尔，非可重入 |
| 3 | `g_line_start` 未跟踪 CLS/LOCATE/INPUT |
| 4 | `parse_var_ref` >8 下标静默截断 |
| 5 | 字符串数组每元素单独 `gwram_alloc(1)`；`D` 指数按 single |
| 6 | `FRE(0)` 实际只统计字符串堆（不反映 gwram），SWAP 泄漏无法用 FRE 观测 |
| 7 | 环境：默认 `gcc` 经 `/myplace` 调用无法定位 `cc1`，需 `CC=D:/...` |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| SWAP get 后立即复制 | ✅ 消除 UAF；❌ 副本未释放（MEDIUM-1） |
| bimisc 三处统一重置 intern 缓存 | ✅ 正确；CLEAR/RUN 实测通过 |
| IF 硬错误跳过 THEN/ELSE dispatch | ✅ 正确；直接 API + ON ERROR 实测 ERR=7 |
| intern key 先分配 | ✅ 失败不占 str_heap |
| 新增 CLEAR/IF OOM 测试 | ✅ 覆盖 v167 两个 CRITICAL/HIGH；⚠️ 缺泄漏/端到端 |
| 零警告门禁 | ❌ 1 warning |

**测试覆盖**: CLEAR/IF OOM 已有；SWAP 副本泄漏、RUN 后字面量、ON ERROR 端到端、控制台仍缺。

---

## 四、改进建议

### 短期（合并后立即）
1. **MEDIUM-1**：SWAP 两次 set 后释放 t1copy/t2copy；补长循环 alloc/free 平衡测试。
2. 删除未使用 `f1`，恢复零警告。
3. 补 IF OOM 的 ON ERROR 端到端测试（FRE 控制留空 + 大新字面量 → CAUGHT）。
4. 40 字面量测试断言 `vf1/vf2` 均有效，避免负 drop 假通过。

### 中期
5. 数值类型后缀按 ASM 实现。
6. 控制台输出列统一计数；`FRE(0)` 明确语义（str_heap vs gwram）。
7. SWAP 副本失败路径避免回退到原指针；或改用槽指针交换避免复制。

### 长期
8. ASAN/UBSAN + fuzz（SWAP 数组、CLEAR/RUN、OOM、长字面量）。
9. CI：零失败/零警告；alloc/free 计数平衡；.bas 全量。

---

## 五、总结

**总体评价**: v167 的两项 CRITICAL 与 IF OOM 均已实测修复——SWAP 输出 `[two][one]`，CLEAR/RUN 后字面量保持 `LIT`，真正耗尽字符串堆后 IF 的 ERR=7 可被 ON ERROR 捕获；单测 3500/3500、86 .bas 全绿。剩余问题为 SWAP 临时副本未释放（MEDIUM）、1 个未使用变量警告，以及既有的数值类型后缀 HIGH。

**关键发现**: **0 CRITICAL / 1 HIGH（既有）/ 4 MEDIUM / 7 LOW**；3500 单测通过、86/86 .bas、1 警告。
**建议**: 本轮变更**可合并**；随后修 SWAP 副本泄漏与警告，类型后缀另案。

---

## 六、指标追踪

| 指标 | v167 | v168 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3495 / 3494 | **3500 / 3500** | ✅ +5，0 失败 |
| 测试函数 | 940 | **942** | +2 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 1 | **1** | ❌ 未清 |
| 86 .bas | 86/86 | **86/86** | 保持 |
| SWAP 数组字符串 | 二进制垃圾/失败 | **`[two][one]`** | ✅ 闭环 |
| CLEAR/RUN 后字面量 | `Z`/悬空 | **`LIT`/`LIT`** | ✅ 闭环 |
| IF 字符串 OOM | 静默 DONE | **CAUGHT，ERR=7** | ✅ 闭环 |
| SWAP 副本 | — | **每次泄漏 2 块** | ❌ 新 MEDIUM |
| 类型后缀/ERL | 未修 | **未修** | ❌ 既有 |
| CRITICAL / HIGH | 2 / 2 | **0 / 1（既有）** | ✅ 无新增 |

**注释**: 单测/86 .bas 使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 构建（默认 `/mnt/d` argv0 找不到 cc1）。SWAP/CLEAR/RUN/IF-OOM 为独立进程手工探针；IF OOM 使用 `FRE(0)<150` + 200 字节新字面量强制失败。仅更新本报告，未改源码。