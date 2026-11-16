# GW-BASIC C Port - Code Review v171

**日期**: 2026-11-17
**审查范围**: `5a7074b` → `d8aab53`（v170 修复轮 `378eef8` + 进度补遗 `d8aab53`）
**测试状态**: 断言 **3569/3569 通过** | 测试函数 **960 = 960**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **111/111 通过**

**总体评分**: **C-（阻断合并）** — **0 CRITICAL**（v170 的 REM/撇号致命项已闭环）+ **3 HIGH**（行余文本指针断裂、IF 行号+尾随报错、2D double 数组写入仍 float）+ **7 MEDIUM + 3 LOW**

---

## 一、概述

`378eef8` + `d8aab53`（12 文件，+388/-94）修复 v170 的 1C/6H，并新增 8 条单测：

| 文件 | 变更 |
|------|------|
| `src/gwmain.c` | -74：复合执行器迁移到 `gwsts_exec_compound`，gwmain 只留分发 |
| `src/gwsts.c` | +118：compound 执行器（REM/`'` 吞行尾、DATA 分支、IF 块吞并）；IF THEN/ELSE 分支改走 compound；g_valtyp 8 时 PRINT 用 `%.15g` |
| `src/gweval.c` | +69：VALTYP 最宽提升；single 运算/比较结果 round 到 float；数学函数按 g_valtyp 选 double 版；STR$ double 分支 |
| `src/next86.c` | +13：FOR 整数初值用 CINT 的 rounded 值 |
| `src/bimisc.c` | +31：READ 扫描支持行中 `: DATA` 段（含引号跟踪） |
| `src/biptrg.c` | +13：typed set 增加秩/界校验 |
| `tests` | +8 单测 + 19 断言；test_typesuffix 标签修正 |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ `CC=D:/myplace/program/mingw64/bin/gcc.exe`；0 警告 |
| 单测 | ✅ **3569/3569**（v170: 3550，+19） |
| 测试函数/注册 | ✅ **960 = 960**，0 孤儿 |
| .bas 目录全量 | ✅ **111/111**（含 89 条已登记项；差值与登记口径有关） |
| REM/撇号注释 | ✅ `REM COMMENT: PRINT "LEAK1"` → 仅 DONE；`' COMMENT: ...` 不再报错；`PRINT "A": REM TRAP: PRINT "LEAK3"` → 仅 A/DONE；`IF 1 THEN REM X: PRINT "Z"` → 不执行 Z。**CRITICAL 闭环** |
| IF 分支冒号 | ✅ `T1 T2`、`E5 E6` 均执行（v170 HIGH-1 闭环） |
| VALTYP 最宽 | ✅ `PRINT 1#/3` → 15 位；`double+0` 与 `0+double` 顺序无关（HIGH-2 闭环） |
| single 舍入 | ✅ `A#=1/3` → 0.333333343267441；`0.1+0.2=0.3` → EQ（HIGH-3 闭环） |
| double 数学 | ✅ `SIN(1#)=0.841470984807897`、`SQR(2#)=1.4142135623731`、`EXP(1#)=2.71828182845905`（HIGH-4 闭环） |
| STR$/1D double 数组 | ✅ `STR$(1.23456789012345#)` 与 `C#(1)`/`D#=C#(1)` 全 15 位（HIGH-5 部分闭环） |
| **2D double 数组写入** | ❌ `E#(1,2)=1.23456789012345#: PRINT E#(1,2)` → **1.23456788063049**（HIGH-5 残留） |
| FOR 整数初值 | ✅ `FOR I%=1.9` → 2,3；`-1.9` → -2；`1.4` → 1,2（HIGH-6 闭环） |
| typed set 秩/界 | ✅ 新增 `test_typed_set_bounds`（MEDIUM-2 闭环） |
| **单行 FOR** | ❌ `FOR I=1 TO 3: PRINT "I="; I;: NEXT I` → 仅 `I= 1` 即 DONE（应 1 2 3） |
| **GOSUB 续行** | ❌ `GOSUB 100: PRINT "BACK"` → 仅 SUB，BACK 丢失（RETURN 直接跳下一行） |
| **复合行内 RESUME NEXT** | ❌ `A=1: B=1/0: PRINT "AFTER-ERR"` → AFTER-ERR 不再输出（RESUME NEXT 跳过行余） |
| **IF 行号+尾随** | ❌ `IF 1 THEN 200: PRINT "NO"` → **Error at line 10**（GW-BASIC 应直接 GOTO 200） |
| **注释内 DATA** | ❌ `A=1: REM x: DATA 5` + `READ Q` → Q=5（应 Out of DATA） |
| **DATA 幻影项** | ❌ `DATA 3,4: PRINT "A","B"` + `READ A,B,C` → C=4（应 Out of DATA） |
| **D 指数 double 字面量** | ❌ `A#=1.23456789012345D0` → 1.23456788063049（按 single 解析） |
| PRINT USING double | ❌ `PRINT USING "###.############"; 1.23456789012345#` → 1.234567890122 |
| 裸 `PRINT RND` | ❌ 返回 0（预存；GW-BASIC 允许省括号） |

