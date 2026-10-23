# GW-BASIC C Port - Code Review v147

**日期**: 2026-10-23
**审查范围**: 用户报告的交互 RUN 缺陷（`FOR/NEXT` 只执行一次）+ `f1c95e9`（v145 MEDIUM-1，v146 已复核）→ 工作区
**测试状态**: 单元测试 **3242/3242 通过**（0 失败，0 警告）✅ | .bas 文件路径 **13/13 一致** ✅ | **交互 RUN 路径实测失效（CRITICAL-1）** ❌
**工作区状态**: clean
**总体评分**: C — 文件执行路径全绿（v145 MEDIUM-1 修复确认），但发现 **CRITICAL-1：交互 RUN 执行路径（第三份独立循环）缺失全部控制流支持**，FOR/NEXT/GOTO/END/IF..THEN 等核心语句在交互模式下失效，且运行时 DELETE 存在 UAF 回归面

---

## 一、概述

本版聚焦用户报告的 bug：交互模式输入程序后 `RUN`，`FOR I=1 TO 10 ... NEXT` 只输出 `1`。

**复现**（stdin 管道驱动交互模式，未使用脚本）：

| 场景（交互 RUN） | 实测 | GW-BASIC 期望 |
|------------------|------|----------------|
| `10 for i=1 to 10 / 20 print i / 30 next` | **仅 ` 1` 后结束（无错误）** | `1..10` |
| `10 FOR I=1 TO 3 / 20 PRINT I / 30 NEXT I`（大写） | 仅 ` 1` | `1 2 3` |
| `10 END / 20 PRINT "AFTER"` | **`Error at line 10`** | 正常结束 |
| `10 GOTO 30 / 20 PRINT "SKIP" / 30 PRINT "TARGET"` | **`Error at line 10`** | `TARGET` |
| `10 IF 1=1 THEN 30 / 20 ... / 30 PRINT "TARGET"` | **`Error at line 10`** | `TARGET` |
| `10 DELETE 20 / 15 PRINT "A" / 20 PRINT "B" / 30 PRINT "C"` | `A` / `C`（未崩溃，但存在悬垂指针读） | `A` / `C`（无 UB） |

**对照（文件路径，同一代码）**：四种 FOR 组合（大写/小写、`NEXT I`/无变量 `NEXT`）经 `gwbasic.exe file.bas` **全部正确输出 1..10**。→ 缺陷**仅存在于交互 RUN 路径**。

### v146 复核结论（保留）

`f1c95e9`（v145 MEDIUM-1）已正确修复：`next86_for` 无后续行返回 END sentinel(-2)，0 次循环 + NEXT 最后行不再执行体（probe 空输出）；双层级单测 + 空 expected 集成；单测 3242/3242、13 个 .bas 文件用例与 expected 一致。

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 |
|---|------|------|
| 1 | **交互 RUN 执行路径是未升级的第三份独立循环**：缺少 `g_curlin` 设置、END(-2)、跳转(rc>0)、ON ERROR、liveness 重定位 — FOR/NEXT/GOTO/GOSUB/RETURN/ON..GOTO/IF..THEN/RESUME 全部失效；运行时 DELETE 存在 UAF 读 | `src/gwmain.c`（RUN 命令分支，L166-211） |

**详情**:

**代码对比**（同一 `gwmain.c` 内两条执行路径）：

```c
/* 交互 RUN 路径（L166-211）——简化版，从未升级 */
int rc = gwsts_exec_statement(curr->text);
if (rc != 0) {                              /* ← 任何非 0 都当错误 */
    printf("Error at line %d\n", line_num);
    break;
}
curr = saved_next;                          /* ← 无跳转/END/重定位 */

/* .bas 文件路径（L280+，v126/v145 已升级） */
g_curlin = curr->number;                    /* ← FOR/GOTO/RESUME/ON ERROR 依赖 */
g_state->current_line = curr->number;
...
if (gwdata_get_error() != 0) { ...ON ERROR / handler-FATAL... }
if (rc == -2) break;                        /* END */
if (rc < 0) { ...错误... }
else if (rc > 0) {                          /* 跳转目标 */
    ... curr = gwlist_find_line(rc) ...     /* GOTO/ON..GOTO/IF..THEN/FOR-NEXT 继续 */
}
/* liveness 重定位（v126 DELETE 运行时修复） */
```

