# GW-BASIC C Port - Code Review v163

**日期**: 2026-11-04
**审查范围**: `369842a` → `4a3bc7e`（修复轮 `4ec8eca`：v162 的 3 HIGH + 6 MEDIUM + LOW 处理）
**测试状态**: 单元断言 **3437/3437 通过**（+36，0 失败）| 编译警告 **0** | 已登记 .bas **86/86 一致** | 定义/注册 **926 = 926**（0 孤儿）
**修复轮结论**: SWAP 越界先读后写、数组赋值错误不写回、USING 换行打印区、写入路径 auto-DIM 多维均已闭环；严格字符串赋值的错误码也从“存字面量”改为 TMERR 13。但严格化实现**在失败时先清空目标变量**，造成原值丢失（新 CRITICAL）；`READ/INPUT` 的 raw 值仍先按表达式求值（可泄露变量值）；auto-DIM 的**读取路径**与秩不匹配仍别名。
**工作区状态**: 本次仅更新 `CODE_REVIEW.md`；`progress.md` 为审查前既有修改；未跟踪 `.reposman`、`GW-BASIC-READONLY-SOURCE`、`QWEN_SETUP.md`、`REVIEWER.md`、`tests/mini_u2.c`
**总体评分**: **C-（阻断合并）** — 本轮 1 项新 CRITICAL（失败赋值破坏原值）

---

## 一、概述

修复轮 `4ec8eca`（8 文件，+233/-25）处理 v162 清单：

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | +102：严格字符串赋值（TMERR 13）+ `assign_value_raw`；写入路径 auto-DIM nd；USING 换行行首；数组赋值错误不写回；SWAP 越界先读后写；陈旧 error 隔离 |
| `src/biptrg.c` | +19：`biptrg_ensure_array_nd()`（每维 10） |
| `src/gweval.c` | +7：字符串数组读取 auto-DIM nd |
| `src/bimisc.c` | +3：`READ` 改用 `assign_value_raw` |
| `tests/test_assign_strict.c` | +95：5 条赋值/auto-DIM/SWAP 越界单测 |
| `tests/test_biprtu_out2.c` | +21/-?：伪输出断言精确化；midline 改为精确列断言 |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3437/3437**，0 失败，0 警告 |
| 定义/注册 | ✅ **926 = 926**，0 孤儿 |
| 已登记 .bas | ✅ **86/86** |
| 写入路径 auto-DIM | ✅ `W 1 5`、`Vxy` |
| 显式 2D 字符串数组 | ✅ `Pabc → Qbac` |
| SWAP 越界 | ✅ `ERR=9`，`A$(1)="a"`、`C(1)=9` 保持 |
| 数组数值赋值溢出 | ✅ `C(1)` 保持 7，ERR=6 |
| 字符串数组函数错误 | ✅ 旧值保持，ERR=5 |
| USING 换行后打印区 | ✅ `Y` 第 15 列 |
| 行中打印区 | ✅ ` 1            X`（X 第 15 列） |
| 严格赋值错误码 | ✅ `A$=X`/`A$=5` → ERR=13 |
| 严格赋值**值保持** | ❌ `A$="old"` → `A$=5` 后变为空/0（`LEN=0`） |
| READ/DATA raw 字面量 | ❌ `DATA A$` → `"SECRET"`；`CHR$(65)` → `"A"` |
| INPUT raw 字面量 | ❌ 输入 `A$` → `"SECRET"` |
| 未 DIM 2D 读取 | ❌ `X=A(1,2)` 后 `A(1,1)=1:A(1,2)=5` → `5 5` |
| 1D 数组秩不匹配 | ❌ `DIM B(10): B(1,2)=5` → `B(1)=B(1,2)=5`（应 ERR 9） |
| 数值类型后缀 | ❌ `A#` → 1.23457；`A%` → 40000（既有） |
| `ERL` | ❌ 仍恒 0 |

### v162 问题闭环表

