# GW-BASIC C Port - Code Review v149

**日期**: 2026-10-25
**审查范围**: commit `d87f8d9`（fix: CODE_REVIEW v148 MEDIUM-1 — RUN 环境复位 `gwsts_reset_runtime_environment`）→ 工作区
**测试状态**: 单元测试 **3260/3260 通过**（0 失败，0 警告）✅ | 交互 RUN 复位 probe ✅ | 文件 I/O 回归 **4/4 一致** ✅ | 文件路径代码未变（v148 全量 80/80 基线保持）✅
**工作区状态**: clean
**总体评分**: B+ — v148 MEDIUM-1 修复确认（变量/栈/ON ERROR/文件/DATA 复位全部验证）+ 新发现 **1 MEDIUM（既有）**：经典 `OPEN "O", #1, "file"` 语法未实现（静默创建名为 `O` 的文件，后续 `PRINT #1` 失败）

---

## 一、概述

本次审查 v148 MEDIUM-1 修复提交 `d87f8d9`（8 文件，+154/-4）。

### v148 MEDIUM-1 修复确认（全部验证通过）

| 复位项 | 实现 | 独立验证 |
|--------|------|----------|
| 变量 + 字符串堆 | `gwdata_init()`（释放变量/数组、`str_heap.used=0`、重置运行状态；**不触碰** gwlist/屏幕/文件） | ✅ 交互两次 RUN：`A= 1` / `A= 1`（v148 为 `A=2`） |
| FOR / GOSUB 栈 | `next86_clear_for_stack()` + 新增 `next86_clear_gosub_stack()` | ✅ 单测：污染 FOR（inside_for=true）/GOSUB（depth>0）→ reset 后均清零 |
| ON ERROR 状态 / ERR | `gwdata_set_on_error_goto(0,0)` + `gwdata_exit_error_handler()` + `gwdata_clear_error()` | ✅ 单测：active→inactive、ERR 变量清除；交互两次 RUN 正常（无 handler 残留） |
| 打开文件 | `giodsk_close_all()` | ✅ 交互两次 RUN（`OPEN ... FOR OUTPUT AS #1` 首次未 CLOSE）→ 无 "File already open"，两次运行无错误 |
| DATA 指针 | `bimisc_data_init()` | ✅ 代码复核（沿用既有） |
| 复位后可用性 | — | ✅ 单测：reset 后 `B = 7` 正常；程序文本（gwlist）保留 |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3260/3260 PASS**（3248 → 3260，+12*） |
| 编译警告 | ✅ **0 条** |
| 交互 RUN：两次 RUN 变量复位 | ✅ `A= 1` / `A= 1` |
| 交互 RUN：ON ERROR 复位 | ✅ 两次运行正常（无残留 handler） |
| 交互 RUN：文件句柄复位 | ✅ 两次 RUN 成功（首次未 CLOSE） |
| FOR/GOSUB 栈复位 | ✅ 单测（accessor 断言） |
| 文件 I/O 回归 | ✅ print_file（PASS）、file_seq（5/5）、file_rand（5/5）、put_get（5/5） |
| 文件路径代码变更面 | ✅ 无（本提交仅 RUN 分支 + 新增复位函数/accessor + 测试）→ v148 的 80/80 全量基线保持 |

*注：+12 = 新增 `test_gwmain_resetenv.c` 3 个测试函数（变量/栈/ON ERROR）。

---

## 二、问题清单

### 🟥 CRITICAL

无（v148 MEDIUM-1 已修复确认）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **经典 `OPEN "O", #1, "file"` 语法未实现（既有缺口）**：被静默解析为"打开文件名 `O`"（实测在目录创建名为 `O` 的文件），后续 `PRINT #1` → `Error at line 20`；GW-BASIC 兼容语法 | `src/gwsts.c`（OPEN 分支）/ `src/giodsk.c` |

**详情**:

**实证**（交互 RUN）：
```basic
10 OPEN "O", #1, "T_CLASSIC.TXT"
20 PRINT "OPENED"
```
输出 `OPENED`（OPEN 未报错）——但目录中出现 **名为 `O` 的文件**，`T_CLASSIC.TXT` 未创建；后续 `PRINT #1, "DATA"` 在行 20 报错（#1 未分配）。→ 整行被解析为 `OPEN "O"`（文件名 "O"），其余 `, #1, "T_CLASSIC.TXT"` 被静默忽略。

**影响**：
- GW-BASIC 旧式 mode-first OPEN（`OPEN mode, #filenum, filespec`）是经典兼容语法，当前静默错误解析 → 程序行为错误 + 文件系统产生垃圾文件 `O`；
- 现有测试（test_print_file/file_seq/file_rand）全部使用 `OPEN ... FOR OUTPUT AS #1` 新式语法 → **该语法路径零覆盖**（测试盲区）。

