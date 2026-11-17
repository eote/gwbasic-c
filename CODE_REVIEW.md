# GW-BASIC C Port - Code Review v172

**日期**: 2026-11-17
**审查范围**: `d8aab53` → `6dd31d0`（v171 修复轮 `3aa4479` + 登记 `12ba3e1` + 补遗 `6dd31d0`）
**测试状态**: 断言 **3592/3592 通过** | 测试函数 **966 = 966**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **111/111 通过***

**总体评分**: **C-（阻断合并）** — **0 CRITICAL** + **2 HIGH**（跨行 NEXT 回归；`error_seg_index` 生命周期——段0 判据与残留不复位）+ **3 MEDIUM + 2 LOW**

---

## 一、概述

`3aa4479` + `6dd31d0`（src 8 文件，~+280）修复 v171 全部 3H/7M，新增 6 条单测：

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | +150：compound 段索引跟踪；FOR 段吞并行尾 + `-3` 体指针恢复；GOSUB 段尾 stash；RETURN 尾随复合执行；RESUME NEXT 段索引重入；`GWSIFS_IS_LINENUM` 只判首 token；`%.16g`；2D+ 数组 typed set |
| `src/next86.c` | +67：`body_text` 单行 FOR 体（NEXT `-3` 续环）；`gosub tail` stash/take；step 数字跳过 |
| `src/bimisc.c` | +21：`parse_data_value` 冒号终止；READ 扫描跳过 `:REM`/`:'` 段 |
| `src/gweval.c` | +8：D 指数字面量 = double |
| `src/biprtu.c` | +15：USING 分数位 `%.*f` 提取；`src/gwdata.c`：`error_seg_index`/`resume_seg_skip` 初始化；`src/gwbasic.h`：3 字段 + `body_text` |
| `tests` | +6 单测 + 23 断言 |

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ `CC=D:/myplace/program/mingw64/bin/gcc.exe`；0 警告 |
| 单测 | ✅ **3592/3592**（v171: 3569，+23） |
| 测试函数/注册 | ✅ **966 = 966**，0 孤儿 |
| .bas 目录全量 | ✅ **111/111**（尾换行差异视同通过） |
| **单行 FOR（同体 NEXT）** | ✅ `FOR I=1 TO 3: PRINT I: NEXT I` → 1 2 3；STEP 1/负步/浮点步/内嵌 GOSUB 全对 |
| **跨行 NEXT + 单行 FOR 体** | ❌ `10 FOR I=1 TO 3: PRINT "F"; I;` + `20 NEXT I` → **仅 F 1 一次**（应 F 1 F 2 F 3） |
| **GOSUB 尾行** | ✅ 简单 `GOSUB 100: PRINT "BACK"` → SUB / BACK / LINE20 |
| **嵌套 GOSUB + 尾行** | ❌ 多层 tail 组合错乱（多余 S3、RETURN without GOSUB Error at 300） |
| **RESUME NEXT 段恢复（错误在段1+）** | ✅ `A=1: B=1/0: PRINT "AFTER-ERR"` → AFTER-ERR 恢复；多段（C2/D2）全恢复 |
| **RESUME NEXT 段0 边界** | ❌ `20 B=1/0: PRINT "SEG2"` → RESUME NEXT 跳下一行，**SEG2 丢失**（应同行段1 执行） |
| **error_seg_index 残留** | ❌ handler 用 RESUME 退出后，下一错误 RESUME NEXT 跳错段、静默丢语句 |
| **IF 行号 + 尾随** | ✅ `IF 1 THEN 200: PRINT "NO"` / `ELSE 400: stmt` / `THEN 500: REM` 全部正确跳转、无 Error |
| **2D/3D double 数组** | ✅ `E#(1,2)`/`F#(1,1,2)` 全 15 位精度 |
| **D 指数 / %.16g** | ✅ `1.23456789012345D0` 全精度、`2.5D2=250`；`1#/3` → 0.3333333333333333（16 位） |
| **DATA 冒号终止** | ✅ `DATA 3,4: PRINT "AFTER"` + `READ A,B,C` → 无幻影项（C 读到注释泄漏值，见 MEDIUM-1） |
| **`:REM`/`:'` 段后 DATA** | ✅ `A=1: REM x: DATA 5` → 不读（Out of DATA） |
| **行首 `'`/REM 注释内 DATA** | ❌ `' note: DATA 9` / `REM foo: DATA 8` → **仍被读出（Q=9、R=8）** |
| PRINT USING 位数 | ✅ `###.##########` 10 位正确；`-1.25` 正确；溢出 `%` 标记正确 |
| **整数 FOR + 小数 STEP** | ❌ `FOR J%=1 TO 2 STEP 0.5` → **死循环**（J% 恒 1，v170 LOW-5 未动） |

