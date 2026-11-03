# GW-BASIC C Port - Code Review v162

**日期**: 2026-11-02
**审查范围**: `85c772b` → `369842a`（修复轮 `2685713`：v161 的 3 项 HIGH + 6 MEDIUM + LOW 处理）
**测试状态**: 单元断言 **3401/3401 通过**（+21，0 失败）| 编译警告 **0** | 已登记 .bas **86/86 一致** | 定义/注册 **921 = 921**（0 孤儿）
**修复轮结论**: 本轮声明的 3 项 HIGH 全部实测闭环（SWAP 标量↔数组、显式 2D 字符串数组、算术 inf→Overflow）；`##-`/`##+` 符号、行中打印区（重定向）、求值错误伪输出、HEX double 边界也均修复。**本轮 0 CRITICAL / 0 新增 HIGH**；仍有 **3 项既有 HIGH**（字符串赋值回退、未 DIM 多维数组别名、数值类型后缀）与 6 MEDIUM。
**工作区状态**: 本次仅更新 `CODE_REVIEW.md`；`progress.md` 为审查前既有修改；未跟踪 `.reposman`、`GW-BASIC-READONLY-SOURCE`、`QWEN_SETUP.md`、`REVIEWER.md`、`tests/mini_u2.c`
**总体评分**: **B（本轮变更可合并；3 项既有 HIGH 建议另案）**

---

## 一、概述

修复轮 `2685713`（8 文件，+320/-70）处理 v161 清单：

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | +170/-65：SWAP 标量↔数组/多维字符串；打印区 `ftell` 列；求值失败不输出伪值；标量数值赋值中止；2D 字符串赋值 multi |
| `src/biptrg.c` | +43：`biptrg_get/set_array_string_multi()`（完整索引） |
| `src/gweval.c` | +10：表达式结果 `isfinite` → Overflow 6 |
| `src/bistrs.c` | +6：HEX/OCT double 边界检查先于 `(int)` |
| `src/biprtu.c` | +35/-5：尾符号计宽、`##+` 负号抑制、`isfinite` 入口、fallback 列计数 |
| `tests/test_biprtu_out2.c` | +105：7 条输出/错误路径断言 |
| `tests/Makefile` / `test_runner.c` | 注册新测试 |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3401/3401**，0 失败，0 警告 |
| 定义/注册 | ✅ **921 = 921**，0 孤儿 |
| 已登记 .bas | ✅ **86/86** |
| SWAP 标量↔数组（数值） | ✅ `M 9 1` |
| SWAP 标量↔数组（字符串） | ✅ `Syx` |
| 2D 字符串数组（显式 DIM） | ✅ `Pabc` → `Qbac`，`LEN=1` |
| SWAP 2D 字符串数组 | ✅ `bac` |
| 算术溢出 `A*1E38` | ✅ `ERR=6`，B=0，`^^^^` 输出 `0.00E+00` |
| `##-`；`##+` 正/负 | ✅ ` 5 `/` 5-`、` 5+`/` 5-` |
| `-##` / `+##` 正/负 | ✅ ` 5 `/` 5-`、` +5`/` -5` |
| USING 求值失败 | ✅ 无伪字段输出（`A` → `ERR=11` → `B`） |
| 行中尾逗号 | ✅ `AB 1          X`（X 第 15 列） |
| HEX$(65535.4/-32768.4) | ✅ `FFFF` / `8000` |
| HEX$(1E+38/3E+9) | ✅ `ERR=6`（先 double 边界，无 UB） |
| 70 个 USING 值 | ✅ 70 个 `1` |
| 字符串标量 SWAP（不同长度） | ✅ `Llongerone` |
| 2D 数值数组 SWAP | ✅ `D 2 1` |

### v161 问题闭环表