**修复建议**：按 ASM（OPEN 解析）支持 mode-first 形式，或至少对 `"O"/"I"/"R"` + `, #` 组合识别后报 SNERR 而非静默打开名为 `O` 的文件；补两种 OPEN 语法的测试（含 mode-first 的 OUTPUT/INPUT/RANDOM）。若确认 ASM 不支持旧式语法，应显式 SNERR 并删除误创建文件路径。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项：

1. **RUN 环境复位缺单测覆盖文件/DATA**：变量/栈/ON ERROR 有单测，文件关闭与 DATA 指针仅交互/代码复核；建议补 `giodsk` 打开计数或文件重开用例。
2. 交互 RUN 入口仍无自动化测试（v148 观察项；执行器同源）。
3. `run_program_lines` 的 `exec_rc` 恒为 SUCCESS（可简化）；v145 图形观察项仍在。

---

## 三、架构评估

### 修复质量（d87f8d9）

| 决策 | 评价 |
|------|------|
| `gwsts_reset_runtime_environment()` 单一入口（8 类状态） | ✅ 正确 — RUN 前调用，职责清晰 |
| `gwdata_init()` 复用（变量/堆/运行状态）+ 边界（不触 gwlist/屏幕） | ✅ 正确 — 复核源码确认 |
| `giodsk_close_all()` 文件关闭 | ✅ 正确（两次 RUN 实测） |
| FOR/GOSUB 栈 clear + depth accessor | ✅ 正确（单测实证） |
| ON ERROR/ERR 复位 | ✅ 正确（单测 + 交互） |
| 新增 3 单测（变量/栈/ON ERROR + 复位后可用性） | ✅ 质量好；缺文件/DATA 单测（观察项 1） |
| 文件路径零改动 | ✅ 低风险（v148 80/80 基线保持；4 个文件 I/O 回归一致） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| RUN 变量/string heap 复位 | ✅ 单测 + 交互 |
| FOR/GOSUB 栈复位 | ✅ 单测（accessor） |
| ON ERROR/ERR 复位 | ✅ 单测 + 交互 |
| 文件句柄复位 | ⚠️ 交互 probe（缺单测） |
| **经典 OPEN mode-first 语法** | ❌ **盲区 → MEDIUM-1** |
| 文件路径 I/O | ✅ test_print_file/file_seq/file_rand/put_get |

---

## 四、改进建议

### 短期
1. **MEDIUM-1**：实现/明确拒绝经典 `OPEN "O", #1, "file"`（按 ASM 确认），避免静默创建 `O` 文件；补两语法测试
2. 观察项 1：补文件/DATA 复位单测；观察项 2：main 级交互 RUN smoke

### 中期
3. v145 图形观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）
4. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
5. ASAN 构建
6. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v148 MEDIUM-1 正确修复 — `gwsts_reset_runtime_environment()` 覆盖变量/string heap、FOR/GOSUB 栈、ON ERROR/ERR、文件、DATA；交互实测两次 RUN 变量复位（`A=1/A=1`）、文件句柄复位、ON ERROR 复位全部通过；新增 3 个单测；单测 3260/3260、0 警告；文件路径零改动、I/O 回归一致。**但发现 1 个既有 MEDIUM**：经典 `OPEN "O", #1, "file"` 语法被静默解析为打开文件名 `O`（实测创建垃圾文件 `O`，后续 `PRINT #1` 失败），新式 `FOR OUTPUT` 语法测试全覆盖而旧式零覆盖。
**关键发现**: 0 CRITICAL / 0 HIGH / **1 MEDIUM（既有兼容语法缺口）**、3 观察项。
**建议**: 按 ASM 处理 mode-first OPEN（实现或 SNERR）+ 补测试；修复后进行 v150 review。

---

## 六、指标追踪

| 指标 | v148 | v149 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3248 | **3260** | ✅ +12（+3 测试函数） |
| 定义数 / 注册数 | 856 / 856 | **859 / 859** | +3 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| 文件路径 .bas | 80/80 | **80/80 基线保持**（代码零变更）+ 4/4 I/O 回归 | 保持 |
| RUN 环境复位（变量/栈/ON ERROR/文件/DATA） | ❌ MEDIUM-1 | ✅ **修复**（全部验证） | 关闭 |
| **经典 OPEN mode-first 语法** | — | ❌ **MEDIUM-1（既有）** | 新发现 |
| M2 进度 | 2/3 + 执行器统一 | **2/3 + RUN 语义修复** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3260 → 交互 RUN 3 项（变量/ON ERROR/文件）+ 单测（栈）→ 文件 I/O 4 项回归
- 全程未使用脚本（管道/命令链 + 内置工具）
- probe 产物（`O`、`T_RUN_ENV.TXT`）已清理；`OPEN "O"` 误创建文件行为已作为 MEDIUM-1 证据记录
- **M2 状态**: RUN 语义完整；剩余既有兼容语法缺口（mode-first OPEN）待处理