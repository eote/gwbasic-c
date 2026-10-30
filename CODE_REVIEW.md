# GW-BASIC C Port - Code Review v159

**日期**: 2026-10-30
**审查范围**: commit `6c477f2` → `7cb9bee`（M4 收尾：PRINT USING 完整重写 + SWAP + HEX$/OCT$）
**测试状态**: 单元断言 **3344/3344 通过**（0 失败）| 编译警告 **0** | 已登记 .bas **85/85 一致** | 定义/注册 **897 = 897**（0 孤儿）
**测试盲区**: 新增 `tests/bas/test_using.bas` **未登记**进 `run_bas_tests.sh`（手工运行通过但不参与 CI）；本提交**未新增任何单测**（3344/897 与 v158 相同）
**工作区状态**: 本次仅更新 `CODE_REVIEW.md`；`progress.md` 为审查前既有修改；未跟踪 `.reposman`、`GW-BASIC-READONLY-SOURCE`、`QWEN_SETUP.md`、`REVIEWER.md`、`tests/mini_u2.c`
**总体评分**: **D+（阻断合并）** — 功能主干可用，但存在 **2 项 CRITICAL 内存安全** + **8 项 HIGH 兼容/语义缺陷**；另有 v158 MEDIUM-1（ERL）仍开放

---

## 一、概述

commit `7cb9bee`（14 文件，+666/-49）一次性重写 PRINT USING、新增 SWAP、HEX$/OCT$，并重写 3 个旧 USING expected：

| 文件 | 变更 |
|------|------|
| `src/biprtu.c` | +306：值列表驱动 `biprtu_print_using_values()`（格式扫描/数字字段/字符串字段/REUSIN） |
| `src/gwsts.c` | +250：`gwsts_print_using()` 语句级处理器 + PRINT USING 分派 + SWAP |
| `src/bistrs.c` / `src/gweval.c` | +25/+14：`bistrs_hex/oct()` 与 HEX$/OCT$ 求值分派 |
| `src/gwbasic.h` | +12：`using_value` 结构、HEX$/OCT$ 原型 |
| `tests/bas/` | 新增 `test_m4_funcs.*`、`test_using.*`；改写 `test_print_using`、`test_using_simple`、`test_using_very_simple` 的 expected |
| `run_bas_tests.sh` | 仅登记 `test_m4_funcs.bas`（**未登记 `test_using.bas`**） |

### 验证结果（独立执行）

| 验证项 | 命令/方法 | 结果 |
|--------|-----------|------|
| 干净重建 + 单测 | `rm -f src/*.o tests/*.o tests/*.exe gwbasic.exe && mingw32-make.exe test` | ✅ **3344/3344**，0 失败，0 警告 |
| 解释器构建 | `mingw32-make.exe` | ✅ `gwbasic.exe` |
| 已登记 .bas（85） | 逐个运行 + 归一化后比对 expected | ✅ **85/85 一致** |
| 未登记新用例 | `./gwbasic.exe tests/bas/test_using.bas` | ✅ 输出与 expected 一致（但未进 CI） |
| 定义/注册 | 897 唯一定义 / 897 `run_test` 注册 | ✅ 0 孤儿 |
| 增量断言 | 本提交新增单测 0 条 | ⚠️ 3344 未变 |
| 交互 probe | 见第二/三节（USING/SWAP/HEX$/OCT$/缓冲区） | ❌ 多项复现缺陷 |

### 正面确认（可保留）

| 功能 | 结果 |
|------|------|
| 基本数字字段 `##.##`、精确适配 `###`、溢出 `%` 前缀 | ✅ `3.14` / `123` / `%1234` 正确 |
| 字符串字段 `!`、`&`、`\  \`、`\ \` | ✅ 与 expected 一致（含截断/补齐） |
| 格式串中字面量、`##, Y=` 字面逗号、REUSIN（值多于字段） | ✅ 基本路径正确 |
| 标量 SWAP：整型 `%`、单精度、双精度 `#`、字符串 `$` | ✅ 实测均正确交换 |
| `HEX$(255)="FF"`、`HEX$(-1)="FFFF"`、`OCT$(64)="100"` | ✅ 16 位补码基本正确 |
| 3 个旧 expected 重写 | ✅ 经 ASM 语义核对（`$` 为字面量 + `%` 溢出、精确适配去前导空格）未发现新错误 |
| `PRINT USING` 格式表达式变量 | ✅ `A$="##.##": PRINT USING A$;3.14159` → ` 3.14` |

