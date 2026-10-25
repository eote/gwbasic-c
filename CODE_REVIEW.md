# GW-BASIC C Port - Code Review v150

**日期**: 2026-10-26
**审查范围**: commit `5133c67`（fix: CODE_REVIEW v149 MEDIUM-1 — 经典 mode-first OPEN 语法 `OPEN "O"/"I"/"A"/"R", #n, file[,reclen]`，ASM GIO86 双形式）→ 工作区
**测试状态**: 单元测试 **3287/3287 通过**（0 失败，0 警告）✅ | .bas **81/81 全量直接运行**（输出与 expected 一致）✅ | OPEN 边界 probe ✅
**工作区状态**: clean（另有 dev 遗留未跟踪文件 `tests/A|I|O|R|X`，见观察项 1）
**总体评分**: A- — v149 MEDIUM-1 修复确认（双形式 OPEN、坏模式拒绝、小写归一、全量回归）；无阻塞问题，6 观察项

---

## 一、概述

本次审查 v149 MEDIUM-1 修复提交 `5133c67`（8 文件，+297/-9）。

### v149 MEDIUM-1 修复确认（全部验证通过）

| 验证项 | 实现 | 独立验证 |
|--------|------|----------|
| 经典 mode-first 解析 | `first_expr` 后为 `,` → classic：mode 单字符 O/I/A/R（大小写归一）→ mode；可选 `#n`；`filespec`（引号）；可选 `,reclen`（→ RANDOM） | ✅ `test_open_classic.bas`：`READ=LINE-ONE` / `ALL=LINE-ONELINE-TWO` / `OK` |
| 双形式共存 | 非 `,` 走原 new-style（`filespec FOR mode AS #n`）；原逻辑保留 | ✅ 全部文件 I/O 测试（print_file/file_seq/file_rand/line_input/put_get）一致 |
| 坏模式拒绝 | 非单字符 O/I/A/R → `ERROR`（ASM ERBFM0 "Bad file mode"），**不创建文件** | ✅ `OPEN "X", #1, ...` → `Error at line 10`；文件未创建 |
| 小写 mode | `"o"` 归一为大写 | ✅ `LOWER-OK`（写/关/KILL 全流程） |
| 资源释放 | classic 成功/失败路径 `first_expr`/`filespec` 均 free | ✅ 代码复核（无泄漏） |
| 新测试 | `test_gwsts_openmodes.c`（6 测试：O/I/A/R/bad-mode/lowercase）+ 集成 | ✅ 注册完整（3287/3287） |

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3287/3287 PASS**（3260 → 3287，+27*） |
| 编译警告 | ✅ **0 条** |
| **.bas 全量（81 个，分 6 批直接运行）** | ✅ **81/81 输出与 expected 一致**（含新 open_classic；terminating 用例 gwram/onerr_off 符合预期；无预期外 Error） |
| probe：坏模式 `"X"` | ✅ `Error at line 10` + 未创建文件 |
| probe：小写 `"o"` | ✅ `LOWER-OK` |

*注：+27 = 新增 `test_gwsts_openmodes.c` 6 个测试函数。

---

## 二、问题清单

### 🟥 CRITICAL

无（v149 MEDIUM-1 已修复确认）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项：

1. **dev 遗留未跟踪文件 `tests/A`、`tests/I`、`tests/O`、`tests/R`、`tests/X`**（均 0 字节，创建于 2026-10-25 05:15，提交前 14 分钟）——开发/实验 mode-first OPEN 时产生，**未清理、未提交**。建议删除并检查 `.gitignore`/测试临时文件规范，避免误 `git add` 入库。本版未删除（非 Reviewer 产物）。
2. **经典/新式 filespec 均仅支持引号字面量**：`OPEN "O", #1, F$`（变量文件名）→ `Error at line 20`（OPEN 未建立通道，后续 PRINT # 失败）；GW-BASIC 支持字符串表达式 filespec，建议 ASM GIO86 对照后决定实现或显式 SNERR。
3. **`reclen` 存在时强制 RANDOM**：`OPEN "O", #1, file, 128` → mode 被覆盖为 RANDOM → 后续 `PRINT #1` 失败（`Error at line 20`）。ASM 中 mode="O" + reclen 的语义需确认（忽略 reclen / Bad file mode / RANDOM）。
4. **经典 R 模式未验证 reclen 实际生效**：单测仅 OPEN/CLOSE；未覆盖 FIELD/GET/PUT 记录读写。
5. 交互 RUN 入口仍无自动化测试（v148 观察项，执行器同源）；`run_program_lines` 的 `exec_rc` 可简化。
6. v145 图形观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）仍开放。

