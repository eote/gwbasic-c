# GW-BASIC C Port - Code Review v170

**日期**: 2026-11-15
**审查范围**: `51a5ad5` → `c092684`（v169 修复轮 `27a6ef8` + 补遗 `c092684`）
**测试状态**: 断言 **3550/3550 通过** | 测试函数 **952 = 952**（0 孤儿）| 编译警告 **0** | 已登记 .bas **87/87 通过**
**总体评分**: **D+（阻断合并）** — **1 CRITICAL**（复合执行器执行 REM/撇号注释后的代码）+ **6 HIGH**（IF 分支冒号、VALTYP 顺序、single 语义、double 数学、STR$/double 数组、FOR 整数初值）

---

## 一、概述

`27a6ef8` + `c092684`（13 文件，+461/-92 与 +28/-1）修复 v169：

| 文件 | 变更 |
|------|------|
| `src/gweval.c` | +97/-?：数值求值全 double、`g_valtyp`、`#` 字面量、single 字面量舍入 |
| `src/biptrg.c` | +74：typed get/set（i/f/d）、混合标量↔数组 |
| `src/gwsts.c` | +92：typed SWAP、PRINT double、ERL 只读、数组赋值 |
| `src/next86.c` | +78：FOR/NEXT 后缀变量、typed 初值/递增/比较 |
| `src/gwmain.c` | +76：冒号复合语句执行器 `gwmain_exec_compound`、THEN/ELSE 行尾吞并 |
| `src/gwdata.c` / `src/bimisc.c` | +29/+27：CLEAR/RUN/NEW 清零 ERR/ERL/ON ERROR、`A!` canon |
| `tests` | +5 单测 |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ `CC=D:/myplace/program/mingw64/bin/gcc.exe`；0 警告 |
| 单测 | ✅ **3550/3550** |
| 测试函数/注册 | ✅ **952 = 952**，0 孤儿 |
| 87 .bas | ✅ **87/87** |
| typed SWAP | ✅ `SD=2.5 1.5`、`MD=1.5 2.5`、`MI=3 4`、`MX=-1 32767` |
| A! 别名/数组/FOR | ✅ `A=5/A!=5/A2=7/B=2/F=1,2` |
| ERL CLEAR/RUN/ON ERROR | ✅ `E1=20/E2=20/E3=0`；CLEAR 后错误不再进 HANDLER；第二次 RUN `0 0`；`ERL=5→ERR 5` |
| FOR 基础 | ✅ `I%=1,2,3`、`A#=1,2,3`、负数 STEP `3,2,1` |
| `A#` 标量存储/PRINT 变量 | ✅ `D=1.23456789012345`、`T=0.333333333333333`、`ABS` 完整 |
| **REM/撇号注释** | ❌ `REM COMMENT: PRINT "LEAK1"` → 打印 LEAK1；`' COMMENT: PRINT "LEAK2"` → Error at line 10 |
| **IF 分支冒号** | ❌ `IF 1 THEN PRINT "T1": PRINT "T2"` → 仅 T1；`ELSE ... E5: E6` → 仅 E5 |
| **double 直接表达式** | ❌ `PRINT 1#/3 → 0.333333`；`PRINT 1.23456789012345# + 0 → 1.23457` |
| **single 语义** | ❌ `A#=1/3 → 0.333333333333333`；`IF 0.1+0.2=0.3 → NE` |
| **double 数学函数** | ❌ `SIN(1#)=0.841470956802368`、`SQR(2#)=1.41421353816986`、`EXP(1#)=2.71828174591064` |
| **STR$/double 数组** | ❌ `STR$=[ 1.23457]`、`C#(1)=1.23457`、`D#=C#(1)=1.23456788063049` |
| **FOR 整数初值** | ❌ `FOR I%=1.9 TO 3 → J=1,2,3`（应为 2,3） |

### v169 问题闭环