### v170 问题闭环

| v170 | 状态 |
|------|------|
| C1 REM/撇号注释 | ✅ 段首 `REM`/`'` 吞行尾；REM 内冒号不再拆分、不再执行 |
| H1 IF 分支冒号 | ✅ T1+T2、E5+E6 全执行（THEN 行号跳转仍保留） |
| H2 VALTYP 顺序 | ✅ 最宽提升、顺序无关 |
| H3 single 舍入 | ✅ 结果/比较均 round float |
| H4 double 数学 | ✅ SIN/SQR/EXP/LOG/TAN/ATN 全 double |
| H5 STR$/数组 | ⚠️ STR$/1D ✅；**2D 写入仍 float** |
| H6 FOR 整数初值 | ✅ CINT rounded 生效（1.9/-1.9/1.4） |
| M2 typed set 秩/界 | ✅ 有校验（1D 路径） |
| M4 typesuffix 标签 | ✅ `SHOULD NOT PRINT` 标签修正 + ERL |

### 正面确认

| 功能 | 结果 |
|------|------|
| REM/`'` 注释含冒号 | ✅ 3 种位置（行首/段中/IF 内）均不执行 |
| IF 真假分支多语句 | ✅ T1 T2 T3 E5 E6 全对 |
| double 直接表达式 | ✅ 15 位、`A#` 变量、ABS 全对 |
| single 语义 | ✅ EQ / 0.333333343 |
| FOR 类型后缀+初值 | ✅ I%/A#/负 STEP/1.9/1.4 |
| ERL 生命周期 | ✅ 不受复合执行器回归影响（ERR=11 ERL=20） |
| 3569 单测/111 .bas/0 警告/0 孤儿 | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无（v170 的 CRITICAL 已在修复轮闭环）。

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **复合执行器丢失"行余"文本指针：单行 FOR / GOSUB 续行 / RESUME NEXT 同行后续 全部静默失效** | `src/gwsts.c` | 850-1080（exec_compound）、2659/2665（IF→compound） |
| 2 | **`IF … THEN <行号>: <尾随语句>` 报 "Error at line N"（GW-BASIC 应直接 GOTO，行余被丢弃）** | `src/gwsts.c` | 2650-2666 |
| 3 | **2D double 数组赋值仍走 float：`E#(1,2)=1.234…#` 存成 1.23456788063049** | `src/biptrg.c` / `src/gwsts.c` | set_array_element_multi（float 参数）/ 3124 |

**HIGH-1 详情:** compound 执行器按段把语句文本交给 `gwsts_exec_statement`，段与段之间没有文本指针承接。任何依赖"本行剩余文本"的语句语义全部断裂：

