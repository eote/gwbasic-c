# GW-BASIC C Port - Code Review v169

**日期**: 2026-11-14
**审查范围**: `58ec586` → `51a5ad5`（v168 修复轮 `d8e5b57` + v169 开发批 `31ff258`：ERL + 数值类型后缀）
**测试状态**: 断言 **3527/3527 通过** | 测试函数 **947 = 947**（0 孤儿）| 编译警告 **0** | 已登记 .bas **87/87 通过**
**总体评分**: **D+（阻断合并）** — **1 CRITICAL**（typed SWAP 数据损坏/不交换）+ **4 HIGH**（A# 单精度、FOR 后缀失败、CLEAR/RUN 错误状态残留、A! 未别名）

---

## 一、概述

| 提交 | 内容 |
|------|------|
| `d8e5b57` | v168 修复：SWAP 副本 free、f1 警告、40 字面量强断言 + ON ERROR e2e |
| `31ff258` | v169：ERL 变量、`%` 16 位整数、`#` VT_DOUBLE、DIM/auto-DIM 后缀推断、4 单测 + `test_typesuffix.bas`（87） |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ⚠️ 默认 gcc 经 `/mnt/d` argv0 找不到 `cc1`；用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 成功 |
| 单测 | ✅ **3527/3527**，0 失败 |
| 测试函数/注册 | ✅ **947 = 947**，0 孤儿 |
| 警告 | ✅ **0**（v168 M2 已修） |
| 87 .bas | ✅ **87/87** |
| `A%` 标量/数组取整与溢出 | ✅ `1.9→2`、auto-DIM `A%(1)=2`、`40000→ERR 6` |
| ERL 在 handler | ✅ `E1=20 ERR=11`；RESUME 后 `E2=20` |
| **A# 双精度** | ❌ `1.23456789012345#→1.23457`、`1/3→0.333333` |
| **FOR I%/A#** | ❌ `Error at line 10`，handler `ERR=0` |
| **typed SWAP** | ❌ `C#/D#` 不交换；`D#,C#(1)` 丢数组值；`F%,E%(1)` 写坏为 `1.07794e+09 / 0` |
| **CLEAR/RUN 后 ERL** | ❌ `CLEAR` 后 `E3=20`；第二次 RUN `E0=20`（ASM 清零） |
| **CLEAR 后 ON ERROR** | ❌ 仍跳旧 handler（输出 `HANDLER`） |
| **A! 别名** | ❌ `A=5`、`A!=0`、`A!=7` 后 `A=5`（应同名） |
| v168 SWAP 副本泄漏 | ✅ 两次 set 后 free（代码确认；1000 次循环由提交说明验证） |

### v168 修复闭环

| v168 | 状态 |
|------|------|
| MEDIUM-1 SWAP 副本泄漏 | ✅ `tmp1_copy/tmp2_copy` 写回后 `gwram_free` |
| MEDIUM-2 f1 警告 | ✅ 0 警告 |
| MEDIUM-4 测试加固 | ✅ 40 字面量强断言、ON ERROR e2e |

### 正面确认

| 功能 | 结果 |
|------|------|
| `%` 标量：round-half-away + Overflow 6 + 旧值保持 | ✅ 单测/手工 |
| `DIM A%()` 与 auto-DIM 整数数组 | ✅ 元素取整、溢出 6 |
| `A%` 基本读写/PRINT | ✅ |
| ERL 错误行读取（handler） | ✅ |
| ERL 无错误（全新进程） | ✅ 0 |
| SWAP 字符串数组（v168） | ✅ `[two][one]` |
| CLEAR/RUN 后字面量缓存 | ✅ `LIT` 保持 |
| IF OOM 传播（v168） | ✅ ERR=7 可捕获 |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **typed SWAP 仍按 `value.f` 读写：double 不交换、整数标量×数组写坏** | `src/gwsts.c` | 1274-1298 |

**详情:** SWAP 数值分支对所有标量都读取/写入 `va->value.f`，不区分 VT_INTEGER / VT_DOUBLE：

```c
f1 = va->value.f;                 /* VT_INTEGER / VT_DOUBLE 被当 float 低 4 字节 */
...
va->value.f = f2;                 /* 写 union 低 4 字节，double 高 4 字节保留 */
```

