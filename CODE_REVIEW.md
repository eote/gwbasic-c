# GW-BASIC C Port - Code Review v141

**日期**: 2026-10-19
**审查范围**: commit `956779e`（feat: M2 图形语句接线 — SCREEN/PSET/PRESET/LINE/POINT；表达式坐标 + LINE INPUT 遮蔽修复 + line_coords 重构）→ 工作区
**测试状态**: 单元测试 **3177/3177 通过**（0 失败，0 警告）✅ | .bas 集成套件 **74/74 全 PASS（0 跳过，+1 test_graphics）** ✅ | 孤儿 **836=836**（新增 10 图形测试全部注册）✅
**工作区状态**: clean
**总体评价**: B- — 图形语句主路径与测试质量高（11 项 probe/单测全绿），但发现 **2 MEDIUM**（隐式数组声明缺失、PSET 坐标解析嵌套括号切割）及 1 LOW（LINE 紧凑写法）

---

## 一、概述

本次审查 M2 图形语句接线提交 `956779e`（13 文件，+531/-13）。

| 变更 | 说明 |
|------|------|
| SCREEN 语句 | SCREEN <mode> → gengrp_set_screen_mode（0/1/2） |
| PSET/PRESET 语句 | 表达式坐标 + 可选颜色；默认 FORCLR/BAKCLR；越界静默（ASM JAE PSTNOT） |
| LINE 语句 | `LINE [(x1,y1)]-(x2,y2)[,color[,B[F]]]`；省略首坐标 = 当前 graphics cursor；B/BF 框线/填充 |
| POINT 函数 | `POINT(x,y)` 数字函数（越界 -1；malformed → SNERR） |
| LINE INPUT 遮蔽修复 | LINE 分支排除 `LINE INPUT `（避免吞掉 LINE INPUT 语句） |
| line_coords 重构 | `gengrp_line_coords`（坐标/属性已解析后执行；box/box_fill） |
| PRESET 签名扩展 | `gengrp_preset(x,y,color)`，color<0 → BAKCLR |
| 新测试 | test_gwsts_graphics.c（10 测试）+ test_graphics.bas/expected 集成 + test_runner/Makefile 注册 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3177/3177 PASS**（826 → 836，+10 测试函数） |
| .bas 集成套件（74 项） | ✅ **74/74 全 PASS，0 跳过**（+1 test_graphics.bas） |
| 孤儿检测（注册核对） | ✅ 10 个图形测试全部 extern+run_test 注册（836=836 推导，0 孤儿） |
| probe：图形主路径（PSET/POINT/LINE 带空格） | ✅ `PSET(100,100),1`→POINT=1；`LINE (10,10)-(20,20),1`→端点=1；SCREEN 0 正常 |
| probe：PSET 默认色 / PRESET 默认色 | ✅ 单测实证：PSET 默认 FORCLR=1 / PRESET 默认 BAKCLR=0 |
| probe：POINT 越界 | ✅ `POINT(5000,5000)` → -1 |
| probe：DIM 数组路径 | ✅ `DIM A(10): A(1)=5` → `A1= 5` |
| probe：**未 DIM 数组赋值** | ❌ `A(1) = 5` → `Error at line 10` — **MEDIUM-1 实证** |
| probe：**PSET 数组坐标（嵌套逗号）** | ❌ `DIM A(10): PSET(A(1), A(1)), 3` → 静默画到 (0,0)（POINT=0）— **MEDIUM-2 实证** |
| probe：**LINE 紧凑写法** | ❌ `LINE(10,10)-(20,20),1`（无空格）→ `Error at line 50` — **LOW-1 实证** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **数组隐式声明缺失**：未 DIM 的数组直接使用（`A(1) = 5`）→ SNERR；GW-BASIC 自动隐式声明为 0-10 元素 | `src/gweval.c` / `src/gwdata.c`（数组引用解析路径） |
| 2 | **PSET/PRESET 坐标解析不感知嵌套括号**：`PSET(A(1), A(1)), 3` 被裸 `strchr(',')`/`strchr(')')` 误切 → 静默画到 (0,0) | `src/gwsts.c`（PSET 分支）、`src/gengrp.c` |