```
10 FOR I=1 TO 3: PRINT "I="; I;: NEXT I
20 PRINT "DONE"
   → 实测: I= 1 / DONE（仅 1 次迭代；NEXT 找不到循环体指针，直接回下一行）

10 GOSUB 100: PRINT "BACK"
   → 实测: 仅 SUB（GOSUB 压入的是本段文本指针，RETURN 后落在下一行）

10 ON ERROR GOTO 100
20 A=1: B=1/0: PRINT "AFTER-ERR"     (100 RESUME NEXT)
   → 实测: H ERR= 11 ERL= 20 / LINE30（AFTER-ERR 丢失）
```

ASM 对照：GWMAIN.ASM 的 FOR 保存文本指针（FOR 子句后的行余即循环体）、GOSUB 压入行余文本指针、RESUME NEXT 恢复 OLDTXT 定位到出错语句的下一条——三者都要求执行器按"文本指针"前进，而不是按段往返拷贝。这是 v169 引入、v170 又把 IF 分支全部改走 compound 后暴露面更大的架构级缺陷，单行 FOR / GOSUB 是 GW-BASIC 极常见写法，当前**静默输出错误结果**。

**修复:** compound 执行器改为文本指针推进（每段执行后把行余文本指针交给下一个语句类，FOR/GOSUB/RESUME NEXT 需要拿"本行剩余"指针）；或对单行 FOR 特判：把 FOR 段+行余绑成一个 mini-text（如 `FOR I=1 TO 3: PRINT I: NEXT I` 整体交给 FOR 处理）。补 TDD：单行 FOR、`GOSUB …: stmt`、分支内 GOSUB、复合行 RESUME NEXT。

**HIGH-2 详情:** `GWSIFS_IS_LINENUM` 要求 then_stmt 全为数字。`IF 1 THEN 200: PRINT "NO"` 的 then_stmt 是 `200: PRINT "NO"`（GW-BASIC 的 THEN 分支 = THEN 后本行全部文本，含冒号），不满足全数字 → 走 compound → 段 1 `200` 被当语句分派 → Syntax Error。GW-BASIC ASM `DOCOND`：CHRGTR 取 THEN 后**首个 token**，是 LINCON 即 `JMP GOTO`（行余被跳转丢弃，不报错）；`IF 0 THEN 100 ELSE 200: REM X` 同理。**修复:** 只检查 then/else 分支首 token 是否为行号（`atoi` + 首个 token 全数字，不要求整串）。

**HIGH-3 详情:** 1D 已接 typed set（v170 修复），但 `assign_value` 的 ndims>1 分支仍调 `biptrg_set_array_element_multi(const char*, const int*, float)`：

```
10 DIM E#(2,2): E#(1,2)=1.23456789012345#: PRINT E#(1,2)
   → 1.23456788063049（float 截断）
```

同时 v170 新增的 typed set 秩/界校验（MEDIUM-2）只保护 1D typed 路径，多维权衡完全没接。**修复:** multi-dim 赋值也按元素类型走 typed set（double 数组用 `double val`），并让 typed set 的秩/界校验覆盖多维护。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **READ 扫描把 REM/撇号注释内的 `: DATA` 当真实数据（新回归）** | `src/bimisc.c` | 新增行中 `:DATA` 扫描段 |
| 2 | **DATA 值解析未按 ASM"止于冒号"截断 → 幻影数据项（预存，被行中 DATA 放大）** | `src/bimisc.c` | parse_data_value |
| 3 | **D 指数 double 字面量按 single 解析：`1.23456789012345D0` 丢精度** | `src/gweval.c` | factor 字面量 |
| 4 | **`gwsts_exec_compound` DATA 分支是死代码且含 UB：`e` 首次为 NULL 即做指针减法** | `src/gwsts.c` | 850-1080 |
| 5 | **PRINT USING 完全忽略 double（字段仅 float 精度）** | `src/gwsts.c` | PRINT USING 数值格式化 |
| 6 | **`%.15g` 比 GW-BASIC double 少 1 位有效数字（16→15）** | `src/gwsts.c` | 2250 |
| 7 | **测试盲区：新增 8 条单测恰好未覆盖以上全部路径** | `tests/test_str_semantics.c` | 8 条新单测 |