实测：

```basic
10 C#=1.5: D#=2.5: SWAP C#,D#: PRINT "SD=";C#;D#          → SD= 1.5 2.5（未交换）
20 DIM C#(2): C#(1)=1.5: D#=2.5: SWAP D#,C#(1)
30 PRINT "D=";D#;C#(1)                                     → D= 2.5 0（数组值丢失）
40 DIM E%(2): E%(1)=3: F%=4: SWAP F%,E%(1)
50 PRINT "MI=";F%;E%(1)                                     → MI= 1.07794e+09 0（数据损坏）
```

`SWAP A%,B%`（1/2、-1/2）碰巧通过，是因小整数位模式经 float 往返未变；这不保证其他整数（如 32767）或 OOM/FPU 模式。**修复**: 按 `t1` 选 `value.i/f/d` 或整 union `memcpy`；数组元素用返回 double/int 的类型化 get/set，不能统一转 float。

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **`A#` 只有 VT_DOUBLE 标签，求值/读取仍是 float 单精度** | `src/gweval.c` / `src/gwsts.c` / `src/biptrg.c` | 991、3180、433 |
| 2 | **FOR/NEXT 不接受后缀变量；`FOR I%` 直接失败且 ERR=0** | `src/next86.c` | 117-122、246-251、322-345 |
| 3 | **CLEAR/RUN 未按 ASM 重置错误状态：ERL 残留、ON ERROR 仍激活** | `src/gwdata.c` | 25-60 |
| 4 | **`A!` 未与 `A` 别名（GW-BASIC `!` 为默认单精度后缀）** | `src/gwsts.c` | 3157-3190 |

**HIGH-1 详情:** 赋值 `A#` 存 `(double)val`，但 `val` 来自 `gweval_eval_numeric()`（float）；读 `A#` 又返回 `(float)v->value.d`；数组 get 对 VT_DOUBLE 也 `return (float)data[index]`：

```basic
10 A#=1.23456789012345#
20 PRINT "D=";A#          → D= 1.23457
30 B#=1/3: PRINT "T=";B#   → T= 0.333333
```

`test_type_suffix_double` 只断言 `(int)1.5==1` 与 `type==VT_DOUBLE`，float 存入 double 同样通过，未验证精度。**修复**: 要么实现 double 求值链（parse/assign/print/array get 全 double），要么明确 `#` 仅类型标签、暂不支持双精度并修正文档/测试。

**HIGH-2 详情:** `next86_for` 变量名只接受 `alnum/_`（`NEXT` 仅 alpha），`%/#/!` 使解析在 `%` 处停下并返回 ERROR，且不 set_error：

```basic
10 ON ERROR GOTO 100
20 FOR I%=1 TO 3
30 PRINT "I=";I%
40 NEXT I%
100 PRINT "ERR=";ERR
```

实测 `ERR= 0`，无 I 输出；`FOR A#` 同样。即使解析修好，`next86` 仍用 `VT_SINGLE`/`value.f` 做 start/limit/step/compare，typed 循环值仍错。**修复**: 变量名允许 `$%#!`；按后缀取类型并用 `i/f/d`；语法错误设 SNERR/类型错误码。

**HIGH-3 详情:** ASM `CLEAR`/`RUN`（`BIMISC.ASM:245-248`）清零 `ERRLIN`、`ONEFLG`、`ONELIN`、`OLDTXT`；C 侧 `gwdata_init()` 只重置变量/current_line/堆，不动 error 状态：

- `ON ERROR` → `A=1/0` → handler `ERL=20`；`CLEAR` 后 `PRINT ERL` 仍 **20**（应 0）；
- 第二次 `RUN` 开始 `PRINT ERR;ERL` → `E0=0 20`（ERL 残留）；
- `10 ON ERROR GOTO 100 / 20 CLEAR / 30 A=1/0` → **仍跳 HANDLER**（ASM 中 CLEAR 已关 ON ERROR，应报未处理错误）。

