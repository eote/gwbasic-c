# GW-BASIC C Port - Code Review v130

**日期**: 2026-10-06
**审查范围**: commit `82c1a83`（feat: M2 第 1 批 — .bas 基线补齐 + ON..GOTO/GOSUB 语句，TDD 7 步）→ 工作区
**测试状态**: 单元测试 **3072/3072 通过**（0 失败）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过，首次全绿）** ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean
**总体评分**: B — ON..GOTO/GOSUB 核心语义与基线补齐质量高（72/72 首次全绿），但发现 **1 MEDIUM 关键字解析缺陷**（变量名含 GOTO/GOSUB 子串时表达式截断）及 3 LOW

---

## 一、概述

本次审查 M2 第 1 批提交 `82c1a83`（11 文件，+295/-57）：

| 变更 | 说明 |
|------|------|
| ON..GOTO/GOSUB 语句（ASM NTONG1/LOOPON L2446-2460） | `gwsts.c` ON 分支：表达式求值 + 目标列表解析 + 1-based 选择；越界/0 → SUCCESS（ASM L2453-2456 RET）；ON..GOSUB → `next86_gosub_to_line`（SETGSB，自 next86_gosub 重构共享） |
| .bas 基线补齐 | 审计确认全部已实现 .bas 均有 expected（修正 v129 备注）；补 test_gwdata（15 行）/test_gwram（14 行） |
| 两个长期跳过项转正 | `test_on_goto` / `test_onerr_off`：run_bas_tests.sh 新增 terminating-error 校验路径（rc=127 + expected 精确匹配）；NOT_IMPLEMENTED 清空 |
| 6 新单测 | `tests/test_gwsts_ongoto.c`：index1/2/3 + 越界 + gosub + zero |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3072/3072 PASS**（777 → 783，+6 ON 测试） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过（首次全绿）**；test_on_goto ✅、test_onerr_off ✅（expected terminating error, output match） |
| 孤儿检测（唯一函数名） | ✅ **783 定义 = 783 注册，0 孤儿 / 0 多余** |
| probe：ON I GOTO 1-based 选择 | ✅ `I=2` → `b2`（跳第 2 目标，正确） |
| probe：越界/0 | ✅ `I=9`（列表中 2 项）→ `fallthrough`（下一语句） |
| probe：ON..GOSUB + RETURN | ✅ `sub1` / `back from gosub`（跳转 + 返回） |
| probe：变量名含 GOTO 子串 | ❌ **MEDIUM-1 实锤**（见下） |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **ON 分支关键字搜索未锚定 token 边界**：`strstr(rest, "GOTO")` 在变量名内匹配到子串时表达式被错误截断，合法 GW-BASIC 程序行为错误 | `src/gwsts.c`（ON 分支 L710-717） |

**详情**:

```c
char *kw_goto = strstr(rest, "GOTO");   /* 首次出现即可 — 未检查是否为完整 token */
char *kw_gosub = strstr(rest, "GOSUB");
...
size_t expr_len = (size_t)(kw - rest);  /* 表达式 = 关键字前的文本 */
memcpy(expr_buf, rest, expr_len);       /* 若关键字在变量名内，这里截断 */
```

**实证**（review 期临时 probe，已清理）：
```basic
10 XGOTO = 2
20 ON XGOTO GOTO 100, 200     ' 合法 GW-BASIC：表达式 XGOTO=2 → 第 2 目标
30 PRINT "fallthrough"
...
200 PRINT "b2"
```
实际输出：`fallthrough`（应为 `b2`）。

**根因**：`strstr` 在 `"XGOTO GOTO 100, 200"` 的**偏移 1**（变量名 XGOTO 内）命中 "GOTO" → `expr_buf = "X"` → `gweval_eval_numeric("X")` = 0（未定义变量）→ 越界 → 不跳转。GW-BASIC 变量名允许含 "GOTO"/"GOSUB" 子串（XGOTO、AGOSUB、MYGOTO 等均合法）。

**连带子问题**：`ON XGOTO 100`（缺真关键字，语法错误）也会因误匹配变量内 "GOTO" 而被**静默接受**（index=0 → SUCCESS），应为 SNERR。

**修复建议**：关键字匹配需完整 token：
- 优先方案：从 `rest` **末尾向前**找最后一个 "GOTO"/"GOSUB"，并校验其前导字符非字母（数字/空白可接受，兼容 `ON 2GOTO` 紧凑）；
- 或先按表达式语法扫描至终止符（遇到空白+字母关键字），再确认关键字 token；
- 补测试：变量名含 GOTO/GOSUB 子串的 ON 语句（`ON XGOTO GOTO ...`）、缺关键字语句（`ON XGOTO 100` → SNERR）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **紧凑写法 `ON2GOTO 100` 不支持**：`starts_with("ON ")` 要求 "ON " 带空格，GW-BASIC 允许 `ON2GOTO100`（无空格 token 边界） | `src/gwsts.c` |
| 2 | **目标列表上限 64**：`targets[64]` 超出部分静默忽略；ASM NTONG1/JSY 无此硬限制，实际使用极少超 64，观察项 | `src/gwsts.c` |
| 3 | **terminating-error 校验路径外延**：`FAIL_SYNTAX + expected 匹配 → PASS` 适用于所有 IMPLEMENTED 测试；当前 72 项语义正确，但若未来某正常 .bas 回归为语法错误且 expected 恰含 "Error at line" 行，可能误判 PASS。建议仅对显式标记的 terminating 测试启用 | `run_bas_tests.sh` |