### v171 问题闭环

| v171 | 状态 |
|------|------|
| H1 行余指针（单行 FOR/GOSUB/RESUME NEXT 主体） | ⚠️ 单行 FOR 同体 ✅、简单 GOSUB tail ✅、RESUME NEXT 段1+ ✅；**跨行 NEXT 回归**、嵌套 tail 错乱、段0/residual 未闭环 |
| H2 IF 行号+尾随 | ✅ 首 token 判据正确（THEN/ELSE/REM 尾随全过） |
| H3 2D double 数组 | ✅ 2D/3D typed set 全精度 |
| M1 READ 跳 REM 段 | ⚠️ `:REM`/`:'` 段 ✅；**行首 `'`/REM 注释整体未跳过** |
| M2 DATA 幻影项 | ✅ 冒号终止（无幻影） |
| M3 D 指数 | ✅ |
| M4 DATA 死分支/UB | ✅ 分支删除、`e` 预读消除 |
| M5 USING 分数位 | ✅ `%.*f` 提取不再丢精度 |
| M6 `.16g` | ✅ 16 位有效数字 |
| STEP 后跳数字 | ✅ 同体 `STEP 1` 正常 |

### 正面确认

| 功能 | 结果 |
|------|------|
| 单行 FOR（同体）+ 负步/浮点步/GOSUB 内嵌 | ✅ 全对 |
| IF 分支复合 + 行号跳转 | ✅ T1 T2 / ELSE 400 / THEN 500 |
| 2D/3D double、D 字面量、`0.1+0.2` EQ | ✅ 无回归 |
| 3592/966/111/0W/0孤儿 | ✅ |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **跨行 NEXT 回归：单行 FOR 体（`FOR I=1 TO 3: stmt`）的 NEXT 放在下一行时循环只执行 1 次** | `src/next86.c` / `src/gwsts.c` | 169-212 / 855-870、940-960 |
| 2 | **`error_seg_index` 生命周期不完整：段0 错误判据（`>0`）漏恢复同行后续段；handler 用 RESUME/GOTO 退出后索引残留导致下一错误跳错段** | `src/gwsts.c` / `src/gwdata.c` | 982-986、2915-2923 / 53、65 |

**HIGH-1 详情:** v171 引入 `body_text`，`next86_int_next`/`next86_float_next` 在 `cmp==0`（循环继续）时**无条件** `return -3`，由 compound 执行器恢复体指针。但当 FOR 行以冒号收尾、NEXT 写在下一条**独立行**时（GW-BASIC 常见写法，ASM 中 FOR 保存文本指针后由 `for_next_line` 续环），-3 在顶层语句循环中无 consumer，被当作普通返回码吞掉：

```
10 FOR I=1 TO 3: PRINT "F"; I;
20 NEXT I
→ 实测: F 1 / DONE（应 F 1 F 2 F 3 / DONE）
```

传统多行 FOR 不受影响（无 body_text）。这是 v171 修复轮的**回归**：v170 及以前 `next86_next` 无条件返回 `for_next_line`，跨行 NEXT 正常。**修复:** NEXT 返回 -3 前判定位：仅当 NEXT 处于同一文本段（当前行 == FOR 行）时走体恢复；否则返回 `entry->for_next_line`（顶层跳转按行号继续）。

**HIGH-2 详情:** 两个表现同根——`error_seg_index` 的 pin/复位/判据三处不完整：

```
① 段0 错误:
   20 B=1/0: PRINT "SEG2"
   → RESUME NEXT 输出 H / LINE30，SEG2 丢失。
   GW-BASIC 应执行错误语句的下一语句（SEG2）。

② 残留:
   30 A=1: IF F=0 THEN X(99)=1: C=9     （F=0 → ERR9，pin seg=1）
   handler: IF ERR=9 THEN F=1: RESUME    （RESUME 不消费 index）
   50 D=1/0: PRINT "S2-SHOULD-PRINT"     （ERR11）
   → RESUME NEXT 用残留索引 1 重入 line50 跳过 2 段 → S2 丢失，直接 LINE60。
```

原因：`error_seg_index < 0` 才 pin、`> 0` 才段恢复、仅在 RESUME NEXT 消费时复位。①应改为 `>= 0`（段0 也要同行恢复）；②应在 handler 每次处理新错误时复位（如 error 触发时清 `error_seg_index=-1`），GOTO/RESUME 退出后不能残留。**修复:** 错误设置路径（`gwdata_set_error`）复位 `error_seg_index=-1`，pinning 改 `>= 0`，RESUME NEXT 段恢复判据用"错误段后仍存在段"而非纯 `>0`；补 A/B 两探针为单测。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **行首 `'`/REM 注释内 `: DATA` 仍被当数据读出** | `src/bimisc.c` | 557-588 |
| 2 | **整数 FOR + 小数 STEP 死循环：`FOR J%=1 TO 2 STEP 0.5` 恒 J%=1** | `src/next86.c` | FOR/NEXT 递增路径 |
| 3 | **嵌套 GOSUB + 行尾续行组合错乱（多余重复执行 + RETURN without GOSUB）** | `src/gwsts.c` / `src/next86.c` | RETURN tail 复合 / gosub tail |