**MEDIUM-1:** v170 在 READ 增加行中 `: DATA` 扫描（含引号跟踪），但**不跳过 REM/撇号**：

```
10 A=1: REM x: DATA 5
20 READ Q: PRINT "Q="; Q      → Q= 5（GW-BASIC: 注释不产生数据，应 Out of DATA）
10 ' note: DATA 8             → 同上
```

ASM 行内程序是 tokenized，REM 段内文本不 tokenize，`DATA` 不产生 token，自然不是数据；C 移植按纯文本扫描，需在扫描时跳过 `REM`/`'`。**修复:** 扫描 0 引号层时若命中段首 `REM`/`'` 直接吞到行尾再继续。

**MEDIUM-2:** `parse_data_value` 只按逗号/NUL 切值，不按冒号切：

```
10 DATA 3,4: PRINT "A","B"
20 READ A,B,C: PRINT A;B;C    → C= 4（"4: PRINT "A"" 被当值求值=4，而非 Out of DATA）
```

ASM `DATA:` 声明"TERMINATES ON ':': AND 0, ':' ONLY APPLIES IF QUOTES HAVE MATCHED UP"，即数据列表止于冒号（引号已配对时）。行首 DATA 时代已存在，v170 支持行中 DATA 后同样触发。**修复:** 值解析器遇到 0 引号层冒号即结束当前值/列表。

**MEDIUM-3:** `PRINT 2.5D2` / `1D3` 数值 OK，但超过 7 位有效数字的 D 字面量被按 single 字面量解析：

```
10 A#=1.23456789012345D0: PRINT A#   → 1.23456788063049（应 1.23456789012345）
```

GW-BASIC 中 `D` 后缀/`D` 指数是标准 double 字面量形式（BASCOM 语法 `1.234D0`），与 `#` 后缀等效。**修复:** factor 字面量支持 `D`/`d` 指数并置 VALTYP=8。

**MEDIUM-4:** 新 executor 的 DATA 分支条件 `if (e - seg_start >= 4 && ...)`：`e` 在赋值前被读取（首段 `e==NULL`，指针减法 UB；后续段 `e` 是上一段 trimmed 尾，恒 `< seg_start` → 条件永假）。因此"DATA 冒号后行尾延伸"声明从未生效——实际行为恰好靠死分支保持与 ASM 一致（DATA 止于冒号，见 MEDIUM-2）。**修复:** 删除该分支并消除对未初始化 `e` 的读取；DATA 语义按 ASM 落实为"冒号截断"。

**MEDIUM-5:** `PRINT USING "###.############"; 1.23456789012345#` → `1.234567890122`。PRINT USING 数值格式仍走 float（v159-170 均未接入 g_valtyp）。**修复:** USING 数值字段按 VALTYP 8 走 double 舍入/截断。

**MEDIUM-6:** `PRINT 1#/3` → `0.333333333333333`（15 位），GW-BASIC double 输出 16 位有效数字（`.3333333333333333`）。v170 LOW-3 陈述过但未修。**修复:** double 用 `%.16g`（并在表达式入口正确复位 g_valtyp，见 v170 MEDIUM-3）。

