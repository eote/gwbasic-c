# GW-BASIC C Port - Code Review v152

**日期**: 2026-10-28
**审查范围**: commit `4a3b8d0`（feat: v151 观察项 2/3 修复 — OPEN filespec 字符串表达式 + reclen RANDOM-only 语义，ASM GIO86 FRMEVL/PRLENC 对照；修 eval 串所有权堆损坏）→ 工作区
**测试状态**: 单元测试 **3308/3308 通过**（0 失败，0 警告）✅ | .bas **82/82 全量直接运行**（输出与 expected 一致）✅ | RECLEN/filespec probe ✅
**工作区状态**: clean（另有 dev 调试残留 `tests/mini_open.c`，未跟踪，见观察项 1）
**总体评分**: A- — 观察项 2/3 全部修复确认（表达式 filespec、reclen RANDOM-only、均不误建文件），eval 串所有权修正正确；无阻塞问题，6 观察项

---

## 一、概述

本次审查 `4a3b8d0`（8 文件，+487/-184）— 修复 v151 观察项 2/3，并把 OPEN 从 `gwsts_exec_statement` 提取为独立执行器。

| 变更 | 说明 |
|------|------|
| OPEN 提取 | `gwsts_exec_open(trimmed)`（gwbasic.h 导出），入口 `result = gwsts_exec_open(trimmed)` |
| filespec 字符串表达式（观察项 2） | classic/new-style 均用 `gweval_eval_string` 求值（ASM NAMSCN→FRMEVL）：变量 `F$`、拼接 `A$+".TXT"` 均支持 |
| reclen RANDOM-only（观察项 3） | classic `,reclen` 与 new-style `LEN=` 仅 RANDOM 接受；O/I/A + reclen → SNERR；reclen≤0 → FCERR（ASM PRLENC/PNRLEN/VARECS） |
| eval 串所有权 | 表达式 filespec 为 string heap 借用（**不 free**）；仅引号字面量副本（`gwram_alloc`）free — 修正误 free 堆损坏 |
| 新测试 | `test_gwsts_openmodes.c` +120（6 用例）+ 集成 `test_open_varspec.bas/expected` |

### v151 观察项 2/3 修复确认（全部验证通过）

| 验证项 | 独立验证 |
|--------|----------|
| classic 变量 filespec | ✅ 集成 `test_open_varspec`：`R=C1` |
| new-style 变量 filespec | ✅ `S=N1` |
| 拼接表达式 filespec（`A$+".TXT"`） | ✅ `T=K1` |
| O + reclen → SNERR 且不建文件 | ✅ probe `OPEN "O", #1, file, 128` → `Error at line 10`；文件不存在 |
| R + reclen 正常 | ✅ probe `OPEN "R", #1, file, 64` → `RECLEN-R-OK` |
| reclen 0 → FCERR | ✅ probe → `Error at line 10`；不建文件 |
| eval 串所有权 | ✅ 代码复核：`gweval_eval_string` 返回 string heap 借用；仅字面量副本 free（无堆损坏/无 malloc 泄漏） |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3308/3308 PASS**（3287 → 3308，+21*） |
| 编译警告 | ✅ **0 条** |
| **.bas 全量（82 个，分 6 批直接运行）** | ✅ **82/82 输出与 expected 一致**（含 open_classic/open_varspec；terminating 用例符合预期；无预期外 Error） |
| filespec 集成 | ✅ `test_open_varspec`（R=C1 / S=N1 / T=K1） |
| RECLEN probe（3 项） | ✅ SNERR / R-OK / FCERR，均无残留文件 |

*注：+21 = 新增 6 个单测（var classic/newstyle/concat、O+reclen、OUTPUT+LEN、reclen 0）。

---

## 二、问题清单

### 🟥 CRITICAL

无（eval 串所有权修正确认；无堆损坏）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项：

1. **dev 调试残留 `tests/mini_open.c`**（未跟踪）— 手动驱动 `gwsts_exec_open` 的临时程序（含 `remove("T_V.TXT")`），其场景已被正式测试覆盖。建议删除，勿误 `git add`。
2. **表达式 filespec 每次 OPEN 消耗 string heap**（不回收，ASM 堆语义一致；长运行程序存在 OM 风险，RUN 时 heap reset 缓解）— 极低，观察。
3. **classic filenum 未显式校验**（空 `#`/0 → `dskcom_open(0)` 行为依赖下层）；**reclen 上限**（ASM 32767）未校验 — 观察，建议 ASM 对照。
4. 交互 RUN 入口仍无自动化测试（v148 观察项，执行器同源）。
5. v145 图形观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）。
6. `run_program_lines` 的 `exec_rc` 可简化（v148 观察）。