| v169 | 状态 |
|------|------|
| C1 typed SWAP | ✅ 全部通过（double/整数极值/混合数组） |
| H1 A# 双精度 | ⚠️ 标量存储/PRINT 变量 full；运算/数组/STR$/数学仍单精度 |
| H2 FOR 后缀 | ⚠️ 基本 `I%`/`A#`/负 STEP ✅；整数初值截断 ❌ |
| H3 CLEAR/RUN 错误状态 | ✅ ERL/ON ERROR/RUN/ERL 赋值全部闭环 |
| H4 A! 别名 | ✅ 标量、数组、FOR 全部别名 |
| 复合执行器 | ❌ REM/注释、IF 分支冒号（新） |
| 测试 | ⚠️ +5，但仍未覆盖上述新问题 |

### 正面确认

| 功能 | 结果 |
|------|------|
| typed SWAP | ✅ 双精度/整数/混合数组全精度 |
| A! 别名 | ✅ 标量、数组 `B!(1)`、`FOR I!` |
| ERL 生命周期 | ✅ handler/RESUME/CLEAR/RUN/只读赋值 |
| FOR 类型后缀 | ✅ `I%`/`A#`/负数 STEP |
| A# 标量存储 | ✅ 15 位 double 存储与 `PRINT A#` |
| ABS(double) | ✅ `1.23456789012345` |
| 3550 单测/87 .bas/0 警告 | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **复合执行器不识别 REM/撇号注释：注释后的代码会被执行或报错** | `src/gwmain.c` | 115-185 |

**详情:** `gwmain_exec_compound` 只按引号跳过 `:`，不识别 `REM`/`'` 注释：

```basic
10 REM COMMENT: PRINT "LEAK1"
20 PRINT "DONE"
```

实测打印 `LEAK1`（REM 后的 PRINT 被执行），随后才 DONE。另一形式：

```basic
10 ' COMMENT: PRINT "LEAK2"
20 PRINT "DONE"
```

实测 **Error at line 10**（合法的撇号注释行直接失败）。同理：

```basic
10 PRINT "A": REM TRAP: PRINT "LEAK3"
```

会在 A 之后执行 LEAK3。注释中的代码可能包含 PRINT/OPEN/GOTO 等任意副作用，属行为/数据破坏级回归。

**修复:** 段首（跳过空白）为 `REM` 或 `'` 时，直接吞到行尾、不拆冒号；`DATA` 也需核对是否同样延伸到行尾。补 `REM/撇号 + 冒号`、`PRINT: REM: PRINT`、IF 内 REM 用例。

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **IF THEN/ELSE 分支的冒号语句只执行第一条** | `src/gwsts.c` | 2546-2570 |
| 2 | **`g_valtyp` 取“最后因子”，不是表达式最宽类型；double 直接表达式打印 7 位** | `src/gweval.c` / `src/gwsts.c` | 78、527、1015-1017 / 2250 |
| 3 | **single 运算未按单精度舍入：`A#=1/3` 变全 double，`0.1+0.2=0.3` 变 NE** | `src/gweval.c` | 1024-1090 |
| 4 | **double 数学函数仍用 `sinf/cosf/tanf/atanf/expf/logf/sqrtf/floorf/truncf`** | `src/gweval.c` | 661-760 |
| 5 | **STR$ 与普通 double 数组仍走 float：`1.234...#` 变 `1.23457`** | `src/bistrs.c` / `src/gweval.c` / `src/gwsts.c` | 3124、993 |
| 6 | **FOR 整数初值最终被截断：`FOR I%=1.9` 从 1 开始（应 2）** | `src/next86.c` | 357、381 |

**HIGH-1 详情:** `gwsts` 的 IF dispatch 直接执行 `then_stmt`/`else_stmt` 单条语句，不再做冒号拆分：