### v158 遗留复核

| 编号 | 状态 |
|------|------|
| v158 MEDIUM-1：用户可见 `ERL` 恒为 0 | ❌ **仍存在**（复测除零 `ERR=11, ERL=0`；`gwmain.c` 未改） |
| v158 LOW-1：新 gate 单测未断言 error_line | ❌ 未处理 |
| v158 LOW-2：teardown 未复位行号/无效 `remove` | ❌ 未处理 |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **USING 输出缓冲 1024 无边界检查 → 解释器崩溃** | `src/biprtu.c` | 393 |
| 2 | **数字格式化 `buf[160]` 栈溢出 + 超范围 cast UB** | `src/biprtu.c` | 257、265、311-334 |

**CRITICAL-1 详情:** `biprtu_print_using_values()` 使用 `char outbuf[1024]`，所有 `strcpy`/手写追加（`using_emit_string`、`using_emit_number`、字面量循环）均无剩余空间检查；`&` 字段 + 多值/REUSIN 可轻松超过 1024。

实测（stdin 交互 RUN）：

```basic
10 A$="XXXX…（200 字符）"
20 PRINT USING "&"; A$;A$;A$;A$;A$     ' 1000 字符
```

→ ✅ 正常输出 1000 个 X（`EXIT=0`）。

```basic
20 PRINT USING "&"; A$;A$;A$;A$;A$;A$  ' 1200 字符
```

→ ❌ **进程崩溃**（`EXIT=5`，无 PRINT 输出）。5×200 与 6×200 仅差 200 字符，精确落在 1024 边界外，证明为 `outbuf` 溢出。

**CRITICAL-2 详情:** `using_emit_number()` 的 `char buf[160]` 被两处无界 `sprintf` 写入：

- 科学计数：`sprintf(buf, "%s%.*fE+%0*d", …, frac_digits, …)`（`frac_digits` 来自格式串，最多可达 200+）；
- 溢出路径：`sprintf(o, "%.*f", frac_digits, val)` / `sprintf(o, "%.6f", val)`。

实测 1：`PRINT USING "#.########…（220 个 #）"; 1.5` → 写出 220 位小数（远超 160 字节），进程虽未崩但栈已被覆盖；ASM 对 `leading+trailing > 24` 直接 FCERR（`BIPRTU.ASM:292-300`），本实现无此上限。

实测 2：`PRINT USING "###"; 1E+300`

- `(long long)abs_val` 对 1e300 是 **UB**；
- `%g` 产生 `1e+300` 后走 `sprintf("%.6f")`，向 `buf[160]` 写 300+ 字符；
- 实际输出 `  1`（明显被破坏），并伴随内存越界写入。

**修复建议:** 输出改为可增长缓冲（或先计算所需长度，`snprintf` + 截断）；`buf` 改为按字段长度动态分配；对格式字段做 ASM 同款 24 位上限检查；所有 double→整型转换先做范围检查。

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | `^^^^` 科学字段恒输出 `E+10` | `src/biprtu.c` | 261-268 |
| 2 | `$$` / `**` / `**$` 特殊字段未解析，按字面量输出 | `src/biprtu.c` | 201-250、354 |
| 3 | 负数溢出输出 `%--1234.56`（双负号） | `src/biprtu.c` | 311-334 |
| 4 | HEX$/OCT$ 截断而非四舍五入、无范围检查、无 Overflow | `src/bistrs.c` | 281-299 |
| 5 | SWAP 不支持数组元素/未定义变量（ASM PTRGET 支持） | `src/gwsts.c` | 1024-1105 |
| 6 | 小写 `print using` 不被识别 | `src/gwsts.c` | 1936 |
| 7 | 值列表 >64 个被静默截断 | `src/gwsts.c` | 724-728 |
| 8 | 字符串/数字字段类型不匹配被静默强转，应 TMERR(13) | `src/biprtu.c` | 417-418、435、445-470 |