| v161 编号 | 状态 | 说明 |
|-----------|------|------|
| H1 标量↔数组 SWAP | ✅ | 数值/字符串均正确交换 |
| H2 2D 字符串数组 | ✅ | 显式 `DIM A$(2,2)` 读写/SWAP 正确（`LEN=1`） |
| H3 算术 inf + `^^^^` | ✅ | 表达式层 Overflow 6；入口 `isfinite` 防御 |
| H4 数值类型后缀 | ❌ | `#` 仍 single、`%` 无整数溢出（既有，未在本轮范围） |
| M1 `##-` 宽度 | ✅ | ` 5 ` / ` 5-` |
| M2 `##+` 双负号 | ✅ | ` 5-` |
| M3 行中打印区 | ⚠️ | 重定向 ✅（X 第 15 列）；USING 换行后 ❌；真实控制台 ❌ |
| M4 求值失败伪输出 | ✅ | 无伪字段 |
| M5 HEX `(int)` UB | ✅ | double 边界先判 |
| M6 输出断言 | ⚠️ | +7 条，但 2 条断言过弱（见 MEDIUM-6） |
| L1 malloc fallback 列计数 | ✅ | `g_out_cols` 累加 |
| L2 科学计数标志 | ❌ | `$`/`*`/`-0.0` 仍未处理 |
| L3 赋值中止 | ⚠️ | 仅标量数值；数组/字符串仍写回 |
| L4 扩容 OOM | ❌ | `vals` 仍静默丢弃 |
| L5 2D 字符串 SWAP | ✅ | 完整索引 |

### v161 更正说明

v161 报告将“未 DIM 的多维数组”视为已由 `DIM A$(2,2)` 支持；本轮实测发现**未显式 DIM 时仍按 1D 自动建维**（`ensure_array` 固定 10 个元素），导致 `A(1,1)`/`A(1,2)` 别名。该问题为既有 `ensure_array` 设计，非本轮引入，已登记为 HIGH-2。

---

## 二、问题清单

### 🟥 CRITICAL

无。

---

### 🟧 HIGH（均为既有，非本轮引入）

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **字符串赋值回退把任意表达式当字面量**：`A$=X` → `"X"`，`A$=1+2` → `"1+2"` | `src/gwsts.c` / `src/gweval.c` | 2980-2995 |
| 2 | **未 DIM 的多维数组自动建维只建 1D，导致下标别名** | `src/biptrg.c` | 35、82 |
| 3 | 数值类型后缀未实现：`#` 为 single、`%` 无整数溢出 | `src/gwsts.c` | 3015 |

**HIGH-1 详情:** `gweval_eval_string()` 对数字表达式返回 NULL 但**不设置错误**，`assign_value` 的字符串分支遂落入“原始文本当字面量”的回退：

```basic
10 X=5
20 A$=X
30 PRINT "A=[";A$;"]"
40 B$=1+2
50 PRINT "B=[";B$;"]"
```

实际：`A=[X]`、`B=[1+2]`；GW-BASIC 应报 `Type mismatch (13)`。同类：

| 语句 | 实际 | 期望 |
|------|------|------|
| `A$=1/0` | `"1/0"`，无错误 | Type mismatch / Division by zero |
| `A$(1)=1E+39` | `"1E+39"` | Overflow 6 |
| `A$(1)=CHR$(256)` | 元素被写成 `"CHR$(256)"`，随后 ERR=5 | 保持旧值，ERR=5 |

数组字符串分支（`gwsts.c:2943-2955`）连 `gwdata_get_error()` 都不检查，已设置错误仍写回；标量分支（`:2980-2995`）只在“有错误”时中止，而 `eval_string` 对数字表达式不设错误。建议：`gweval_eval_string` 对非字符串表达式设置 13；数组字符串分支复用标量的错误检查；原始文本回退仅限 `DATA/READ/LINE INPUT` 专用入口。

**HIGH-2 详情:** `biptrg_ensure_array()` 固定创建 `biptrg_dim_array(name, 10, type)`（1D）：

```basic
10 A(1,1)=1
20 A(1,2)=5
30 PRINT "N";A(1,1);A(1,2)
40 A$(1,1)="x"
50 A$(1,2)="y"
60 PRINT "S";A$(1,1);A$(1,2)
```

实际：`N 5 5`、`Syy`（第二个下标被忽略，两个元素别名）；期望 `N 1 5`、`Sxy`。显式 `DIM A(2,2)` 正常。GW-BASIC 对未声明数组按**每个维度 10** 自动定维；`ensure_array` 需接受实际下标个数并创建对应维度（或首次引用时按索引长度 `dim_sizes`）。