```basic
10 IF 1 THEN PRINT "T1": PRINT "T2"       → 仅 T1
20 IF 0 THEN PRINT "F1": PRINT "F2"       → 无（false 正确）
30 IF 1 THEN PRINT "T3" ELSE PRINT "E3": PRINT "E4"  → 仅 T3
40 IF 0 THEN PRINT "T5" ELSE PRINT "E5": PRINT "E6"  → 仅 E5
```

GW-BASIC 中 THEN/ELSE 后冒号语句属于同一分支，应 T1+T2、E5+E6。`gwmain_exec_compound` 只吞并行尾，最终仍交给 `gwsts_exec_statement(then_stmt)`，后者不会执行第二条。**修复:** THEN/ELSE 分支调用共享的 compound 执行器（注意 THEN 行号跳转分支），补真假分支多语句用例。

**HIGH-2 详情:** `g_valtyp` 在 factor 中赋值，但 term/expression 不提升；最后一个因子决定 PRINT 精度：

```basic
PRINT 1#/3                    → 0.333333（应 15 位）
PRINT 1.23456789012345# + 0   → 1.23457
PRINT 0 + 1.23456789012345#   → 1.23456789012345（顺序依赖）
A#=1.23456789012345#: PRINT A# → 完整（变量路径设 valtyp 8）
```

数组读取/数学函数/ERR/ERL/RND/FRE 等也不设置/复位 valtyp，会沿用上一表达式的类型。**修复:** 按“最宽类型”提升（2<4<8），在每个 expression 入口复位，数组/函数返回时设置，PRINT 再消费。

**HIGH-3 详情:** 求值全用 double，没有在 single 表达式结果处 round 到 float：

```basic
A#=1/3                 → 0.333333333333333（GW-BASIC single 应为 0.333333343）
IF 0.1+0.2=0.3 THEN    → NE（GW-BASIC single 为 EQ）
```

`A#=1/3` 两个操作数都是默认 single，运算结果应先按 single 舍入再存 double；当前把 float 字面量 widen 后做 double 运算。**修复:** 用 VALTYP=4 时对每步/最终结果 round 到 float（或实现 MBF single 语义），double 仅在 valtyp 8 时保留。

**HIGH-4 详情:** 参数已 double，但函数仍单精度：

```
SIN(1#) = 0.841470956802368   （double 0.841470984807897）
SQR(2#) = 1.41421353816986    （double 1.414213562373095）
EXP(1#) = 2.71828174591064    （double 2.718281828459045）
```

`ABS` 已用 `fabs`（✅），其余 SIN/COS/TAN/ATN/EXP/LOG/SQR/INT/FIX 仍是 `sinf/sqrtf/...`。**修复:** 按 valtyp 选择 double 版本，或在 double 入参时统一调用 double 数学。

**HIGH-5 详情:** `STR$`/数组未接 double：

```
STR$(1.23456789012345#) = " 1.23457"
DIM C#(2): C#(1)=1.23456789012345#
PRINT C#(1)              = 1.23457
D#=C#(1)                 = 1.23456788063049（double of float）
```

`gweval.c:993/1006` 数组读取仍返回 float；`gwsts.c:3124/3136` 赋值仍 `float val`；`biptrg_get_array_element` VT_DOUBLE 返回 float。**修复:** 普通数组访问改用 typed get/set（或返回 double）；`STR$` 按 valtyp 8 用 `%.15g/16g`。

**HIGH-6 详情:** `next86_for` 先 round 得到 `rounded`（line357）并做范围检查，但末尾又 `v->value.i = (int32_t)start`（line381）直接截断：

```
FOR I%=1.9 TO 3 → J=1,2,3（GW-BASIC 应按 CINT 从 2 开始）
```