**根因链（FOR/NEXT 只 1 次）**：
1. RUN 路径执行 FOR 前**未设置 `g_curlin`**（文件路径设了）；
2. `next86_for` 依赖 `g_curlin` 扫描匹配 NEXT、计算 `for_next_line` → 在错误行号下计算失效；
3. NEXT 首次"继续"返回的行号不可用/为 0 → 返回 0；
4. RUN 路径 `rc == 0 → curr = saved_next` → 循环体下一行已无（或直接结束）→ **只输出 1、无错误**。

**同根连带缺陷**（RUN 路径逐项缺失）：
- `rc == -2`（END）→ 被当错误 → `10 END` 报 `Error at line 10`
- `rc > 0`（GOTO/GOSUB/RETURN/ON..GOTO/IF..THEN/WHILE 跳转/FOR 继续）→ 被当错误 → `GOTO`/`IF..THEN` 报 `Error at line 10`
- ON ERROR 不生效（无 `gwdata_get_error()` 处理）
- 无 liveness 重定位 → 运行时 DELETE 后 `curr = saved_next` 可能指向已释放节点（实测未崩溃属 UB 侥幸；v126 修复的 UAF 在 RUN 路径未生效）

---

## 三、测试盲区分析（为什么逃过测试）

**测试矩阵实际只覆盖了执行路径中的 1.5 条**：

| 测试层 | 覆盖的执行路径 | 缺口 |
|--------|----------------|------|
| 单元测试（3242） | 直接调用 `next86_for/next`、`gwsts_exec_statement` | 测试内**手动设置 `g_curlin`**（如 `test_next86_firstiter.c: g_curlin = 10`）→ **掩盖了"调用方未设置 g_curlin"**；不经过 main 的 RUN 分支 |
| .bas 集成（run_bas_tests.sh，77 个） | `gwbasic.exe file.bas` → **文件加载路径**（完整控制流） | 不覆盖交互 RUN；所有控制流测试（for_edges/for_lastnext/goto/on_goto/if_else/renum/delete 等）都在文件路径 |
| 交互 RUN 路径（L166-211） | **零自动化覆盖** | 无 stdin 驱动 RUN 的测试；历次 review 的 probe 也全部使用 `gwbasic.exe xxx.bas`（文件路径） |

**结论**：这不是某个用例缺失，而是**执行路径级测试盲区** — 两条执行循环分叉后，只有文件路径被持续测试与修复（v126 重定位、v145 END sentinel 等均只改文件路径 + 单测），RUN 路径停留在旧实现。单测的 `g_curlin` 预置进一步掩盖了调用方契约。

**Review 流程改进**：历次 review 的验证方式（文件路径 + 单测）本身不覆盖用户主交互路径；后续必须包含交互 RUN end-to-end。

---

## 四、架构评估

### 执行循环分叉

| 决策 | 评价 |
|------|------|
| 存在两份执行循环（RUN 命令 / 文件加载） | ❌ **根本问题** — 重复实现导致修复不同步（v126/v145 只修文件路径） |
| 文件路径完整（g_curlin/END/跳转/ON ERROR/liveness/DEBUG） | ✅ 正确 |
| RUN 路径简化（rc!=0 即错误、无 g_curlin） | ❌ **CRITICAL-1** |
| 单测直接调用 + 手动 g_curlin | ⚠️ 掩盖调用方契约 |

### 测试覆盖

| 功能 | 文件路径 | 交互 RUN |
|------|----------|----------|
| FOR/NEXT（大小写/无变量/0 次/最后行） | ✅ | ❌ 只 1 次 |
| GOTO / ON..GOTO / IF..THEN 跳转 | ✅ | ❌ Error |
| GOSUB/RETURN | ✅ | ❌ Error（rc>0） |
| END | ✅ | ❌ Error（-2） |
| ON ERROR / RESUME | ✅ | ❌ 未处理 |
| 运行时 DELETE（liveness/UAF） | ✅ | ❌ 悬垂读 UB |