**HIGH-3（沿用 v160 H4）:** `A#=1.23456789012345#` → ` 1.23457`；`A%=40000` → `40000`。`assign_value` 统一 `alloc_variable(..., VT_SINGLE)`。需单独立项，修复后 SWAP/HEX/USING 应按真实类型读写。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | PRINT USING 的换行未更新 `g_line_start`，后续尾逗号区错位 | `src/gwsts.c` | 2094、2107 |
| 2 | 打印区仅对“非 tty”生效；真实控制台仍走旧 fallback | `src/gwsts.c` | 2107 |
| 3 | 数值数组赋值遇求值错误仍写 0 | `src/gwsts.c` | 2959-2963 |
| 4 | 字符串数组赋值遇错误仍写字面量（见 HIGH-1） | `src/gwsts.c` | 2943-2955 |
| 5 | SWAP 越界：先写有效操作数，再报 ERR=9（数据丢失） | `src/gwsts.c` | 1172-1198 |
| 6 | 新增测试断言过弱：midline 只查 X 存在；no_pseudo 条件近乎恒真 | `tests/test_biprtu_out2.c` | 101-123 |

**MEDIUM-1 详情:** 行中打印区已修（`AB 1          X`，X 第 15 列），但 USING 自身换行未记录行首：

```basic
10 PRINT USING "##"; 1,
20 PRINT "X"
30 PRINT USING "##"; 2
40 PRINT USING "##"; 3,
50 PRINT "Y"
```

实际：`Y` 在第 11 列（` 3        Y`）；期望第 15 列。`gwsts.c:2094` 的 `printf("\n")` 未调用 `gwsts_note_newline()`（对比普通 PRINT 的 `:2215-2216`）。

**MEDIUM-2:** `if (!isatty(fileno(stdout)))` 才使用 `ftell` 列；真实控制台（isatty=1）回退到 `biprtu_last_output_columns()`，行中尾逗号仍按“仅 USING 自身列数”计算。当前只在管道/文件重定向下正确；控制台是主要使用场景，建议维护全局输出列计数（PRINT/PRINT USING/INPUT 均更新），而不是依赖 `ftell`。

**MEDIUM-3:** `C(1)=1E+39` → 元素被写成 0，随后 ERR=6；标量路径已中止，数组路径未检查：

```c
float val = gweval_eval_numeric(expr);
result = biptrg_set_array_element_multi(base_name, indices, val);  // 未检查 error
```

应在 set 之前 `if (gwdata_get_error() != 0) return ERROR;`。

**MEDIUM-4:** 数组字符串赋值（`A$(1)=CHR$(256)`）→ `ERR=5` 后元素仍被写成 `"CHR$(256)"`；应像标量字符串一样“有错误则不写”。

**MEDIUM-5:** `SWAP A$(1),B$(3)`（B$ 未定义 3 号元素）：

- `ERR=9`，但 `LEN(A$(1))` 变为 0（原 "a" 被清空）；
- 数值 `SWAP A(1),A(3)`：`A(1)` 变为 0。

根因：两次 get 后未检查错误即写回。应在两次读取后统一 `if (gwdata_get_error()) return ERROR;`，或先完成全部读取/校验再写。

**MEDIUM-6:** 两条新断言无效：

- `test_biprtu_out2_eval_err_no_pseudo`：
  ```c
  TEST_ASSERT(strstr(capbuf, " 1") == NULL || strstr(capbuf, " 1") > capbuf, ...);
  ```
  `strstr` 返回要么 NULL、要么 ≥ capbuf；除非匹配恰在首字符，否则恒真，无法验证“无伪输出”。
- `test_biprtu_out2_midline_zone` 只断言 `strstr(capbuf,"X")`，不校验列位置，因此未发现 MEDIUM-1。

应改为精确比较输出字符串/列号（如 `strstr(capbuf,"AB 1          X")` 或解析换行后位置）。

---

### 🟩 LOW

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | `g_line_start` 未跟踪 CLS/LOCATE/INPUT 提示，之后打印区可能错 | `src/gwsts.c` | 32-36 |
| 2 | 字符串数组 set 先 `gwram_free` 再 `gwram_alloc`；OOM 时 dangling | `src/biptrg.c` | 407-408 |
| 3 | `D` 指数仍按 single 解析（并入 HIGH-3） | `src/gweval.c` | 455 |
| 4 | `vals` 扩容失败仍 `break`，静默丢值且不设 OM | `src/gwsts.c` | 754-762 |
| 5 | 字符串 SWAP 对 scalar 每次复制新串，旧指针弃用（bump heap，非实际泄漏） | `src/gwsts.c` | 1190-1205 |

### 观察项

