# GW-BASIC C Port - Code Review v148

**日期**: 2026-10-24
**审查范围**: commit `95dc312`（fix: CODE_REVIEW v147 CRITICAL-1 — 统一执行器 `run_program_lines`，交互 RUN 与文件路径共用；IF..THEN 行号跳转）→ 工作区
**测试状态**: 单元测试 **3248/3248 通过**（0 失败，0 警告）✅ | .bas **80/80 全量直接运行**（文件路径，输出与 expected 一致）✅ | **交互 RUN 复测全绿**（FOR/GOTO/END/IF..THEN/ON..GOTO/DELETE）✅
**工作区状态**: clean
**总体评分**: B+ — v147 CRITICAL-1 修复确认（执行器统一，交互 RUN 全控制流恢复）+ IF..THEN 行号跳转；新发现 **1 MEDIUM**（RUN 未复位运行时环境：变量跨 RUN 保留）

---

## 一、概述

本次审查 v147 CRITICAL-1 修复提交 `95dc312`（9 文件，+288/-172）。

### v147 CRITICAL-1 修复确认（交互 RUN 全控制流恢复）

| 交互 RUN 场景（stdin 管道复测） | 修复前（v147） | 实测（v148） |
|----------------------------------|----------------|--------------|
| `10 for i=1 to 10 / 20 print i / 30 next` | 仅 ` 1` | ✅ **`1..10`** |
| `10 goto 30 / 20 print "SKIP" / 30 print "TARGET"` | `Error at line 10` | ✅ `TARGET` |
| `10 END / 20 print "AFTER"` | `Error at line 10` | ✅ 无错误、正常结束 |
| `10 IF 1=1 THEN 30 ...`（行号跳转） | `Error at line 10` | ✅ `TARGET`（新增支持） |
| `10 I=2 / 20 ON I GOTO 100,200 ...` | `Error at line 20` | ✅ `B2` |
| `10 DELETE 20 / 15 print "A" / 20 print "B" / 30 print "C"` | `A`/`C`（悬垂读 UB） | ✅ `A`/`C`（liveness 生效） |

**实现**：`run_program_lines()` 单一执行器（g_curlin/current_line、END(-2)、rc<0 错误/ON ERROR/handler-FATAL、rc>0 跳转、DELETE liveness + link_version fast path）；交互 RUN 与文件路径共用；RUN 前 `bimisc_data_init()`（DATA 指针复位）；文件路径删除 127 行重复循环。

### IF..THEN 行号跳转（附带新功能）

- `gwsts.c` +17：`IF <cond> THEN <行号>` 返回跳转行号（rc>0）
- 新单测 `test_gwsts_ifgoto.c`（76 行）+ 集成 `test_if_lineno.bas/expected`（T1/ESTMT/SKIP4/DONE）✅

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3248/3248 PASS**（3242 → 3248，+6*） |
| 编译警告 | ✅ **0 条** |
| **.bas 全量（80 个，分批直接运行）** | ✅ **80/80 输出与 expected 一致**（批 1-6；含 terminating 用例 gwram/onerr_off 的预期终止错误；无预期外 Error） |
| 交互 RUN：FOR/NEXT（小写 + 无变量） | ✅ `1..10` |
| 交互 RUN：GOTO / IF..THEN / ON..GOTO | ✅ `TARGET` / `TARGET` / `B2` |
| 交互 RUN：END | ✅ 无错误 |
| 交互 RUN：运行时 DELETE | ✅ `A`/`C`（与文件路径一致） |
| 交互 RUN：**连续两次 RUN 变量状态** | ❌ `A= 1` → **`A= 2`** — **MEDIUM-1 实证** |

*注：+6 = IF..THEN 跳转相关新测试函数（单测总数 3248）。

---

## 二、问题清单

### 🟥 CRITICAL

无（v147 CRITICAL-1 已修复确认）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **`RUN` 未复位运行时环境**：GW-BASIC 语义为「RUN 清除所有变量并关闭文件」，当前仅复位 DATA 指针 → 变量/FOR 栈/GOSUB 栈/ON ERROR 状态/ERR/文件句柄跨 RUN 残留 | `src/gwmain.c`（RUN 分支 + `run_program_lines`） |

**详情**:

**实证**：
```
10 A=A+1
20 PRINT "A=";A
RUN     → A= 1
RUN     → A= 2      ← GW-BASIC 期望 A= 1（RUN 清变量）
```

**根因**：`95dc312` 在 RUN 前仅调用 `bimisc_data_init()`（复位 READ/DATA 指针）；未重置变量表/字符串堆/FOR 栈(`g_for_sp`)/GOSUB 栈/ON ERROR 状态(`on_error_active`/`in_error_handler`)/`ERR`/`ERL`/文件句柄。v147 旧 RUN 路径同样未复位（历史缺口），执行器统一后该语义缺口成为可观察问题。

**影响**：交互模式下连续 RUN 结果受上次运行污染（变量残留、未完成 FOR 栈残留、错误处理状态残留）；文件加载启动路径不受影响（新进程环境本身干净）。

**修复建议**：抽取 `reset_runtime_environment()`（变量表/string heap/FOR 栈/GOSUB 栈/ON ERROR 状态/ERR/ERL/DATA 指针/打开文件句柄）→ RUN 命令执行前调用；文件入口不调用。注意保留程序行（gwlist 不受影响）与屏幕/图形状态（GW-BASIC RUN 不清屏）。补交互测试：两次 RUN 变量复位、FOR 栈残留清理、ON ERROR 状态复位。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项：