**详情**:

1. **MEDIUM-1 — 隐式数组声明**：
   ```
   10 A(1) = 5
   20 PRINT "A1="; A(1)
   ```
   实测：`Error at line 10`。GW-BASIC 语义：未 DIM 的数组首次引用自动声明为 10 元素（下标 0-10）。显式 `DIM A(10)` 路径正常（probe `A1= 5`）。套件 3 个数组测试（test_arrays/test_multi_array/test_string_arrays）**全部使用显式 DIM** → 隐式路径为测试盲区。修复：数组引用解析（parse_factor 与 assign LHS / biptrg 入口）在变量不存在时自动 `DIM A(10)`（ASM 数组隐式声明语义），补未 DIM 用例（赋值/读取/字符串数组/下标 >10 → FCERR）。

2. **MEDIUM-2 — PSET 坐标嵌套逗号切割**：
   ```c
   /* PSET 分支 */
   const char *comma = strchr(p, ',');
   const char *closep = strchr(p, ')');
   ```
   `PSET(A(1), A(1)), 3`（A(1)=5）：第一个 `,` 落在 `A(1` 内 → x 段 = `"A(1"` → eval 部分解析 = 0；closep 取 `A(1)` 的 `)` → y 段错位 → **PSET(0,0) 静默执行**（无错误、无提示），`POINT(5,5)=0`。合法程序静默错位。LINE 分支的 y 段扫描已有 depth/inq 感知（while 循环），PSET 未采用 → 分支间不一致；LINE 首坐标 `closep` 定位同样为裸 strchr（`LINE(ARR(1,2),...)` 同类问题）。修复：抽取统一坐标段解析（depth/inq 感知，与 LINE y 段一致）供 PSET/PRESET/LINE 复用；补数组/多参函数坐标用例（`PSET(A(1),A(2))`、`LINE(A(1),A(2))-(B(1),B(2))`）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **LINE 紧凑写法不支持**：`LINE(10,10)-(20,20)`（关键字后无空格）→ SNERR；`starts_with("LINE ")` 要求空格（PSET 无此限制） | `src/gwsts.c`（LINE 分支） |

**详情**: GW-BASIC 关键字与括号间空格可选；`LINE (10,10)-(20,20)` ✅ 但 `LINE(10,10)-(20,20)` ❌。与 v130 ON2GOTO 紧凑问题同型。修复：`starts_with("LINE")` + 跳过空白 + 保持 `LINE INPUT` 排除。另注：`SCREEN1` 同型（`starts_with("SCREEN ")`），影响较小。

---

## 三、架构评估

### 图形批次质量（956779e）