| v162 编号 | 状态 | 说明 |
|-----------|------|------|
| H1 字符串赋值回退 | ⚠️ | 错误码 13 正确；但失败时**清空原值**（新 CRITICAL） |
| H2 未 DIM 多维数组 | ⚠️ | 写入路径 ✅（1 5 / xy）；读取路径 ❌；1D 秩不匹配 ❌ |
| H3 数值类型后缀 | ❌ | 未处理（既有） |
| M1 USING 换行打印区 | ✅ | Y 第 15 列 |
| M2 控制台 isatty fallback | ❌ | 未处理 |
| M3 数值数组赋值错误 | ✅ | 旧值保持 |
| M4 字符串数组赋值错误 | ✅ | 旧值保持 |
| M5 SWAP 越界部分写入 | ✅ | 先读后写，旧值保持 |
| M6 弱断言 | ⚠️ | `eval_err` 已精确；`midline` 仍只捕获 USING 自身（未覆盖真实行中） |
| L1 陈旧 error 隔离 | ✅ | 各赋值分支 `clear_error` |
| L2 科学计数标志 | ❌ | 未处理（未在本轮范围） |
| L3 赋值中止 | ⚠️ | 数值/数组已中止；字符串“中止但已清空” |
| L4 扩容 OOM | ❌ | 未处理 |
| L5 2D 字符串 SWAP | ✅ | 完整索引 |

### 正面确认

| 功能 | 结果 |
|------|------|
| 严格 `A$=X` / `A$=5` | ✅ ERR=13（不再存 `"X"`/`"5"`） |
| 数值数组错误保护 | ✅ 旧值保持 |
| 字符串数组错误保护 | ✅ 旧值保持 |
| SWAP 越界保护 | ✅ 两操作数均保持 |
| USING 换行行首跟踪 | ✅ 后续尾逗号 14 列正确 |
| 写入路径 auto-DIM | ✅ 2D 数值/字符串均不再别名 |
| 显式 2D/SWAP | ✅ |
| 单测 | ✅ +5 条（共 926） |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **严格字符串赋值失败时先清空目标变量，原值丢失** | `src/gwsts.c` / `src/gwdata.c` | 3049-3073 / 82-84 |

**详情:** `assign_value` 在字符串标量分支先求值、再调用 `gwdata_alloc_variable(base_name, VT_STRING)`（该函数会清空已有变量的 `value`），最后才判断求值是否失败：

```c
char *str_val = gweval_eval_string(expr);
struct variable *v = gwdata_alloc_variable(base_name, VT_STRING);  /* 清空旧值 */
if (v) {
    if (str_val) { ... }
    else if (gwdata_get_error() != 0) result = ERROR;   /* 旧值已丢 */
    else if (!g_assign_allow_raw) { set_error(13); ... } /* 旧值已丢 */
```

复现：

```basic
10 ON ERROR GOTO 1000
20 A$="old"
30 PRINT "PRE[";A$;"]";LEN(A$)
40 A$=5
50 PRINT "POST[";A$;"]";LEN(A$)
60 A$="old2"
70 A$=CHR$(256)
80 PRINT "POST2[";A$;"]";LEN(A$)
```

实际：

```
PRE[old] 3
ERR= 13
POST[ 0] 0
ERR= 5
POST2[ 0] 0
```

即：类型不匹配或字符串函数错误后，**原值 "old"/"old2" 被清空（LEN=0，打印为 0）**。`CHR$(256)` 的旧值丢失在 v162 已存在；本轮严格化把 `A$=5` 也纳入同一路径，扩大了影响面。

**修复建议:** 先完成求值与错误判定，确认成功后再 `alloc/assign`；或在 `alloc` 前保存旧 `value.s`，失败时恢复。数组字符串路径已正确（不 alloc 标量），可复用其顺序。

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **`assign_value_raw` 仍先按表达式求值：READ/DATA/INPUT 文本可被替换为变量值** | `src/gwsts.c` | 2952-2956、3049-3073 / `src/bimisc.c:545` |
| 2 | **auto-DIM 多维只在写入路径生效；读取路径与秩不匹配仍别名** | `src/gweval.c` / `src/biptrg.c` | 940 / 41-51 |
| 3 | 数值类型后缀未实现（既有）：`#` 为 single、`%` 无整数溢出 | `src/gwsts.c` | 3089 |

**HIGH-1 详情:** `assign_value_raw` 只把 `g_assign_allow_raw` 置真，字符串分支仍先 `gweval_eval_string(expr)`，仅当求值失败才把原文当字面量。因此合法的字符串表达式会被执行：

```basic
10 A$="SECRET"
20 DATA A$,"A$",CHR$(65),hello,1+2
30 READ B$,C$,D$,E$,F$
40 PRINT B$;"|";C$;"|";D$;"|";E$;"|";F$
```