---

## 三、架构评估

### 修复质量（5133c67）

| 决策 | 评价 |
|------|------|
| 首表达式后 `,` 判定 classic（ASM GIO86 L59-101 双形式） | ✅ 正确 — 与 new-style 无歧义分离 |
| mode 单字符校验 + 大小写归一 + ERBFM0 | ✅ 正确（坏模式不建文件） |
| 可选 `#n` / 引号 filespec / 可选 reclen | ✅ 正确（常规路径） |
| classic 成功/失败均 free（first_expr/filespec） | ✅ 无泄漏 |
| 新-style 原逻辑保留（块重构） | ✅ 全部文件 I/O 测试一致 |
| 6 单测（O/I/A/R/bad/lowercase）+ 集成 | ✅ 质量好；⚠️ R 的 reclen 未深度验证（观察项 4） |
| **reclen 强制 RANDOM / filespec 变量** | ℹ️ 待 ASM 对照（观察项 2/3） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| mode-first O（写/读回） | ✅ 单测 + 集成 |
| mode-first I（读） | ✅ 单测 + 集成 |
| mode-first A（追加） | ✅ 单测 + 集成 |
| mode-first R（OPEN/CLOSE） | ✅ 单测（⚠️ reclen 生效未验） |
| 坏模式 | ✅ 单测 + probe |
| 小写 mode | ✅ 单测 + probe |
| new-style 回归 | ✅ 81/81 全量 |
| **变量 filespec** | ❌ 观察项 2 |
| **mode O + reclen** | ❌ 观察项 3 |

---

## 四、改进建议

### 短期
1. 清理 `tests/A|I|O|R|X` 遗留文件；测试临时文件命名规范/清理（观察项 1）
2. ASM 对照：变量 filespec 与 mode+reclen 组合语义（观察项 2/3），按确认结果实现或显式 SNERR
3. 补 R 模式 reclen（FIELD/GET/PUT）测试（观察项 4）

### 中期
4. 交互 RUN smoke（沿用 v148 观察）
5. v145 图形观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句）
6. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
7. ASAN 构建
8. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v149 MEDIUM-1 正确修复 — `OPEN "O"/"I"/"A"/"R", #n, file[,reclen]` 经典语法实现（ASM GIO86 双形式），坏模式显式报错且不创建文件、小写归一、资源释放完整；6 个新单测 + 集成验证 O/I/A 全流程；单测 3287/3287、0 警告、**81/81 全量 .bas 一致**。遗留 dev 测试残留文件与 2 个语义观察项（filespec 变量、reclen 组合）待 ASM 对照。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**（6 观察项）。
**建议**: 清理残留文件、ASM 对照 filespec/reclen 语义、补 R 模式深度测试；随后继续 M2 下一批。

---

## 六、指标追踪

| 指标 | v149 | v150 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3260 | **3287** | ✅ +27（+6 测试函数） |
| 定义数 / 注册数 | 859 / 859 | **865 / 865** | +6 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 80/80 | **81/81 全量直接运行一致** | ✅ +1（open_classic） |
| 经典 mode-first OPEN | ❌ MEDIUM-1 | ✅ **修复**（O/I/A/R + 坏模式 + 小写） | 关闭 |
| 观察项 | 3 | **6**（含 filespec 变量 / reclen 组合 / R 深度 / 残留文件） | 更新 |
| M2 进度 | 2/3 + RUN 语义 | **2/3 + OPEN 语法修复** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3287 → 81 个 .bas 分 6 批直接运行（expected 对比）→ OPEN 边界 probe（坏模式/小写）
- 全程未使用脚本（分批命令链 + 内置工具）
- dev 残留 `tests/A|I|O|R|X`（0 字节，未跟踪）已记录，未删除（Reviewer 权限边界）
- **M2 状态**: OPEN 双形式语法闭环；filespec/reclen 边角待 ASM 对照