**HIGH-1 详情:** 科学计数分支把指数写死为 `10`：

```c
sprintf(buf, "%s%.*fE+%0*d", sgn, frac_digits, abs_val, exp_digits, 10);
```

实测 `PRINT USING "##.##^^^^"; 123.45` → **`123.45E+10`**（ASM `$PUFE` 应为规范化尾数/指数，如 `1.23E+02`）。该分支还忽略字段宽度、`$`/`*`/尾符号。

**HIGH-2 详情:** 扫描器只在遇到 `#` 时进入字段，`$$`/`**`/`**$` 位于首个 `#` 之前，因此被当普通字符输出：

| 输入 | 实际 | 期望（ASM DOLRNM/SPCNUM） |
|------|------|---------------------------|
| `PRINT USING "$$###.##"; 12.34` | `$$ 12.34` | 浮动 `$`（如 `  $12.34`） |
| `PRINT USING "**###"; 12` | `** 12` | `***12` |
| `PRINT USING "**$###.##"; 12.3` | `**$ 12.30` | `**$12.30`（星号填充+美元） |

且 `USING_FLAG_ASTERISK` 只置位、从未使用；`$` 标志在 `biprtu.c:354` 被插到整数与小数点之间（如 `12$.34`），与 ASM 的浮动美元语义不符。

**HIGH-3 详情:** 溢出路径先写 `%`、再按 `negative` 写 `-`，随后 `sprintf("%.*f", val)` / `%lld` 又自带负号：

```
PRINT USING "###.##"; -1234.56  →  %--1234.56
```

**HIGH-4 详情:** `(int)val` 直接截断后 `& 0xFFFF`，无舍入/范围检查：

| 表达式 | 实际 | GW-BASIC（ASM FRQINT/FRCINT） |
|--------|------|-------------------------------|
| `HEX$(1.9)` | `1` | `2`（四舍五入） |
| `HEX$(65536)` | `0` | **Overflow (6)** |
| `OCT$(65536)` | `0` | **Overflow (6)** |
| `HEX$(-32769)` | `7FFF` | **Overflow (6)** |

ASM 允许范围 `-32768..65535`（`GWMAIN.ASM:3083-3098` 注释），越界 → `$OVERR`；`FRCINT` 用 ADC 舍入（`MATH2.ASM:230-291`）。另外 `gweval.c:204/211` 用 `float` 接收 `double`，超过 2^24 会丢精度，且 `(int)` 转换对超范围 double 是 UB。

**HIGH-5 详情:** SWAP 只查标量：

- `DIM A(3): A(1)=1: A(2)=2: SWAP A(1),A(2)` → **ERR=13**，数组未交换（ASM `BIMISC.ASM:675` 用 `PTRGET`，天然支持数组元素）；
- `X=5: SWAP X,Y` → **ERR=13**，`X=5,Y=0`；ASM `PTRGET` 会自动创建 `Y`（默认 0）后交换（应 `X=0,Y=5`）。
- 标量 `%`/`!`/`#`/`$` 交换本身正确（已实测）。

**HIGH-6 详情:** `up == strstr(up, "USING")` 大小写敏感；GW-BASIC 关键字不区分大小写。实测 `10 print using "##.##"; 3.14159` → ` 0 3.14159` + `Error executing statement`。

**HIGH-7 详情:** `struct using_value vals[64]` + `while (… && vcount < 64)`。实测 70 个值 → 只输出 **64 个 `1`**，随后 `Error executing statement`，其余值丢失。

**HIGH-8 详情:** 类型不匹配未报 TMERR(13)：