实际：`SECRET|SECRET|A|hello|1+2`
期望：`A$|A$|CHR$(65)|hello|1+2`（DATA/READ 为字面文本）。

同样，`INPUT B$` 输入 `A$` → `B$` 得到 `SECRET`（实测 `? B=[SECRET]`），属**输入内容被当作表达式执行**，可泄露/串改变量。

**修复建议:** raw 模式对字符串目标应直接分配字面量，不调用 `gweval_eval_string`；只有数值目标才解析数值。`DATA/READ/INPUT/LINE INPUT` 统一走此“字面字符串”入口。

**HIGH-2 详情:** 写入路径已按维建数组，但**读取路径未调用 `ensure_array_nd`**，且 `ensure_array_nd` 对已存在数组直接返回、不校验秩：

```basic
10 X=A(1,2)      ' 首次读取 → 仍按 1D 建 A(0..10)
20 A(1,1)=1
30 A(1,2)=5
40 PRINT "R";A(1,1);A(1,2)
```

实际 `R 5 5`（别名），期望 `R 1 5`。

秩不匹配：

```basic
50 DIM B(10)
60 B(1)=1
70 B(1,2)=5
80 PRINT "M";B(1);B(1,2)
```

实际 `M 5 5`；GW-BASIC 应报 `Subscript out of range (9)`。根因：

- `src/gweval.c:940` 直接 `biptrg_get_array_element_multi()`，未先 `ensure_array_nd`；
- `src/biptrg.c:41-42` `if (v) return v;` 不检查 `dimensions`/`dim_sizes` 与 `ndims` 是否一致。

**HIGH-3:** 沿用 v160 H4：`A#=1.23456789012345#` → ` 1.23457`；`A%=40000` → `40000`（无 Overflow 6）。`assign_value` 数值统一 `alloc_variable(..., VT_SINGLE)`（`gwsts.c:3089`）。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 打印区仅对“非 tty”生效；真实控制台仍走旧 fallback（v162 M2 未处理） | `src/gwsts.c` | 2138 |
| 2 | 新测试未覆盖关键失败语义：严格赋值未验旧值、raw 用 "hello"、auto-DIM 只测写入、midline 只捕获 USING 自身 | `tests/test_assign_strict.c` / `tests/test_biprtu_out2.c` | 13、26、43、115 |
| 3 | 严格赋值错误优先级：`A$=1/0` → 13、`A$=1E+39` → 13（GW-BASIC 可能先报 11/6） | `src/gwsts.c` | 3049-3073 |

**MEDIUM-1:** `if (!isatty(fileno(stdout)))` 分支外的真实控制台仍用 `biprtu_last_output_columns()`（仅 USING 自身列数）；行中尾逗号在控制台仍会错位。`ftell` 方案只在管道/文件重定向下正确。

**MEDIUM-2:** 本轮新增 5 条测试，但恰好绕过了三个缺陷：

- `test_assign_strict_scalar_tmerr` 只断言 `ERR=13`，未断言 `A$` 原值保持 → 漏掉 CRITICAL-1；
- `test_assign_strict_read_input_raw` 直接用 `assign_value_raw("S$","hello")`，`hello` 不是合法字符串表达式，无法暴露 `A$`/`CHR$()` 被求值 → 漏掉 HIGH-1；
- `test_assign_autodim_multidim` 只测写入路径，未测 `X=A(1,2)` 首读与 1D 秩不匹配 → 漏掉 HIGH-2；
- `test_biprtu_out2_midline_zone` 捕获的是单独的 USING 语句（列 0 起），并未验证“AB + USING”真实行中场景（注释与断言不一致）。

**MEDIUM-3:** `A$=1/0` 现在返回 13（Type mismatch），`A$=1E+39` 返回 13；若 GW-BASIC 先求值再类型检查，应分别为 11（除零）与 6（Overflow）。建议用真机/ASM 核对错误优先级后再定。

---

### 🟩 LOW

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | `g_assign_allow_raw` 为全局布尔，非可重入/无线程安全保护 | `src/gwsts.c` | 2950-2956 |
| 2 | `g_line_start` 仍未跟踪 CLS/LOCATE/INPUT 提示，之后打印区可能错 | `src/gwsts.c` | 32-36 |
| 3 | `parse_var_ref` 超过 8 个下标时静默截断（`num_indices < 8`） | `src/gwsts.c` | 2905 |
| 4 | `biptrg_set_array_string_multi` 先 `gwram_free` 再 `gwram_alloc`，OOM 时 dangling | `src/biptrg.c` | 407-408 |
| 5 | `vals` 扩容失败仍 `break` 静默丢值；`D` 指数按 single 解析 | `src/gwsts.c` / `src/gweval.c` | 754-762 / 455 |