---

## 三、架构评估

### 修复质量（4a3b8d0）

| 决策 | 评价 |
|------|------|
| OPEN 提取为 `gwsts_exec_open`（单一入口、语句分派瘦身） | ✅ 合理 — 可测试性提升（单测可直接调用） |
| filespec 表达式（FRMEVL 语义） | ✅ 正确 — 变量/拼接/字面量三形式均验证 |
| reclen RANDOM-only + SNERR/FCERR 分层 | ✅ 正确 — O/I/A+LEN 拒绝、R 接受、0 拒绝；均不误建文件 |
| eval 串所有权（借用 vs 副本） | ✅ 正确 — string heap 借用不 free；仅 `gwram_alloc` 副本 free |
| 错误路径资源释放（first_expr/filespec 各分支） | ✅ 完整（复核） |
| 新测试（6 + 集成） | ✅ 质量好 — 断言"不建文件"、内容回读；cleanup_files 隔离 |
| `tests/mini_open.c` | ⚠️ 观察项 1（调试残留） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| classic/new-style 字面量 filespec | ✅ 原有测试 + 全量 |
| classic/new-style 变量 filespec | ✅ 新单测 + 集成 |
| 拼接表达式 filespec | ✅ 新单测 + 集成 |
| reclen：R 接受 / O 拒绝 / 0 拒绝 | ✅ 新单测 + probe |
| LEN=：OUTPUT 拒绝 | ✅ 新单测 |
| eval 串所有权/堆 | ✅ 代码复核（无 ASAN；观察项） |
| 全量回归 | ✅ 82/82 |

---

## 四、改进建议

### 短期
1. 清理 `tests/mini_open.c` 调试残留（观察项 1）
2. ASM 对照 class filenum/reclen 上限校验（观察项 3）

### 中期
3. 交互 RUN smoke（v148 观察）
4. v145 图形观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
6. ASAN 构建（eval 串所有权/堆类问题）
7. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v151 观察项 2/3 全部修复 — OPEN filespec 支持字符串表达式（变量/拼接，ASM FRMEVL）、reclen 严格 RANDOM-only（O/I/A 拒绝且不建文件、R 接受、0 拒绝）；OPEN 提取为独立执行器；eval 串所有权（string heap 借用不 free）修正堆损坏且无泄漏；6 个新单测 + 集成；单测 3308/3308、0 警告、**82/82 全量 .bas 一致**。仅遗留 dev 调试文件与若干 ASM 边角观察项。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**（6 观察项）。
**建议**: 清理 `tests/mini_open.c`、ASM 对照 filenum/reclen 上限；继续 M2 下一批。

---

## 六、指标追踪

| 指标 | v151 | v152 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3287 | **3308** | ✅ +21（+6 测试函数） |
| 定义数 / 注册数 | 865 / 865 | **872 / 872** | +7（含 gwsts_exec_open 导出） |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 81/81 | **82/82 全量直接运行一致** | ✅ +1（open_varspec） |
| filespec 字符串表达式（观察项 2） | ❌ 不支持 | ✅ **修复**（变量/拼接） | 关闭 |
| reclen RANDOM-only（观察项 3） | ❌ 强制 RANDOM | ✅ **修复**（O/I/A 拒绝 + 0 拒绝） | 关闭 |
| eval 串所有权堆损坏 | ⚠️ | ✅ **修正**（借用不 free） | 关闭 |
| 观察项 | 6 | **6**（1 新增残留文件；2/3 关闭） | 更新 |
| M2 进度 | 2/3 + OPEN 语法 | **2/3 + OPEN 语义完整** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3308 → 82 个 .bas 分 6 批直接运行（expected 对比）→ filespec 集成 + 3 项 RECLEN probe
- 全程未使用脚本（分批命令链 + 内置工具）
- probe 产物已清理；`tests/mini_open.c` 为 dev 调试残留，已记录未删除（Reviewer 权限边界）
- **M2 状态**: OPEN 双形式 + 表达式 filespec + reclen 语义完整闭环