- `PRINT USING "###"; "AB"` → 直接输出 `AB`（ASM `PUFOUT→$SIGNS` 对字符串 → TMERR）；
- `PRINT USING "!"; 123` → 输出空行（ASM `CHKSTR` → TMERR，`BIPRTU.ASM:414`）。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 格式无字段但值未尽 → 应 FCERR(5)，实际静默输出字面量 | `src/biprtu.c` / `src/gwsts.c` | 485 / 696-782 |
| 2 | 缺少格式后 `;` → ERR=0（应 SNERR=2）并多打空行 | `src/gwsts.c` | 710、1940 |
| 3 | `_`（下一字符字面量）未实现 | `src/biprtu.c` | 201-250 |
| 4 | 逗号分组字段宽度少 1 列（ASM 逗号计入字段宽） | `src/biprtu.c` | 201-250、289-305 |
| 5 | 尾随 `,` 不推进打印区，仅抑制换行 | `src/gwsts.c` | 1939-1941 |
| 6 | `test_using.bas` 未登记；文档却称 85/85 含 +test_using | `run_bas_tests.sh` / `DEVELOPMENT_PROGRESS.md` | 98 / 88 |
| 7 | 新功能 0 单测（3344/897 与 v158 相同） | `tests/` | — |
| 8 | 未实现 ASM 的 24 位字段上限（长字段直接触发 CRITICAL-2） | `src/biprtu.c` | 201-250 |

**MEDIUM-1:** `PRINT USING "abc"; 1` → 输出 `abc` 后正常继续；ASM 在格式耗尽且 `USFLG=0` 时 `REUSST → FCERR3`（`BIPRTU.ASM:52-54`）。

**MEDIUM-2:** `PRINT USING "##" 5` → 输出空行 + `ERR= 0`（ASM `SYNCHR 73O` 缺分号 → SNERR=2）。`gwsts_print_using` 的 `return ERROR` 未设置错误码。

**MEDIUM-3:** `PRINT USING "_##"; 5` → `_ 5`；ASM `LITCHR` 应把 `_` 后字符当字面量输出（且不打印 `_`）。

**MEDIUM-4:** `PRINT USING "##,###"; 12` → `   12`（5 列）；ASM `NUMNUM` 对逗号执行 `INC DL`，字段宽 6 → 应右对齐到 6 列（`    12`）。

**MEDIUM-5:** `PRINT USING "##"; 1,` 后接 `PRINT "X"` → ` 1X`；GW-BASIC 逗号应推进到下一打印区（14 列）。

**MEDIUM-6:** `run_bas_tests.sh:98` 只新增 `test_m4_funcs.bas`；`test_using.bas` 手工通过但 `grep '"test_using.bas"'` = 0。`DEVELOPMENT_PROGRESS.md:88` 写 “85/85（+test_using +test_m4_funcs）”，与实际登记不符（85 中不含 test_using）。

**MEDIUM-7:** 新增/修改约 593 行生产代码，单测数仍 3344、897；全部覆盖仅靠 2 个 .bas（其中 1 个未登记），边界/错误路径几乎无自动覆盖。

**MEDIUM-8:** ASM 在字段总位数 >24 时 FCERR；本实现无检查，直接进入缓冲区溢出路径。

---

### 🟩 LOW

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | `gwbasic.h` 中 FRE 注释被错挂到 OCT$ 原型后 | `src/gwbasic.h` | 369-370 |
| 2 | 逗号分组含一段无意义死循环/重复构建代码 | `src/biprtu.c` | 289-305 |
| 3 | 空格式串先打印换行再报 ERR=5（ASM 直接 FCERR） | `src/gwsts.c` | 1940 |
| 4 | `!` 字段遇空串写入 `'\0'` 而非补 1 空格 | `src/biprtu.c` | 445-447 |
| 5 | SWAP 数值按 `i/f/d` 逐成员交换（实现定义行为，含死变量） | `src/gwsts.c` | 1080-1095 |