**MEDIUM-7:** 8 条新单测覆盖：REM compound、IF 分支 compound、VALTYP 顺序、single 舍入、math double、STR$/1D 数组、FOR 1.9、typed set bounds。以下路径全部无测试：单行 FOR、GOSUB 续行、复合行 RESUME NEXT、`THEN 行号: 尾随`、2D double 数组、D 字面量、DATA 幻影项、REM 内 DATA、PRINT USING double、裸 RND。整个 .bas 套件无单行 FOR/GOSUB（已 grep 确认）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | 裸 `PRINT RND` 返回 0（GW-BASIC 允许 `RND` 省括号 = RND(1)）；预存，v170 diff 未触及 |
| 2 | 数字 PRINT 格式：`PRINT .5` → ` 0.5`（GW-BASIC ` .5`，前导零未省略）；科学计数小写 `e`（`1e+10` vs `1E+10`）；预存 |
| 3 | 1D typed get 每次读取做两遍（先 float 校验再 typed 读），数组密集场景有性能余量；复合执行器对每段做动态分配 + 拷贝 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| REM/`'` 段首吞行尾 | ✅ 修复彻底，3 种位置全部正确 |
| IF THEN/ELSE 分支走 compound（v170 HIGH-1） | ⚠️ 语义正确了，但递归 compound 让"行余指针"的丢失更隐蔽（HIGH-1） |
| VALTYP 最宽提升 + single round | ✅ 顺序无关、比较正确；PRINT USING/STR$ 漏接是入口遗漏而非机制错误 |
| math 按 g_valtyp 选 double 版 | ✅ SIN/SQR/EXP/LOG/TAN/ATN 全对 |
| FOR 整数初值 CINT rounded | ✅ 1.9/-1.9/1.4 全对；但 next86 的 FOR 仍把"行余"当下一行（HIGH-1 同源） |
| 行中 `: DATA` READ 扫描 | ⚠️ 引号跟踪有，但 REM/撇号未跳过、值解析不按冒号截断（MEDIUM-1/2） |
| 字符串扫描执行器 vs tokenizer | ❌ v169 选择、v170 沿用；已暴露行余指针断裂（HIGH-1）、IF 行号截断（HIGH-2）两类本质缺陷 |
| 测试 +8 | ⚠️ 补上了上一轮清单，但整体仍停留在"每轮只测上轮结论"的覆写式覆盖 |

**根因**: 复合执行器用段扫描替代文本指针推进，导致所有"本行剩余文本"语义（FOR body、GOSUB/RETURN、RESUME NEXT）与行号形式（THEN 先行号）集体失配；double 求值链的数值入口（2D 数组、D 字面量、PRINT USING）仍有 float 残留。

---

## 四、改进建议

### 短期（阻断合并）
1. **HIGH-1**：改为文本指针推进或单行 FOR/GOSUB/RESUME NEXT 整体绑定行余文本；补单行 FOR、GOSUB 续行、复合行 RESUME NEXT 单测。
2. **HIGH-2**：`IS_LINENUM` 只判分支首 token（`atoi` + 首 token 全数字），`THEN 行号` 后行余按 GW-BASIC 丢弃；补 `THEN 200: stmt`、`ELSE 200: stmt` 用例。
3. **HIGH-3**：multi-dim 赋值走 typed set（double），并复用秩/界校验；补 2D/3D double 数组精度单测。
4. **MEDIUM-1/2**：READ 扫描跳过 REM/`'` 到行尾；parse_data_value 遇 0 引号层冒号结束；补 `REM x: DATA 5`、`DATA 3,4: PRINT` 幻影项用例。
5. **MEDIUM-3**：factor 支持 `D` 指数 double 字面量；补 `1.23456789012345D0` 单测。
6. **MEDIUM-4**：删除 DATA 死分支、消除 `e` 的 UB 读取。

### 中期
7. PRINT USING 数值字段接 g_valtyp（double 舍入）；`%.15g` → `%.16g` 并对齐 ASM 16 位格式。
8. 裸 RND 补默认参数；数字 PRINT 前导零/大写 E 对齐 GW-BASIC。
9. 测试翻转为"类型矩阵"：i/f/d × 标量/1D/2D/3D × 字面量(`#`/D) × PRINT/PRINT USING/STR$/数组 × 运算符，一次性封死 VALTYP 类回归。