**MEDIUM-1:** v171 只跳过 `:REM`/`:'` **段**，行首注释未整体跳过：

```
10 ' note: DATA 9        + READ C → C=9（注释当数据）
10 REM foo: DATA 8       + READ R → R=8（同上）
70 PRINT "X: DATA 7"     → READ S 正确 Out of DATA（字符串引号跟踪 OK）
```

ASM 中 READ 的 DATA 行扫描基于已 tokenize 文本，REM 内容不产生 DATA token；C 端文本扫描必须在行首识别 `'` 或 REM（词边界）时整行跳过。**修复:** 行扫描入口处：`trimmed` 以 `'` 开头或匹配 `REM`+边界 → 直接 next line；补行首撇号/REM 注释含 `: DATA` 用例。

**MEDIUM-2:** v170 LOW-5 原样保留。ASM 整数 FOR 的 STEP 经 `GETINT`（强制整数对）读取，0.5 按 CINT 取整；当前实现 step 保留 double 0.5，整数变量 `v->value.d += 0.5` 后写回 `(int32_t)` 恒为 1 → **无限循环**，且与 `.bas` 超时行为一致（探针 3 秒打了数百个 "B 1"）。**修复:** 整数变量 FOR 的 STEP 在解析时 CINT 取整（并与 ASM GETINT 的舍入规则对齐）；补 `FOR I%=.. STEP 0.5`、`STEP 1.5` 用例。

**MEDIUM-3:** 简单 `GOSUB 100: tail` 正确（v172 已验证），但嵌套 GOSUB + 尾部续行的多层组合错乱：`GOSUB 100: B1: B2: GOSUB 200: B3` 且 100 内部再 GOSUB 300 + 尾行 RETURN 时，输出 `S1 S3 S1-TAIL2 B1 B2 S2 B3 S3 Error at line 300`——S3 重复执行、300 行 RETURN 无匹配 GOSUB。tail 存储在 gosub entry 上，RETURN 弹出时把整个 tail 当复合段执行，tail 内部再遇 GOSUB/RETURN 时与弹栈次序交错。**修复:** tail 复合执行不做多级嵌套（或在 tail 内暂存新 GOSUB、RETURN 只弹一层最邻近表项）；补嵌套 GOSUB tail 集成用例（当前 `test_gosub_same_line_tail` 只测 missing-line 返回码，未覆盖真实返回）。

---

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | `test_gosub_same_line_tail` 名实不符：只断言 `GOSUB 900`（缺行）返回 -2，未测真正 RETURN 尾行；注释亦承认"covered by .bas probe" |
| 2 | 6 条新单测未覆盖本轮 5 个回归点：跨行 NEXT、段0 RESUME NEXT、index 残留、行首注释 DATA、整数 STEP 死循环 |

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| 单行 FOR 用 `body_text` + `-3` 体恢复 | ⚠️ 同体型正确，但 -3 无条件、跨行 NEXT 无 consumer → 回归（HIGH-1） |
| GOSUB tail stash/take + RETURN 复合 | ⚠️ 简单场景正确；嵌套 + 弹栈次序未闭环（MEDIUM-3） |
| RESUME NEXT 段索引重入 | ⚠️ 段1+ 正确；段0 判据与残留生命周期缺失（HIGH-2） |
| `GWSIFS_IS_LINENUM` 首 token | ✅ 与 ASM DOCOND 一致（THEN/ELSE/REM 尾随全过） |
| 2D/3D typed set、D 字面量、`%.16g`、USING `%.*f` | ✅ 精度链完整闭环 |
| READ 扫描跳过注释段 | ⚠️ 只覆盖段首；行首注释仍泄漏（MEDIUM-1） |
| 测试 +6 | ⚠️ 覆盖了修复主体，但遗漏本轮全部 4 处行为缺陷（全部手工探针发现） |

**根因**: 段索引/体指针机制本身可行（同体型全对），但三处生命周期边界（跨行 NEXT 的 -3 归属、error_seg_index 的 pin/reset/判据、GOSUB tail 的弹栈次序）未闭环；READ 注释跳过只做了段级、没做行级。

---

## 四、改进建议