**修复:** 保存并使用 `rounded`；负数同样按 round-half-away；补 `1.9/-1.9` 用例。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 测试盲区：double 仅验存储/valtyp；FOR 仅 start=1；无 REM/IF 分支、PRINT 顺序、single、math、STR$/数组、FOR 1.9 用例 | `tests/test_str_semantics.c` | 323-400 |
| 2 | `biptrg_set_array_element_typed` 直接写 `data[index]`，无 bounds/rank 检查 | `src/biptrg.c` | 294-365 |
| 3 | `g_valtyp` 未覆盖数组读取/多数函数；`%.15g` 与 GW-BASIC double 16 位仍可能差 1 位 | `src/gweval.c` / `src/gwsts.c` | 78、993 / 2250 |
| 4 | `test_typesuffix.bas` 的 `SHOULD NOT PRINT` 实际会打印 | `tests/bas/test_typesuffix.*` | .bas:12 / expected:6 |

**MEDIUM-1:** `test_double_precision_chain` 只断言 `A#` 存储值与 `VALTYP==8`；不测 `PRINT 1#/3`、`A#=1/3`、`0.1+0.2=0.3`、`SIN/SQR`、`STR$`、`C#(1)`。`test_for_typed` 只用 start=1，无 1.9/负数。`test_bang_alias` 只标量（手工数组/FOR 已通过）。**复合执行器完全没有单测**，REM/IF 回归因此漏网。

**MEDIUM-2:** typed set 在 ndims==1 直接 `((double*)data)[indices[0]]`，未校验 index<size/dimensions；当前 SWAP 先 get（会校验），但公共 API 被其他调用时存在越界写风险。

**MEDIUM-3:** `g_valtyp` 未在数组读取、`ABS`/`SIN`/`SQR`/`FRE`/`RND`/`ERL` 等结果处设置；PRINT 的 `%.15g` 与 GW-BASIC double 16 位格式可能差末位。

**MEDIUM-4:** 程序 line110 溢出 → handler → `RESUME NEXT` 到 line120，确实打印 `SHOULD NOT PRINT`；测试通过但标签误导。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | `gwmain_exec_compound` 的 `if_block` 只要段内出现 THEN/ELSE 就吞行尾，未区分 REM/DATA/字符串（字符串已跳过，REM 未） |
| 2 | `DATA ...: ...` 是否应延伸到行尾未核对；当前会被拆分 |
| 3 | `g_valtyp`/`%.15g` 与 GW-BASIC 16 位格式差异 |
| 4 | `VAL`/`RND`/`FRE` 等函数 valtyp 可能沿用上一表达式 |
| 5 | `FOR I% STEP 0.5` 整数步长转换未验证（可能 0 → 无限循环） |
| 6 | 默认 gcc 经 `/mnt/d` argv0 找不到 `cc1`，需 `CC=D:/...` 构建（环境） |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| typed SWAP（i/f/d + typed array get/set） | ✅ 修复彻底；4 组用例正确 |
| A! canon 归一化 | ✅ 标量/数组/FOR 均别名 |
| gwdata_init/bimisc 清错误状态 | ✅ ERL/ON ERROR/RUN 全部正确 |
| FOR/NEXT 后缀类型 | ⚠️ 基本可用；整数初值末端截断 |
| 求值全 double + VALTYP | ❌ VALTYP 只记最后因子；single 未 round；math/数组/STR$ 未接 |
| 冒号复合执行器 | ❌ 无词法层，REM/`'` 被拆；IF 分支只执行第一条 |
| 测试 +5 | ⚠️ 覆盖 typed SWAP/A# 存储/FOR 基础/ERL/A!，但漏掉新执行器与 double 语义 |

**根因**: 类型/精度改造横跨 evaluator、PRINT、STR$、array、FOR、SWAP，但只完成了部分入口；复合执行器用字符串扫描替代词法分析，无法处理 REM/注释与分支语句列表。

---

## 四、改进建议