**LOW-5:** 逐成员交换最终结果在 x86/MinGW 上正确（已实测 `%`/`!`/`#`），但读取非活跃 union 成员属实现定义行为，且 `tmpv` 为死代码；建议改为 `union` 临时变量或 `memcpy` 整块交换。

### 观察项

1. 数值值列表先用 `gweval_eval_string` 做类型探针（`gwsts.c:748-770`），再求值；对有副作用的表达式（如 RND）存在二次求值风险（本次 RND 测试被既有 `RND=0` 干扰，未能定论）。
2. 尾随 `;` 语义正确（` 2Y`），尾随 `,` 需补打印区。
3. SWAP 对 `SWAP A,B,C`、非 lvalue 表达式等返回 13 而非 SNERR。
4. ASM `PRINT USING` 对无值字段会提前结束格式扫描；本实现继续输出后续字面量，属边角差异。

---

## 三、架构评估

### 设计决策

| 决策 | 评价 |
|------|------|
| 值列表驱动 + 顶层 `;` 分割（应对跨表达式片段） | ✅ 方向正确，解决了旧实现 “fmt 在 USING 前” 的设计错误 |
| 格式扫描手写字符机 | ⚠️ 未按 ASM 的 “字段起始组合” 解析：`$$`/`**`/`**$` 完全失效；`_` 缺失 |
| 数字格式化用 `sprintf` + 手写补位 | ❌ 固定缓冲 + 多处无界写，已实测崩溃/栈覆盖 |
| 科学计数 | ❌ 指数写死 10，属未完成实现 |
| 类型探测 “先试字符串再试数字” | ⚠️ 可工作，但静默强转掩盖 TMERR；有二次求值风险 |
| SWAP 标量指针/union 交换 | ⚠️ 标量可用；数组/未定义变量与 ASM 契约不符 |
| HEX$/OCT$ 直接位掩码 | ❌ 缺 FRCINT 的舍入与范围检查 |

### 测试覆盖

| 功能 | 覆盖情况 |
|------|----------|
| `#` 字段基本/精确适配/`%` 溢出 | ✅ test_using / test_m4_funcs |
| `!` `&` `\` 字符串字段 | ✅ |
| REUSIN（值多于字段） | ✅ 基本路径 |
| `^^^^` 科学计数 | ❌ 无测试（且实现错误） |
| `$$` / `**` / `**$` | ❌ 无测试（且实现错误） |
| `_` 字面量 | ❌ 无测试 |
| 类型不匹配错误 | ❌ 无测试 |
| 无字段/缺分号错误 | ❌ 无测试 |
| >64 值 / 长格式 / 长字符串 | ❌ 无测试（且崩溃） |
| SWAP 数组元素/未定义变量 | ❌ 无测试（且失败） |
| HEX$/OCT$ 舍入/范围 | ⚠️ 仅 255/-1/64 三个值 |
| 单测层 | ❌ 0 条新增 |

### 测试实现质量

| 决策 | 评价 |
|------|------|
| 新增 `test_using.bas`（12 条基本格式） | ✅ 覆盖主干，但**未登记**，CI 不执行 |
| `test_m4_funcs.bas` 混合验证 USING/SWAP/HEX$ | ✅ 已登记；但仅正向值 |
| 3 个旧 expected 重写 | ✅ 经 ASM 核对合理（非“迁就实现”）；`$%1234567.88` 等可由字面 `$` + `%` 溢出解释 |
| 边界/错误路径 | ❌ 缺失，导致上述缺陷全部未被发现 |

---

## 四、改进建议

### 短期（阻断合并，必须）
1. **修 CRITICAL-1/2**：输出缓冲改为动态/可增长（或严格 `snprintf` + 长度检查）；字段位数按 ASM 限制 ≤24；所有 double→整型先范围检查。
2. **修 HIGH-1/2/3**：`^^^^` 按 ASM `$PUFE` 规范化尾数与指数；在字段起始解析 `$$`/`**`/`**$`；溢出路径只输出一个负号。
3. **修 HIGH-4**：HEX$/OCT$ 改为 `FRCINT` 语义（四舍五入 + `-32768..65535` + Overflow 6），参数用 double。
4. **修 HIGH-5/6/7/8**：SWAP 支持数组元素与自动建变量；USING 分派改大小写不敏感；值列表改动态数组；类型不匹配报 13。
5. **测试**：登记 `test_using.bas`；补单测覆盖上述每个 HIGH（尤其崩溃用例与范围边界）。

### 中期
6. 按 ASM 重写字段扫描（字段起始组合、`_`、逗号计宽、尾逗号打印区），以表驱动替代手写分支。
7. 增加 24 位字段上限、空格式、缺分号、无字段等错误路径用例。
8. 建立 “真 GW-BASIC 输出” 或 ASM 逐步对照的黄金基准，避免仅以本实现输出反写 expected。

### 长期
9. 为 `biprtu`/`gwsts` 增加 ASAN/UBSAN 构建与 fuzz（格式串/长值/异常类型）。
10. CI 门禁：零警告 + 孤儿检测 + 未登记 .bas 检测 + 所有新增 .bas 必须登记。

---

## 五、总结

**总体评价**: PRINT USING 的分派架构修正（值列表驱动）方向正确，基本 `#`/`!`/`&`/`\`、REUSIN、标量 SWAP、HEX$/OCT$ 的常见值可用，3 个旧 expected 重写也有 ASM 依据。但当前实现存在 **2 项可复现的栈缓冲区溢出（其中 1 项直接崩溃解释器）**，以及科学计数、`$$`/`**`、HEX$/OCT$ 范围、SWAP 数组、大小写、值数量上限、类型错误等 8 项 HIGH 兼容缺陷；新增测试未登记且无单测，未能拦截这些问题。