**详情**:

1. **LOW-1**：`ON 2 GOTO 100` ✅ 但 `ON2GOTO 100` 不进入 ON 分支（第 3 字符非空格）→ 后续分支不匹配 → 报错。与 RENUM/DELETE 分支（`starts_with("DELETE")` 无空格要求）不一致。建议：`starts_with("ON")` + 跳过空白 + 排除 `ON ERROR`（已排除）。
2. **LOW-2**：`while (*p && count < 64)` — 第 65 个目标起被忽略；index>64 时误判越界（SUCCESS）。低风险。
3. **LOW-3**：新路径设计合理（expected 即契约），但建议将 terminating 标记与普通测试区分（命名约定或注释），避免未来误判。

---

## 三、架构评估

### M2 第 1 批质量（82c1a83）

| 决策 | 评价 |
|------|------|
| ON..GOTO/GOSUB 1-based 选择；越界/0 → SUCCESS（ASM L2453-2456 RET） | ✅ 正确 — probe 实证（1-based 选择、越界继续） |
| ON..GOSUB → `next86_gosub_to_line`（SETGSB 重构共享） | ✅ 正确 — probe 实证（sub1 → RETURN → back）；重构无回归（网关 GOSUB 单测保持） |
| `next86_gosub` 重构为「解析 + 委托 to_line」 | ✅ 语义保持，共用路径 |
| 目标列表解析（数字/逗号/空白；非数字即止） | ✅ 常规场景正确 |
| **关键字 token 搜索（strstr 首次匹配）** | ⚠️ **MEDIUM-1** — 未锚定词边界 |
| .bas 基线全补齐 + 2 转正 + NOT_IMPLEMENTED 清空 | ✅ 优秀 — 72/72 首次全绿，0 跳过 |
| terminating-error 校验路径 | ✅ 设计合理（rc=127 + expected 精确匹配），范围需收敛（LOW-3） |
| 6 新单测（index1/2/3/越界/gosub/zero，自 init 隔离） | ✅ 隔离正确、断言明确；缺变量名含子串用例（MEDIUM-1 未被发现） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| ON..GOTO 1-based 选择（1/2/3） | ✅ 单测 + .bas |
| ON..GOTO 越界 / 0（不分支） | ✅ 单测 + probe |
| ON..GOSUB 跳转 + GOSUB 栈 | ✅ 单测（rc=100, g_curlin）+ .bas（RETURN 返回） |
| **变量名含 GOTO/GOSUB 子串** | ❌ **盲区 → MEDIUM-1** |
| 紧凑写法 ON2GOTO | ❌ LOW-1 |
| 目标 >64 | ❌ LOW-2 |
| 注册一致性 | ✅ 783=783，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期
1. **MEDIUM-1**：关键字 token 边界匹配（从后往前/词边界校验），补 `ON XGOTO GOTO ...` 与缺关键字 SNERR 用例
2. LOW-1：`starts_with("ON")` + 跳过空白（支持紧凑写法），保持与 RENUM/DELETE 分支一致
3. LOW-3：terminating-error 校验收窄到显式标记测试

### 中期（M2 规划）
4. ON ERROR 剩余语义（RESUME 各形式已有单测；`resume_next`/`resume_line` 套件 ✅）
5. 其他语句分支按 M2 里程碑推进（DATA/READ 已实现；剩余待实现语句审计）
6. CI 门禁落地（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
7. ASAN 构建
8. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: M2 第 1 批高质量完成 —— ON..GOTO/GOSUB 核心语义（1-based 选择/越界/0/GOSUB 栈）实现正确且 probe 实证，.bas 基线全补齐、两个长期跳过项转正、**套件 72/72 首次全绿**；单测 3072/3072、0 警告、0 孤儿。**但 ON 关键字搜索未锚定 token 边界（1 MEDIUM）：变量名含 GOTO/GOSUB 子串时表达式被错误截断，合法程序行为错误，需修复**。
**关键发现**: **1 MEDIUM**（strstr 关键字匹配截断表达式）、**3 LOW**（紧凑写法、targets 64 上限、terminating-error 路径外延）。
**建议**: 修复 MEDIUM-1（token 边界匹配 + 补用例）后继续 M2；该缺陷不阻塞其余 M2 并行推进。

---

## 六、指标追踪

| 指标 | v129 | v130 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3066 | **3072** | ✅ +6（+6 测试函数） |
| 定义数（唯一函数） | 777 | **783** | +6 |
| 注册数（run_test 调用） | 777 | **783** | +6 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 68/70 | **72/72（0 跳过，首次全绿）** | ✅ +4（+2 基线 + 转正 2） |
| NOT_IMPLEMENTED | 2 | **0（清空）** | ✅ |
| ON..GOTO/GOSUB | ⏳ 未实现 | ✅（含 MEDIUM-1 待修复） | 实现 |
| M1 状态 | 正式关闭 | **正式关闭** | 保持 |
| M2 进度 | — | **1/N 批**（ON..GOTO/GOSUB + 基线全补齐） | 启动 |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3072 → 套件 72/72 → 孤儿 783=783 → 5 项 probe（含 MEDIUM-1 实证，已清理）
- 72/72 全绿为项目**首次**（此前 2 个预期跳过项：test_on_goto/test_onerr_off 已转正）
- **M2 进度**: 第 1 批（ON..GOTO/GOSUB + .bas 基线全补齐）；MEDIUM-1 修复后继续下一批