### 观察项

1. `assign_value` 在每个分支开头 `gwdata_clear_error()`；正常语句边界无问题，但会掩盖“入口已有未处理错误”的异常调用场景。
2. `READ` 数值仍按表达式求值（`DATA 1+2` → 3）；若 GW-BASIC 要求 DATA 文本常量需核对。
3. 显式 2D 字符串数组、SWAP 越界、数组错误保护均保持正确。
4. 空格式、`$$/**`、符号字段、长串/长字段回归保持通过。

### 遗留问题

| 编号 | 状态 |
|------|------|
| v158 MEDIUM-1：`ERL` 恒为 0 | ❌ 仍存在（复测 `ERR=11, ERL=0`） |
| v158 LOW-1/2：gate 单测未断言 error_line；teardown 残留 | ❌ 未处理 |
| v157 obs-3..6 | ❌ 仍开放 |
| v162 H3：数值类型后缀 | ❌ 转入既有 HIGH-3 |
| v162 M2：控制台打印区 | ❌ 仍开放 |

---

## 三、架构评估

### 设计决策

| 决策 | 评价 |
|------|------|
| 严格字符串赋值（非字符串 RHS → 13） | ⚠️ 语义方向正确，但“先 alloc 后判错”破坏旧值（CRITICAL） |
| `assign_value_raw` 供 READ/INPUT | ❌ 只控制回退，未绕过求值 → 文本被当表达式执行 |
| `ensure_array_nd`（每维 10） | ✅ 写入路径正确；读取路径未用、秩不匹配未校验 |
| SWAP 先读后写 | ✅ 越界不再部分写入 |
| 数组赋值错误不写回 | ✅ 数值/字符串均保持旧值 |
| USING 换行 `note_newline` | ✅ 后续打印区正确 |
| `ftell` 列跟踪 | ⚠️ 仅非 tty；控制台仍错（MEDIUM-1） |
| 测试新增 5 条 | ⚠️ 方向正确，但恰好绕开本轮三个缺陷（MEDIUM-2） |

### 测试覆盖

| 功能 | 覆盖情况 |
|------|----------|
| 严格赋值错误码 | ✅ 单测（仅错误码） |
| 严格赋值旧值保持 | ❌ 无测试（实测丢失） |
| READ/INPUT raw 字面量 | ⚠️ 仅 `"hello"`；未测 `A$`/`CHR$()` |
| 写入 auto-DIM 2D | ✅ 单测 |
| 读取 auto-DIM 2D | ❌ 无测试（实测 5 5） |
| 1D 秩不匹配 | ❌ 无测试（实测别名） |
| SWAP 越界保持 | ✅ 单测 |
| 数组错误不写回 | ✅ 单测 |
| USING 换行后打印区 | ❌ 无测试（手工 Y 第 15 列） |
| 真实行中打印区 | ⚠️ 测试捕获单语句，未覆盖前缀 |
| 全部 .bas | ✅ 86/86 |

### 测试实现质量

| 决策 | 评价 |
|------|------|
| `test_assign_strict.*` 直接调用语句 API | ✅ 可复现 |
| 严格测试只验错误码 | ❌ 漏掉数据丢失 |
| raw 测试选 `hello` | ❌ 未触发表达式求值路径 |
| auto-DIM 测试只覆盖写入 | ❌ 漏读取/秩 |
| `eval_err_no_pseudo` 精确 strlen=0 | ✅ 改进有效 |
| `midline_zone` 捕获单语句 | ⚠️ 未验证真实行中 |

---

## 四、改进建议

### 短期（阻断合并，必须）
1. **CRITICAL-1**：字符串赋值先求值/判错，成功后才 `alloc/assign`；失败保持旧 `value.s`。补 `A$="old": A$=5`、`A$="old": A$=CHR$(256)` 的“旧值保持”断言。
2. **HIGH-1**：raw 模式对字符串目标直接字面量赋值，不调用 `eval_string`；补 `READ` `DATA A$`/`CHR$(65)` 与 `INPUT A$` 用例。
3. **HIGH-2**：`gweval.c:940` 读取前 `ensure_array_nd(name, num_indices)`；`ensure_array_nd` 对已存在数组校验维度并报 ERRBS(9)；补首读/秩不匹配用例。
4. **MEDIUM-1**：控制台也用统一输出列计数（或封装输出函数），不要只依赖 `ftell`。
5. **MEDIUM-2**：修正测试以覆盖上述失败语义；`midline_zone` 应在同一输出流中捕获“AB + USING”。