**关键发现**: **2 CRITICAL / 8 HIGH / 8 MEDIUM / 5 LOW**；另有 v158 MEDIUM-1（ERL）未修。
**建议**: **禁止合并**。先修 C1/C2 内存安全与 H1-H8，再补齐单测与 `test_using.bas` 登记，最后按 ASM 逐项验收。

---

## 六、指标追踪

| 指标 | v158 | v159 | 变化 |
|------|------|------|------|
| 单元断言通过 | 3344 | **3344** | 0（无新增单测） |
| 测试函数定义 / 注册 | 897 / 897 | **897 / 897** | 0 |
| 孤儿数 | 0 | **0** | 保持 |
| 编译警告 | 0 | **0** | 保持 |
| 已登记 .bas | 84/84 | **85/85** | +1（m4_funcs） |
| 新增 .bas 文件 | 0 | **2** | test_using 未登记 |
| PRINT USING 字段覆盖 | 旧分派失效 | **基本字段可用；`$$`/`**`/`^^^^`/`_` 失效** | 部分实现 |
| SWAP | 无 | **标量可用；数组/未定义变量失败** | 新增（不完整） |
| HEX$/OCT$ | 无 | **基本值可用；范围/舍入错误** | 新增（不完整） |
| CRITICAL / HIGH | 0 / 0 | **2 / 8** | ❌ 阻断 |
| MEDIUM / LOW | 1（遗留）/ 2（遗留） | **8 + 1 遗留 / 5 + 2 遗留** | 恶化 |
| v158 ERL | 未实现 | **未实现（复测 ERR=11,ERL=0）** | 保持 |

**注释**:
- 独立验证：WSL 原生 `gcc.exe`/`mingw32-make.exe` 干净重建 → 3344 单测 → 85 个已登记 .bas 全量比对 → 未登记 `test_using` 手工比对 → 10+ 组交互 RUN probe（USING/SWAP/HEX$/OCT$/缓冲区）→ ASM `BIPRTU/MATH1/MATH2/BIMISC` 对照。
- 崩溃 probe 均为独立进程（5×200 正常 / 6×200 `EXIT=5`），未影响主构建。
- 全程未写脚本、未改 `src/`/`tests/`、未 commit；仅更新本报告，等待审批。