1. `PRINT USING` 值列表仍先做字符串探针再求值，副作用表达式有二次求值风险。
2. `ftell(stdout)` 依赖可 seek 的 stdout；管道/控制台行为差异已由 MEDIUM-2 覆盖。
3. 显式 2D 字符串数组已修复；未 DIM 多维自动建维仍为 HIGH-2。
4. 空格式、`-##/##-`、`$$/**`、长串/长字段回归保持通过。

### 遗留问题

| 编号 | 状态 |
|------|------|
| v158 MEDIUM-1：`ERL` 恒为 0 | ❌ 仍存在（`ERR=11, ERL=0`） |
| v158 LOW-1/2：gate 单测未断言 error_line；teardown 残留 | ❌ 未处理 |
| v157 obs-3..6 | ❌ 仍开放 |
| v161 H4：数值类型后缀 | ❌ 转入既有 HIGH-3 |
| v161 M6：尾符号/行中打印区 | ✅ 本轮闭环（行中新问题另记） |

---

## 三、架构评估

### 设计决策

| 决策 | 评价 |
|------|------|
| SWAP 支持标量↔数组、多维字符串、描述符/整 union 交换 | ✅ 本轮核心目标达成；无崩溃 |
| 2D 字符串数组 `get/set_array_string_multi` | ✅ 显式 DIM 正常；auto-DIM 仍 1D（HIGH-2） |
| 表达式结果 `isfinite` → Overflow 6 | ✅ 算术 inf 已被拦截 |
| USING 入口 `isfinite` + 符号计宽/尾符号抑制 | ✅ 2 项 MEDIUM 闭环 |
| 求值失败先返回、不驱动输出 | ✅ 伪字段消失 |
| `ftell(stdout)` + 行首跟踪 | ⚠️ 重定向可用；控制台 fallback（MEDIUM-2）；USING 换行漏记（MEDIUM-1） |
| 赋值中止 | ⚠️ 仅标量数值；数组/字符串未统一（MEDIUM-3/4） |
| 测试捕获 stdout | ✅ 方向正确；2 条断言过弱（MEDIUM-6） |

### 测试覆盖

| 功能 | 覆盖情况 |
|------|----------|
| SWAP 标量↔数组（数值/字符串） | ✅ 单测输出断言 |
| 2D 字符串数组（显式 DIM） | ✅ 单测 |
| SWAP 2D 字符串 | ✅ 手工（Pabc→Qbac） |
| 算术 inf / Overflow | ✅ 单测 |
| `##-`/`##+`/`-##`/`+##` | ✅ 单测 |
| 行中打印区 | ⚠️ 单测只查存在，未查列 |
| 求值失败伪输出 | ⚠️ 断言近乎恒真 |
| 未 DIM 多维数组 | ❌ 无测试（实测 5 5） |
| SWAP 越界部分写入 | ❌ 无测试（实测清空） |
| 数组/字符串赋值错误路径 | ❌ 无测试 |
| USING 换行后打印区 | ❌ 无测试（实测 Y 第 11 列） |
| 全部 .bas | ✅ 86/86 |

### 测试实现质量

| 决策 | 评价 |
|------|------|
| 7 条新测试（输出/错误路径） | ✅ 数量与方向合理 |
| `swap_scalar_array` / `2d_string_array` / `arith_inf` | ✅ 直接守护本轮 HIGH |
| `midline_zone` 仅 `strstr("X")` | ❌ 无法发现列错位 |
| `eval_err_no_pseudo` 条件恒真 | ❌ 形同虚设 |
| 缺少 auto-DIM/SWAP 越界/赋值错误用例 | ⚠️ 盲区 |

---

## 四、改进建议

### 短期（可合并后立即处理）
1. **HIGH-1**：`gweval_eval_string` 对数字表达式设置 `Type mismatch (13)`；数组字符串分支复用标量的错误检查；原始文本回退限定到 `DATA/READ/LINE INPUT`。
2. **HIGH-2**：`biptrg_ensure_array` 接受下标个数，按 GW-BASIC 每维 10 自动定维；补 `A(1,1)=1:A(1,2)=5` 与 `A$(1,1)`/`A$(1,2)` 用例。
3. **MEDIUM-1**：PRINT USING 换行后调用 `gwsts_note_newline()`；补 `USING → USING ,` 列号断言。
4. **MEDIUM-2**：用全局输出列计数替代 `ftell`/`isatty` 分支（控制台/管道一致）。
5. **MEDIUM-3/4/5**：所有赋值/ SWAP 在写回前检查 `gwdata_get_error()`；SWAP 先读齐两操作数再写。
6. **MEDIUM-6**：把弱断言改为精确输出串/列号比较。