### 短期（阻断合并）
1. **HIGH-1**：NEXT 仅当与 FOR 同文本段时返回 -3；否则返回 `for_next_line`。补 `FOR…: body` + 下一行 NEXT 用例（正/负步）。
2. **HIGH-2**：`gwdata_set_error` 复位 `error_seg_index=-1`；pin 用 `>= 0`；RESUME NEXT 判"错误段后仍有段"。补段0、残留两探针为单测。
3. **MEDIUM-1**：行首 `'`/REM（词边界）整行跳过；补 `' note: DATA 9`、`REM foo: DATA 8` 用例。
4. **MEDIUM-2**：整数 FOR STEP 解析时 CINT 取整；补 0.5/1.5/负步用例并防死循环。
5. **MEDIUM-3**：GOSUB tail 复合与弹栈次序（tail 内 GOSUB/RETURN 只弹最近一层）；补嵌套用例。

### 中期
6. `test_gosub_same_line_tail` 改为真实 GOSUB+RETURN 行集成断言，去掉 -2 假测试。
7. 类型矩阵测试追加：FOR（同体/跨行/整数 STEP）× 尾行（GOSUB/RESUME）× 段错误 × 注释 DATA。

### 长期
8. compound 执行器整体换 tokenizer/文本指针模型（见 v171 建议），彻底消除 `-3`/段索引/尾行 stash 三类补丁式机制。
9. ASAN/UBSAN + fuzz（嵌套 GOSUB tail、跨行 FOR、段错误恢复）。

---

## 五、总结

**总体评价**: v171 修复轮主体真实有效——单行 FOR 同体型、简单 GOSUB 尾行、RESUME NEXT 段1+ 恢复、IF 行号+尾随、2D/3D double 数组、D 字面量、`%.16g`、USING 分数位、DATA 冒号终止全部实测闭环，3592/3592、966=966、0 警告、111/111 .bas。但补丁式机制的边界三连击：跨行 NEXT 回归（只执行 1 次）、`error_seg_index` 段0/残留（静默丢语句）、行首注释 DATA 泄漏，均未被 6 条新单测覆盖，全部由独立进程手工探针暴露。**建议：阻断合并**，先修 HIGH-1/2，再补 MEDIUM-1/2/3 与对应用例。

**关键发现**: **0 CRITICAL / 2 HIGH / 3 MEDIUM / 2 LOW**；3592 单测、111/111 .bas、0 警告、0 孤儿。
**建议**: 阻断合并。HIGH-1 与 HIGH-2 是同一"段/体指针生命周期"问题的两个边界，修复应一并做。

---

## 六、指标追踪

| 指标 | v171 | v172 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3569 / 3569 | **3592 / 3592** | +23 |
| 测试函数 | 960 | **966** | +6 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas | 111/111 | **111/111** | 保持 |
| 单行 FOR（同体） | 仅 1 次（v171 前坏） | **1 2 3 / STEP / GOSUB 内嵌全对** | ✅ 修复 |
| 跨行 NEXT + 单行 FOR 体 | — | **F 1 后即 DONE** | ❌ 新回归（HIGH-1） |
| GOSUB 尾行（简单） | BACK 丢失 | **SUB / BACK / LINE20** | ✅ 修复 |
| 嵌套 GOSUB + 尾行 | — | **S3 重复 + Error 300** | ❌ 新 MEDIUM-3 |
| RESUME NEXT 段1+ | 行余跳过 | **AFTER-ERR / C2 / D2 恢复** | ✅ 修复 |
| RESUME NEXT 段0 | — | **SEG2 丢失** | ❌ 新 HIGH-2(①) |
| error_seg_index 残留 | — | **S2 丢失（跳错段）** | ❌ 新 HIGH-2(②) |
| IF 行号+尾随 | Error at line 10 | **正确跳转（THEN/ELSE/REM）** | ✅ 修复 |
| 2D/3D double 数组 | 1.23456788063049 | **全 15 位** | ✅ 修复 |
| D 字面量 / %.16g / USING 分数位 | 丢精度 | **全对** | ✅ 修复 |
| DATA 冒号终止 | 幻影 C=4 | **无幻影** | ✅ 修复 |
| `:REM` 段后 DATA | Q=5（泄漏） | **Out of DATA** | ✅ 修复 |
| 行首 `'`/REM 注释 DATA | — | **Q=9、R=8（泄漏）** | ❌ 新 MEDIUM-1 |
| 整数 FOR + 小数 STEP | 未测 | **死循环** | ❌ 新 MEDIUM-2 |
| CRITICAL / HIGH | 0 / 3 | **0 / 2** | 维度下降但边界面扩大 |

**注释**: 单测/构建使用 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 按目录全量 111 逐一比对（\r 归一、stdin=/dev/null、尾换行差异视同通过）。所有 new 项均为独立进程手工探针；ASM 对照 GWMAIN.ASM（`FOR`/`GETINT`/`NEXT`/OSHTXT 语义）。仅更新本报告，未改源码、未提交。