### 长期
10. compound 执行器换 tokenizer/文本指针模型（可先在 FOR/GOSUB/RETURN/RESUME NEXT 四条语句上试点），逐步替代段扫描。
11. ASAN/UBSAN + fuzz（注释冒号、IF 分支、单行 FOR 深度嵌套、DATA 超长）。

---

## 五、总结

**总体评价**: v170 修复轮的 1C/6H 主体是真实的——REM/撇号、IF 分支 compound、VALTYP 最宽、single 舍入、math double、STR$/1D 数组、FOR 初值全部实测闭环，3569/3569 与 111/111 .bas 全绿、0 孤儿、0 警告。但"字符串扫描执行器"的先天缺陷在 IF 分支化的暴露面下显现：单行 FOR / GOSUB 续行 / 复合行 RESUME NEXT 静默丢失，`IF…THEN 行号: 尾随` 直接报错，2D double 数组与 D 字面量仍留 float 残尾。**建议：阻断合并**，先修 HIGH-1/2/3 与 DATA 两条（MEDIUM-1/2）。

**关键发现**: **0 CRITICAL / 3 HIGH / 7 MEDIUM / 3 LOW**；3569 单测、111/111 .bas、0 警告、0 孤儿。
**建议**: 阻断合并。短期清单见上；行余指针语义（HIGH-1）是执行器架构级修复，建议单独排期。

---

## 六、指标追踪

| 指标 | v170 | v171 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3550 / 3550 | **3569 / 3569** | +19 |
| 测试函数 | 952 | **960** | +8 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas | 87/87（登记） | **111/111（目录全量）** | ✅ |
| REM/撇号注释 | LEAK1 / Error at 10 | **全部不执行/不报错** | ✅ CRITICAL 闭环 |
| IF 分支冒号 | 仅 T1/E5 | **T1 T2 T3 E5 E6** | ✅ 闭环 |
| VALTYP/double 表达式 | 7 位、顺序依赖 | **15 位、顺序无关** | ✅ 闭环 |
| single 语义 | NE / 全 double | **EQ / 0.333333343** | ✅ 闭环 |
| math double | float | **SIN/SQR/EXP/LOG/TAN/ATN 全 double** | ✅ 闭环 |
| STR$/1D double 数组 | 1.23457 | **全 15 位** | ✅ 闭环 |
| **2D double 数组写入** | 1.23457 | **1.23456788063049** | ❌ 残留（HIGH-3） |
| FOR 整数初值 | 1.9→1 | **1.9→2、-1.9→-2** | ✅ 闭环 |
| typed set 秩/界 | 无 | **有（1D）** | ✅ |
| **单行 FOR** | 未测 | **仅 1 次迭代** | ❌ 新 HIGH-1 |
| **GOSUB 续行** | 未测 | **BACK 丢失** | ❌ 新 HIGH-1 |
| **复合行 RESUME NEXT** | 未测 | **行余跳过** | ❌ 新 HIGH-1 |
| **IF 行号+尾随** | 未测 | **Error at line 10** | ❌ 新 HIGH-2 |
| **注释内 DATA / 幻影项** | 未测 | **Q=5 / C=4** | ❌ 新 MEDIUM-1/2 |
| **D 字面量 / PRINT USING** | 未测 | **丢精度** | ❌ 新 MEDIUM-3/5 |
| CRITICAL / HIGH | 1 / 6 | **0 / 3** | 降级但未到可合并 |

**注释**: 单测/构建使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 以目录全量 111 个逐一比对（含 \r 归一化与 stdin=/dev/null）。所有 new 项均为独立进程手工探针实测；ASM 对照取 GWMAIN.ASM（IFS/DOCOND：THEN 行号 GOTO、DATA 终止于冒号、FOR/GOSUB 保存文本指针）。仅更新本报告，未改源码、未提交。