### 中期
7. 数值类型后缀（`%`/`#`/`!`）按 ASM 实现；随后 HEX/SWAP/USING 按类型读写。
8. 补 auto-DIM 多维、SWAP 越界、数组/字符串赋值错误路径、USING 换行打印区用例。
9. 打印列跟踪覆盖 CLS/LOCATE/INPUT，避免后续再依赖 `ftell`。

### 长期
10. ASAN/UBSAN + fuzz（多维数组、越界、非有限数、混合类型）。
11. CI：输出断言必须精确比较；`run_bas_tests.sh` 登记检查。

---

## 五、总结

**总体评价**: 本轮修复质量较好——v161 声明的 3 项 HIGH（SWAP 标量↔数组、显式 2D 字符串数组、算术 inf→Overflow）全部实测闭环，`##-`/`##+`、行中打印区（重定向）、求值伪输出、HEX double 边界也均修复；单测从 3380 增至 3401、新增 7 条错误/输出断言。未发现本轮引入的 CRITICAL/HIGH。剩余 HIGH 均为既有（字符串赋值回退、未 DIM 多维自动建维、数值类型后缀）；本轮新增的 2 项 MEDIUM 集中在打印区跟踪与测试断言质量。

**关键发现**: **0 CRITICAL / 0 新增 HIGH / 3 既有 HIGH / 6 MEDIUM / 5 LOW**（另 v158 ERL 未修）。
**建议**: 本轮变更**可合并**；既有 3 HIGH 另案跟踪，优先修 HIGH-1（`A$=X` 存 "X"）与 HIGH-2（未 DIM 2D 别名），并补强两条弱断言。

---

## 六、指标追踪

| 指标 | v161 | v162 | 变化 |
|------|------|------|------|
| 单元断言通过 | 3380 | **3401** | ✅ +21 |
| 测试函数定义 / 注册 | 914 / 914 | **921 / 921** | +7 |
| 孤儿数 | 0 | **0** | 保持 |
| 编译警告 | 0 | **0** | 保持 |
| 已登记 .bas | 86/86 | **86/86** | 保持 |
| SWAP 标量↔数组 | 13（合法程序被拒） | **M 9 1 / Syx** | ✅ 闭环 |
| 2D 字符串数组（显式 DIM） | LEN=0 / ARRA^C | **Pabc→Qbac / LEN=1** | ✅ 闭环 |
| 未 DIM 多维数组 | 未测 | **N 5 5 / Syy（别名）** | ❌ 既有 HIGH |
| 算术 inf + `^^^^` | INT_MIN 指数 | **ERR=6 / 0.00E+00** | ✅ 闭环 |
| `##-` / `##+` | 宽度/双负号错误 | **正确** | ✅ 闭环 |
| 行中打印区（重定向） | 列数偏 2 | **X 第 15 列** | ✅ 闭环 |
| USING 换行后打印区 | 未测 | **Y 第 11 列** | ❌ 新 MEDIUM |
| 求值失败伪输出 | ` 1ERR= 11` | **无伪值** | ✅ 闭环 |
| 字符串赋值回退 | 未测 | **`A$=X` → "X"** | ❌ 既有 HIGH |
| HEX double 边界 | `(int)` UB | **先 double 边界** | ✅ 闭环 |
| 输出断言单测 | 8 | **15** | +7（2 条过弱） |
| CRITICAL / HIGH | 0 / 4 | **0 / 3（均既有）** | ✅ 本轮无新增 |
| `ERL` | 恒 0 | **恒 0** | 未修 |

**注释**:
- 独立验证：WSL 原生 `gcc.exe`/`mingw32-make.exe` 干净重建 → 3401 单测 → 86 个已登记 .bas 全量比对 → v161 全部 H/M/L 探针复跑 → 新增 auto-DIM/SWAP 越界/赋值错误/打印区探针。
- 分配/字符串堆路径未出现崩溃；SWAP 越界为“先写后错”的数据丢失，不是内存破坏。
- 全程未写脚本、未改 `src/`/`tests/`、未 commit；仅更新本报告，等待审批。