1. **交互 RUN 仍无自动化测试**：执行器已统一（逻辑同源，风险大幅降低），但「RUN 命令 → run_program_lines」入口接线无测试保护；建议 main 级 smoke（stdin `RUN` + `FOR 1..3` → 3 行输出）。
2. RUN 后 FOR/GOSUB 栈、ON ERROR、文件句柄复位缺失（与 MEDIUM-1 同源，一并处理）。
3. `run_program_lines` 的 `exec_rc` 恒为 SUCCESS（错误路径直接 return ERROR）— 无功能影响，可简化。
4. .bas 全量验证采用分批命令链直接运行（未使用脚本）；终态与自动化套件一致性建议在套件脚本恢复运行时复核。

---

## 三、架构评估

### 修复质量（95dc312）

| 决策 | 评价 |
|------|------|
| 抽取单一 `run_program_lines()`，两入口共用 | ✅ 正确 — 根治执行路径分叉（v147 根因） |
| 删除文件路径 127 行重复循环 | ✅ 正确 — 单一事实源 |
| RUN 前 `bimisc_data_init()`（DATA 复位） | ✅ 正确但**不完整**（MEDIUM-1） |
| 交互 RUN 复测：FOR/GOTO/END/IF..THEN/ON..GOTO/DELETE | ✅ 全部恢复（6 项 probe） |
| IF..THEN 行号跳转（rc>0） | ✅ 正确（单测 + 集成 + 交互 probe） |
| 全量 .bas（80）回归 | ✅ 80/80 一致 |
| **RUN 环境复位（变量/栈/ON ERROR/文件）** | ❌ **MEDIUM-1** — 仅 DATA 指针复位 |
| 测试（IF..THEN goto 单测 + 集成） | ✅ 质量好；交互 RUN 入口仍无自动化（观察项 1） |

### 测试覆盖分析

| 功能 | 文件路径 | 交互 RUN |
|------|----------|----------|
| FOR/NEXT / GOTO / GOSUB / ON..GOTO / IF..THEN / END / ON ERROR / DELETE | ✅ 80/80 | ✅ 手动 probe 全绿（无自动化） |
| **RUN 连续执行的环境复位** | —（文件路径不适用） | ❌ MEDIUM-1 |
| IF..THEN 行号跳转 | ✅ test_if_lineno | ✅ probe |
| 单元测试 | ✅ 3248/3248 | — |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：实现 `reset_runtime_environment()` 并于 RUN 前调用（变量/string heap/FOR 栈/GOSUB 栈/ON ERROR/ERR/ERL/DATA/文件句柄）；补交互回归（两次 RUN 变量复位、FOR 栈、ON ERROR）
2. 观察项 1：增加 main 级交互 RUN smoke 测试（至少 FOR 1..3、GOTO、END、IF..THEN 行号）

### 中期
3. 统一执行器后，可让单测直接构造行列表验证控制流（覆盖文件/交互共同逻辑）
4. v145 观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）随批次处理
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
6. ASAN 构建
7. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v147 CRITICAL-1 正确修复 — 执行器统一为 `run_program_lines()`，交互 RUN 的 FOR/NEXT、GOTO、END、IF..THEN（含新增行号跳转）、ON..GOTO、运行时 DELETE 全部恢复；文件路径 80/80 全量验证一致、单测 3248/3248、0 警告；根除「两份循环不同步」的结构性缺陷。**但发现 1 MEDIUM**：RUN 未复位运行时环境（实测两次 RUN 变量保留 `A=1 → A=2`），与 GW-BASIC「RUN 清变量/关文件」语义不符。
**关键发现**: 0 CRITICAL / 0 HIGH / **1 MEDIUM**（RUN 环境复位）、4 观察项。
**建议**: 补 `reset_runtime_environment()`（RUN 前调用）+ 交互 RUN smoke 测试；修复后进行 v149 review。

---

## 六、指标追踪

| 指标 | v147 | v148 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3242 | **3248** | ✅ +6（IF..THEN goto 测试） |
| 定义数（唯一函数） | 853 | **856** | +3 |
| 注册数（run_test 调用） | 853 | **856** | +3 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 13/13（文件路径） | **80/80 全量直接运行一致** | ✅ 全量 |
| 交互 RUN 路径 | ❌ CRITICAL（FOR/GOTO/END 失效 + UAF） | ✅ **全控制流恢复**（6 项 probe） | 关闭 |
| IF..THEN 行号跳转 | ⏳ 未支持 | ✅ 实现（单测 + 集成 + probe） | 新增 |
| **RUN 环境复位（变量/栈/ON ERROR/文件）** | — | ❌ **MEDIUM-1** | 新发现 |
| M2 进度 | 2/3 + 修复轮 | **2/3 + 执行器统一** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3248 → **80 个 .bas 分 6 批直接运行**（expected 对比）→ 交互 RUN 6 项管道 probe → RUN 语义 probe
- 全程未使用脚本（分批命令链 + 内置工具）
- TERMERROR 用例（test_gwram: `Error at line 800`；test_onerr_off: `Error at line 120` + exit -1）符合设计预期
- **M2 状态**: 交互/文件执行器统一；RUN 环境复位为剩余语义缺口