**修复**: 在 `gwdata_init()` 清零 `error_code/error_line/error_text_ptr/on_error_line`（以及 old-text/handler 状态）；补 CLEAR/RUN 后 `ERL/ERR/ON ERROR/RESUME` 用例。

**HIGH-4 详情:** GW-BASIC 中 `A!` 与 `A` 是同一变量（`!` 为默认 single 后缀）：

```basic
10 A=5: PRINT "A=";A        → 5
20 PRINT "ABANG=";A!        → 0（应 5）
30 A!=7
40 PRINT "A2=";A            → 5（应 7）
```

`assign_value`/DIM/parse 只对 `$/%/#` 建类型，`!` 保留在名字里，形成独立变量；`A!(1)` 数组同样不与 `A(1)` 别名。**修复**: lookup/alloc/DIM/SWAP/FOR 统一把尾随 `!` 归一化为无后缀。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 测试盲区：double 只验类型标签；ERL 测试手工重置行号；无 FOR/SWAP/A!/CLEAR/RUN 用例 | `tests/test_str_semantics.c` | 281、309 |
| 2 | VT_DOUBLE 数组 get 返回 float（精度必然丢失） | `src/biptrg.c` | 433-437 |
| 3 | ERL 被硬编码为只读：`ERL=...` 创建无用变量；ASM 中 ERL 指向 ERRLIN | `src/gweval.c` / `src/gwsts.c` | 573-578 |
| 4 | `test_typesuffix.bas` 的 `SHOULD NOT PRINT` 实际会打印（RESUME NEXT 到 line120） | `tests/bas/test_typesuffix.*` | .bas:12 / expected:6 |

**MEDIUM-1:** `test_type_suffix_double` 只验证 `(int)1.5` 和 `VT_DOUBLE`；`test_erl_variable` 用 `gwdata_set_error(0,0,0)` 手工清行号，掩盖 `gwdata_init` 残留；缺少 typed FOR、typed SWAP、A!、CLEAR 后 ON ERROR、第二次 RUN ERL、auto-DIM 精度测试。**这是本轮 1C/4H 全部漏检的直接原因。**

**MEDIUM-2:** `biptrg_get_array_element()/..._multi()` 的 VT_DOUBLE 分支 `return (float)data[index]`；即使存储是 double，读到 float 已丢精度。

**MEDIUM-3:** `parse_factor` 对 `ERL` 特判读取 `gwdata_get_error_line()`，但赋值路径仍创建名为 `ERL` 的普通变量；`ERL=5` 不会改错误行，且不报错。ASM 中 `ERL` 描述符指向 `ERRLIN`，应确认是否允许赋值/应拒绝。

**MEDIUM-4:** 期望值与标签矛盾：程序 line110 溢出 → handler → `RESUME NEXT` 到 line120，确实打印 `SHOULD NOT PRINT`；测试通过但标签误导（若意图不打印应 `RESUME 130`）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | 整数标量 SWAP 依赖 float 位往返（32767/NaN 位型不保证）；应类型化 |
| 2 | SWAP 副本 alloc 失败时回退原槽指针，OOM 边缘仍可能 UAF |
| 3 | `next86_for` 解析失败返回 ERROR 但不设错误码（诊断困难） |
| 4 | `ERL` 残留只修 read 不修生命周期；error_text/OLDTXT 同样残留 |
| 5 | `A!` 数组/DIM/SWAP 同样未别名 |
| 6 | 默认 gcc 经 `/mnt/d` argv0 找不到 `cc1`，需 `CC=D:/...` 构建；环境问题 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| `%` 标量：round + Overflow6 | ✅ 正确；旧值保持 |
| DIM/auto-DIM 后缀推断 + 元素 FWRTN | ✅ 数值数组基本可用 |
| `#` = VT_DOUBLE | ❌ 仅类型标签，求值仍 float（HIGH-1） |
| ERL 特判读取 error_line | ✅ handler 内正确；❌ CLEAR/RUN 生命周期（HIGH-3） |
| typed SWAP | ❌ 仍 `value.f`，double/混合数据损坏（CRITICAL-1） |
| FOR/NEXT | ❌ 不接受后缀、仍 `value.f`（HIGH-2） |
| `!` 后缀 | ❌ 未归一化别名（HIGH-4） |
| 测试 | ❌ 只覆盖标签/类型，未覆盖精度/跨语句/生命周期 |