---

## 五、改进建议

### 短期（阻塞项）
1. **消除执行循环分叉（首选）**：抽取单一 `run_program_lines()`（以文件路径 L280+ 为蓝本，含 g_curlin/current_line、END(-2)、rc>0 跳转、ON ERROR/handler-FATAL、liveness 重定位、DEBUG）；`RUN` 命令与 `main` 文件执行**共用**该函数，删除 RUN 分支的简化循环。**这一处改动可同时修复 FOR/NEXT/GOTO/GOSUB/ON..GOTO/IF..THEN/END/ON ERROR/DELETE-UAF 全部分支**。
2. 若暂不重构（不推荐）：逐项将 RUN 路径升级到与文件路径同构（g_curlin、-2、>0、ON ERROR、liveness），并加注释说明两份逻辑必须同步。
3. **补交互 RUN 自动化测试**（关键）：
   - 端到端：stdin 驱动（管道/测试 harness）覆盖 `FOR 1..3`、`GOTO`、`GOSUB/RETURN`、`ON..GOTO`、`IF..THEN`、`END`、`RESUME`、`ON ERROR`、运行时 `DELETE`、大小写与无变量 `NEXT`；
   - 重构后：单测直接调用统一执行器（构造行列表），另加 main 级 smoke（RUN FOR 1..10 → 10 行输出）。
4. 单测约定：`next86_for/next` 调用前必须有效 `g_curlin`（在入口加 assert/文档），并新增"未设置 g_curlin"回归用例以防调用方契约漂移。

### 中期
5. CI 门禁：交互 RUN smoke（FOR 1..10 输出 10 行）必须纳入；两条入口共用执行器后，文件/交互行为一致性由同一测试集保证。
6. v145 观察项（PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句等）随批次处理。

### 长期
7. ASAN 构建（RUN 路径 DELETE UAF 可被捕获）
8. 定期文件 + 交互双路径回归

---

## 六、总结

**总体评价**: 文件执行路径保持全绿（v145 MEDIUM-1 修复确认、3242 单测、13 个 .bas 一致），但**用户日常使用的交互 RUN 路径是第三份未升级的执行循环**：未设置 `g_curlin`、把 END(-2) 与跳转(rc>0) 当错误、无 ON ERROR、无 DELETE liveness → FOR/NEXT 只执行一次、GOTO/END/IF..THEN 直接报错、运行时 DELETE 存在 UAF 读。**测试未发现的根因是执行路径级盲区**：单测手动设置 `g_curlin` 并直接调用语句函数，集成测试只跑文件加载路径，交互 RUN 零覆盖。
**关键发现**: **1 CRITICAL**（交互 RUN 控制流全面失效 + UAF 回归面）、0 HIGH、0 MEDIUM。测试盲区：执行路径分叉未同步 + 交互入口零测试。
**建议**: 统一为单一执行器（首选，一次修复全部分支）+ 交互 RUN 端到端测试 + CI 双路径 smoke；修复后进行 v148 review。

---

## 七、指标追踪

| 指标 | v146 | v147 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3242 | **3242** | 保持（未新增测试） |
| 定义数 / 注册数 | 853 / 853 | **853 / 853** | 保持 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 文件路径 | 13/13 一致 | **13/13 一致** | 保持 |
| **交互 RUN 路径** | 未覆盖 | ❌ **CRITICAL（FOR/GOTO/END/IF..THEN 失效；DELETE UB）** | 新发现 |
| v145 MEDIUM-1（FOR 最后行） | ✅ 修复确认 | ✅ 保持（文件路径） | 保持 |
| 测试架构 | 文件路径 + 单测 | **暴露执行路径级盲区（交互零覆盖）** | 需补强 |

**注释**:
- 复现方式：stdin 管道驱动交互模式（单条命令，未使用脚本）；文件路径对照确认缺陷仅限交互 RUN
- 历次 review/probe 均使用文件路径（`gwbasic.exe xxx.bas`）与直接函数单测 → 未覆盖用户主交互路径
- **修复优先级高**：交互 RUN 是主要使用方式；统一执行器可一次性消除全部分支差异并关闭 UAF 回归面