### 中期
6. 数值类型后缀（`%`/`#`/`!`）按 ASM 实现；HEX/SWAP/USING 随后按类型读写。
7. 核对严格赋值的错误优先级（`A$=1/0` 11 vs 13；`A$=1E+39` 6 vs 13）。
8. `parse_var_ref` 对 >8 维给出明确错误而非静默截断；字符串数组 set 的 OOM 安全。
9. 打印列跟踪覆盖 CLS/LOCATE/INPUT。

### 长期
10. ASAN/UBSAN + fuzz（raw 输入、秩不匹配、地址越界、非有限数）。
11. CI：赋值错误路径必须断言“错误码 + 旧值保持”；raw 输入必须有字面量测试。

---

## 五、总结

**总体评价**: 本轮把 v162 的 SWAP 越界、数组赋值错误、USING 换行、写入路径 auto-DIM、弱断言（eval_err）都修好了，方向正确；但严格字符串赋值的实现顺序引入**数据丢失型 CRITICAL**（失败赋值清空旧值），`assign_value_raw` 只做“回退”导致 READ/INPUT 文本仍被求值（变量泄露），auto-DIM 只覆盖写入路径。新增 5 条测试恰好没有覆盖这三个失败语义。

**关键发现**: **1 CRITICAL / 3 HIGH（1 项既有）/ 3 MEDIUM / 5 LOW**（另 v158 ERL 未修）。
**建议**: **阻断合并**。先修字符串赋值旧值保护与 raw 字面量入口，再补读取路径 auto-DIM/秩校验；随后用“错误码 + 旧值 + 字面量”三类断言加固测试。

---

## 六、指标追踪

| 指标 | v162 | v163 | 变化 |
|------|------|------|------|
| 单元断言通过 | 3401 | **3437** | ✅ +36 |
| 测试函数定义 / 注册 | 921 / 921 | **926 / 926** | +5 |
| 孤儿数 | 0 | **0** | 保持 |
| 编译警告 | 0 | **0** | 保持 |
| 已登记 .bas | 86/86 | **86/86** | 保持 |
| 严格赋值错误码 | `"X"`/`"5"`（错） | **ERR=13** | ✅ 部分 |
| 严格赋值旧值 | 保留 | **清空（LEN=0）** | ❌ 新 CRITICAL |
| READ/INPUT raw | 求值（未测） | **`DATA A$`→SECRET** | ❌ 新 HIGH |
| 写入 auto-DIM 2D | 别名 5 5 | **1 5 / xy** | ✅ 闭环 |
| 读取 auto-DIM 2D | 未测 | **5 5（别名）** | ❌ HIGH |
| 1D 秩不匹配 | 未测 | **5 5（应 ERR9）** | ❌ HIGH |
| SWAP 越界 | 清空有效值 | **旧值保持 + ERR9** | ✅ 闭环 |
| 数组赋值错误 | 写 0/字面量 | **旧值保持** | ✅ 闭环 |
| USING 换行打印区 | Y 第 11 列 | **Y 第 15 列** | ✅ 闭环 |
| 控制台打印区 | 未测 | **仍 fallback** | ❌ 未修 |
| 数值类型后缀 | `#` single / `%` 无溢出 | **同左** | ❌ 既有 |
| `ERL` | 恒 0 | **恒 0** | 未修 |
| CRITICAL / HIGH | 0 / 3 | **1 / 3** | ❌ 新 CRITICAL |

**注释**:
- 独立验证：WSL 原生 `gcc.exe`/`mingw32-make.exe` 干净重建 → 3437 单测 → 86 个已登记 .bas 全量比对 → v162 全部 H/M 探针复跑 → 新增 READ/INPUT raw、读取 auto-DIM、秩不匹配、赋值旧值探针。
- 无进程崩溃；CRITICAL-1 为数据丢失（旧值被清空），非内存破坏。
- 全程未写脚本、未改 `src/`/`tests/`、未 commit；仅更新本报告，等待审批。