**类型后缀只加在 assign/parse/DIM/array set 四个入口，未贯穿 FOR/NEXT、SWAP、`!` 别名与 double 求值链**，这是本轮回归的根因。

---

## 四、改进建议

### 短期（阻断合并）
1. **CRITICAL-1**：SWAP 类型化：`i/f/d`/union memcpy；数组 get/set 返回对应类型；补 `C#/D#`、`D#/C#(1)`、`F%/E%(1)`、32767/-1 整数用例。
2. **HIGH-1**：实现 double 求值链或撤回 `#` 双精度声明；测试 `1.23456789012345#` 与 `1/3` 的 15+ 位输出。
3. **HIGH-2**：FOR/NEXT 变量名允许 `%#!`，按类型读写；补 `FOR I%`/`FOR A#`/`STEP` 用例；错误设码。
4. **HIGH-3**：`gwdata_init` 清零 error_code/error_line/error_text_ptr/on_error_line/OLDTXT；补 CLEAR/RUN 后 `ERL/ERR/ON ERROR/RESUME` 用例。
5. **HIGH-4**：lookup/alloc/DIM/SWAP/FOR 统一把尾随 `!` 归一化；补 `A/A!` 与 `A()/A!()` 用例。

### 中期
6. `biptrg` double getter 返回 double（或 typed value union）；消除 float 中间层。
7. ERL 按 ASM 变量表实现（读写 ERRLIN 或明确只读）。
8. 测试 `test_type_suffix_double` 改断言精度；`test_erl_variable` 用干净 init，不手工清行号；`test_typesuffix` 修正 SHOULD NOT PRINT 标签。

### 长期
9. 建立“类型后缀矩阵”测试：变量/数组/循环/SWAP/函数参数/PRINT USING 各类型组合。
10. ASAN/UBSAN + 长循环；CI 零失败/零警告。

---

## 五、总结

**总体评价**: v168 的 MEDIUM 修复到位（SWAP 副本释放、0 警告、测试加固），v169 的 `%` 整数与 ERL handler 读取也真实可用；但类型后缀只接入了一半入口：typed SWAP 直接损坏数据，`A#` 只有 double 标签没有 double 精度，FOR/NEXT 遇到后缀变量失败且 ERR=0，CLEAR/RUN 未清 ERL/ON ERROR，`A!` 未与 `A` 别名。3527 个单测全绿，但恰好避开了这些组合。

**关键发现**: **1 CRITICAL / 4 HIGH / 4 MEDIUM / 6 LOW**；3527 单测、87/87 .bas、0 警告。
**建议**: **阻断合并**。优先修 typed SWAP、double 精度、FOR 后缀、CLEAR/RUN 错误状态与 `!` 别名，再扩展类型矩阵测试。

---

## 六、指标追踪

| 指标 | v168 | v169 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3500 / 3500 | **3527 / 3527** | +27 |
| 测试函数 | 942 | **947** | +5 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 1 | **0** | ✅ 清零 |
| 86/87 .bas | 86/86 | **87/87** | +1 |
| v168 SWAP 副本泄漏 | 未释放 | **已 free** | ✅ |
| `%` 标量/数组 | 无 | **round + Overflow6** | ✅ |
| ERL handler | 0 | **20（正确）** | ✅ |
| ERL CLEAR/RUN | — | **残留 20** | ❌ HIGH |
| A# 精度 | single | **仍 single（1.23457）** | ❌ HIGH |
| FOR 后缀 | 无 | **ERR=0/失败** | ❌ HIGH |
| A! 别名 | 无 | **独立变量** | ❌ HIGH |
| typed SWAP | 无 | **数据损坏/不交换** | ❌ CRITICAL |
| 10k 字符串数组/长串 | 通过 | **通过** | 保持 |
| CRITICAL / HIGH | 0 / 1（既有） | **1 / 4** | ❌ 阻断 |

**注释**: 单测/87 .bas 使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe` 构建。类型/ERL/SWAP 为独立进程手工探针；`%` 与 ERL handler 为正面结果。仅更新本报告，未改源码。