### 短期（阻断合并）
1. **CRITICAL-1**：复合执行器识别 `REM`/`'`（段首）并吞到 EOL；补 REM/撇号/PRINT:REM:PRINT/IF 内 REM 用例；核对 DATA。
2. **HIGH-1**：IF THEN/ELSE 分支执行 compound 语句列表（共享 helper），保持 THEN 行号跳转；补 T1+T2、E5+E6、F1/F2 用例。
3. **HIGH-2/3**：VALTYP 取最宽并在 expression 入口复位；single 结果 round 到 float；补 `1#/3`、`double+0`、`0.1+0.2=0.3`、`A#=1/3`。
4. **HIGH-4**：math 函数按 double 调用；补 SIN/SQR/EXP 精度断言。
5. **HIGH-5**：double 数组普通 get/set 改 typed；STR$ 输出 double 16 位。
6. **HIGH-6**：FOR 整数初值使用已 round 的 `rounded`；补 1.9/-1.9。

### 中期
7. typed set 增加 bounds/rank 校验；补 OOB 用例。
8. g_valtyp 覆盖数组/函数/ERR/ERL/RND/FRE；`%.15g` 与 ASM 16 位格式核对。
9. `test_typesuffix` 修正 SHOULD NOT PRINT 标签并加 ERL。
10. 复合语句执行器改为 tokenizer/词法驱动，不靠字符串扫描。

### 长期
11. 类型矩阵测试：变量/数组/循环/SWAP/函数/PRINT USING × i/f/d；single/double 比较语义。
12. ASAN/UBSAN + fuzz（注释冒号、IF 分支、double 数组、OOM）。

---

## 五、总结

**总体评价**: v169 的 1C/4H 中，typed SWAP、A!/ERL 生命周期、CLEAR/RUN 状态、FOR 基础后缀已真实修复；`A#` 标量存储与 `PRINT A#` 也能到 15 位，ABS 为 double。但 double 求值链只完成“存储”，VALTYP/single 舍入/math/STR$/数组仍缺失；新引入的冒号复合执行器使 REM/撇号注释代码被执行或报错，IF THEN/ELSE 分支的冒号语句只执行第一条。3550/3550 与 87/87 全绿，但测试恰好没有覆盖这些语义。

**关键发现**: **1 CRITICAL / 6 HIGH / 4 MEDIUM / 6 LOW**；3550 单测、87/87 .bas、0 警告。
**建议**: **阻断合并**。先修复注释/IF 分支执行器与 double 的 VALTYP/single/math/STR$/数组/FOR 初值，再扩展类型矩阵与复合语句测试。

---

## 六、指标追踪

| 指标 | v169 | v170 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3527 / 3527 | **3550 / 3550** | +23 |
| 测试函数 | 947 | **952** | +5 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| 87 .bas | 87/87 | **87/87** | 保持 |
| typed SWAP | 数据损坏 | **SD/MD/MI/MX 全对** | ✅ 闭环 |
| A! 别名 | 独立变量 | **标量/数组/FOR 别名** | ✅ 闭环 |
| ERL CLEAR/RUN | 残留 20 | **E3=0、RUN 0 0、ON ERROR 关闭** | ✅ 闭环 |
| FOR 后缀 | ERR=0 | **I%/A#/负 STEP 正确** | ⚠️ 初值 1.9 截断 |
| A# 标量存储 | single | **D/T 15 位、PRINT 变量 full** | ⚠️ 部分 |
| A# 直接表达式 | single | **P/Q 仍 7 位** | ❌ HIGH |
| single 语义 | double | **`0.1+0.2=0.3` NE、`A#=1/3` full** | ❌ HIGH |
| math double | float | **仍 float** | ❌ HIGH |
| STR$/double 数组 | float | **仍 1.23457** | ❌ HIGH |
| FOR 初值 | — | **1.9→1（应 2）** | ❌ HIGH |
| REM/注释 | 未测 | **LEAK1/Error at line 10** | ❌ 新 CRITICAL |
| IF 分支冒号 | 未测 | **T2/E6 丢失** | ❌ 新 HIGH |
| CRITICAL / HIGH | 1 / 4 | **1 / 6** | 阻断 |

**注释**: 单测/87 .bas 使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 构建。SWAP/A!/ERL/FOR/double/复合语句均为独立进程手工探针。仅更新本报告，未改源码。