| 决策 | 评价 |
|------|------|
| SCREEN/PSET/PRESET/LINE/POINT 语句接线 | ✅ 主路径正确（probe 全绿）；⚠️ 坐标解析细节见 MEDIUM-2 |
| PSET/PRESET 默认色（FORCLR/BAKCLR，color<0 哨兵） | ✅ 正确（单测实证 1/0）|
| 越界静默（ASM JAE PSTNOT） | ✅ 正确（PSET OOB 无错误；POINT OOB=-1）|
| LINE 省略首坐标 → graphics cursor（ASM SCAND） | ✅ 正确（单测 + probe `LINE -(9,9)`) |
| LINE B/BF（框线/填充） | ✅ 正确（BF 内部点=1）|
| LINE INPUT 遮蔽排除 | ✅ 正确（`!starts_with("LINE INPUT ")`）|
| line_coords 重构（坐标已解析，cursor 更新语义） | ✅ 合理 |
| POINT 数字函数（malformed → SNERR） | ✅ 正确 |
| **PSET 坐标段裸 strchr（无 depth）** | ⚠️ **MEDIUM-2** — 嵌套逗号静默错位 |
| **隐式数组声明** | ⚠️ **MEDIUM-1** — 既有缺口（非本批引入，本批首次暴露）|
| 10 新单测 + .bas 集成（74/74） | ✅ 质量高；缺数组坐标/未 DIM 用例 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| PSET/PRESET 默认色 / OOB 静默 / roundtrip | ✅ 单测 |
| LINE 简单/表达式/BF/省略首坐标 | ✅ 单测 + .bas |
| POINT / POINT OOB | ✅ 单测 |
| SCREEN 切换 | ✅ 单测 |
| **未 DIM 数组（隐式声明）** | ❌ 盲区 → MEDIUM-1 |
| **PSET/LINE 数组/嵌套函数坐标** | ❌ 盲区 → MEDIUM-2 |
| **LINE 紧凑写法** | ❌ 盲区 → LOW-1 |
| 注册一致性 | ✅ 836=836，0 孤儿 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：数组引用路径补隐式 DIM(10)（赋值/读取/字符串数组/越界 FCERR），补未 DIM 单测与 .bas
2. **MEDIUM-2**：抽取统一坐标段解析（depth/inq 感知，与 LINE y 段一致）供 PSET/PRESET/LINE 复用；补 `PSET(A(1),A(2))`、`LINE(A(1),A(2))-(B(1),B(2))` 用例
3. **LOW-1**：`starts_with("LINE")` + 空白跳过（保持 LINE INPUT 排除）；SCREEN 同型可一并处理

### 中期（M2 规划）
4. 图形深化：CIRCLE/PAINT/GET/PUT（M2 后续批次）
5. .bas 套件补图形数组坐标/紧凑写法场景
6. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
7. ASAN 构建
8. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: M2 图形语句接线完成度良好 — SCREEN/PSET/PRESET/LINE/POINT 主路径正确（默认色、OOB 静默、BF 填充、省略首坐标、line_coords 重构），10 个新单测质量高、套件 74/74 全绿（76→836、0 孤儿）。**但发现 2 MEDIUM**：① 未 DIM 数组隐式声明缺失（`A(1)=5` → Error；套件全用显式 DIM 掩盖）；② PSET/PRESET 坐标解析裸 strchr 不感知嵌套括号（`PSET(A(1),A(1)),3` 静默画到 (0,0)）。另 1 LOW（LINE 紧凑写法）。
**关键发现**: **2 MEDIUM**、**1 LOW**、0 HIGH/CRITICAL。
**建议**: 补隐式数组声明 + 统一坐标解析（depth 感知）+ LINE/SCREEN 紧凑写法；修复后 M2 图形深化（CIRCLE/PAINT）。

---

## 六、指标追踪

| 指标 | v140 | v141 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3147 | **3177** | ✅ +30（+10 测试函数） |
| 定义数（唯一函数） | 826 | **836** | +10 |
| 注册数（run_test 调用） | 826 | **836** | +10 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 73/73 | **74/74** | ✅ +1（test_graphics） |
| 图形语句（SCREEN/PSET/PRESET/LINE/POINT） | ⏳ 未接线 | ✅ 主路径 | 完成（含 2 MEDIUM） |
| **隐式数组声明** | 缺口（未暴露） | ❌ **MEDIUM-1** | 新发现 |
| **PSET 嵌套坐标** | — | ❌ **MEDIUM-2** | 新发现 |
| LINE 紧凑写法 | — | 🟩 LOW-1 | 新发现 |
| M2 进度 | 2/3 + 3 修复轮 | **2/3 + 图形批接线** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3177 → 套件 74/74 → 注册核对（10 图形测试全注册）→ 12 项 probe（已清理）
- 本轮不使用脚本（按用户要求）；孤儿检测改用内置 grep_search 核对注册
- MEDIUM-1 为既有缺口（数组隐式声明从未实现/未测），本批数组坐标 probe 首次暴露；MEDIUM-2 为本批 PSET 解析引入
- **M2 状态**: 图形语句主路径完成；CIRCLE/PAINT/GET/PUT 待后续批次