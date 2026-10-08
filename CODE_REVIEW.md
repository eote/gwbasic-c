# GW-BASIC C Port - Code Review v131

**日期**: 2026-10-07
**审查范围**: commit `8fb1496`（fix: CODE_REVIEW v130 — MEDIUM-1 ON 关键字 token 边界 + 3 LOW）→ 工作区
**测试状态**: 单元测试 **3075/3075 通过**（0 失败）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过）** ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean（v130 报告已随 8fb1496 入库）
**总体评分**: B- — v130 的单语句场景修复回归正确（probe + 3 新单测），但发现 **1 MEDIUM**（ON 关键字解析无语句边界概念：多语句行/REM 注释污染）及 2 LOW

---

## 一、概述

本次审查开发者对 v130 修复提交 `8fb1496`（6 文件，+243/-114）。

### v130 修复确认（单语句场景全部验证通过）

| v130 问题 | 修复方式（8fb1496） | 独立验证 |
|-----------|--------------------|----------|
| 🟨 MEDIUM-1 关键字截断 | 关键字搜索改**从后向前**（最后出现）+ 单字符边界校验（前导 letter/_/$ → 变量名内拒绝 → SNERR；digit → 紧凑接受）；3 新单测 | ✅ probe `ON XGOTO GOTO 100,200`（XGOTO=2）→ `b2`；`test_gwsts_on_goto_var_with_goto_substring/missing_keyword/compact` 全过 |
| 🟩 LOW-1 紧凑写法 | `starts_with("ON")` + 跳过空白（与 RENUM/DELETE 一致） | ✅ `ON 2GOTO 100,200` → 200（单测 compact） |
| 🟩 LOW-2 targets[64] | 保留观察（未修） | — |
| 🟩 LOW-3 terminating-error | 显式 `TERMERROR_TESTS` 列表 + `run_test 'termerror'` 参数；普通测试 FAIL_SYNTAX 仍 FAIL | ✅ 代码复核：test_onerr_off 在列表中，路径收窄正确 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3075/3075 PASS**（783 → 786，+3 测试） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过**；test_onerr_off ✅（termerror 输出匹配） |
| 孤儿检测（唯一函数名） | ✅ **786 定义 = 786 注册，0 孤儿 / 0 多余** |
| probe：`ON XGOTO GOTO ...`（v130 回归） | ✅ `b2`（修复正确） |
| probe：**多语句行** `ON X GOTO 100 : GOTO 500`（X=1） | ❌ `b500`（应 `b100`）— **MEDIUM-1 实证** |
| probe：**REM 注释** `ON X GOTO 100 : REM GOTO later`（X=1） | ❌ `after on`（应 `b100`）— **MEDIUM-1 实证** |
| probe：**变量名 GOTO 开头** `ON GOTOX 100, 200` | ❌ `fallthrough`（应 SNERR）— **LOW-1 实证** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **ON 关键字搜索与目标解析无语句边界概念**：多语句行（冒号）与 REM 注释中的 GOTO/GOSUB 字样污染 ON 解析，导致错误跳转或吞掉后续语句 | `src/gwsts.c`（ON 分支） |

**详情**:

**实证 1**（多语句行，行内冒号为 GW-BASIC 标准特性）：
```basic
10 X = 1
20 ON X GOTO 100 : GOTO 500
30 PRINT "after on"
...
100 PRINT "b100"
500 PRINT "b500"
```
实测输出：**`b500`**（GW-BASIC 期望：`b100` — ON 语句目标列表以冒号结束）。

**实证 2**（REM 注释）：
```basic
20 ON X GOTO 100 : REM GOTO later
```
实测输出：`after on`（期望 `b100` — REM 注释中的 "GOTO" 被当作关键字）。

**根因**：`gwmain.c` 执行模型**无冒号多语句拆分**（每行 text 整体传给 `gwsts_exec_statement`）；ON 分支：
1. 从后向前找**最后一个** GOTO/GOSUB → 在多语句行中命中**后续语句**的关键字（": GOTO 500"）或 REM 注释中的字样
2. 目标列表从该（错误）关键字之后解析 → `500` 成为唯一目标 → X=1 → 跳 500（实证 1）；或解析到注释内容 → count=0 → 不跳（实证 2）

**修复建议**：关键字搜索与目标列表解析需止于**语句边界**（冒号 `:` / 行尾）：
- 先按冒号切出当前语句（与执行模型对齐），再在语句片段内解析；或
- 关键字搜索时跳过 REM 注释段（`:` 后遇 REM 即语句结束）；
- 补测试：`ON X GOTO 100 : GOTO 500`、`ON X GOTO 100 : REM GOTO later`、`ON X GOTO 100 : PRINT "X"`（无 GOTO 字样的多语句应正常）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **`kw == rest` 时跳过边界校验**：变量名以 GOTO/GOSUB 开头（GOTOX）且语句缺真关键字 → 静默接受（应 SNERR） | `src/gwsts.c`（L722 `if (kw && kw > rest)`） |
| 2 | **观察**：`gweval_eval_numeric` 宽松解析（`"X GOTO 100 : "` 解析 X=1 后忽略垃圾），与 RENUM 分支共用；表达式后有垃圾被静默接受 | `src/gweval.c`（既有行为，观察） |

**详情**:

1. **LOW-1**：`ON GOTOX 100, 200`（GOTOX=2，缺真关键字）实测 `fallthrough`（无报错）。根因：唯一 "GOTO" 在 **rest 起始位置**（变量 GOTOX 内）→ `strncmp` 命中 `kw == rest` → 边界校验条件 `kw > rest` 为假被跳过 → expr 长度 0 → `eval_numeric("")` = 0 → index 0 → SUCCESS。修复：`kw == rest` 也应拒绝（"ON GOTO 100" 缺表达式、"ON GOTOX ..." 变量名前缀，两者均非法 → SNERR）。补测试：`test_gwsts_on_goto_keyword_at_start_missing_expr`（缺表达式/行首关键字非法场景）。
2. **LOW-2**：eval_numeric 的宽松部分解析放大了 MEDIUM-1（表达式 "X GOTO 100 : " 解析 X=1 成功而非报错）。不单独修复，随 MEDIUM-1 语句边界修复后不再暴露。

---

## 三、架构评估

### 修复质量（8fb1496）

| 决策 | 评价 |
|------|------|
| 从后向前找最后出现 + 单字符边界校验（letter/_/$ 拒绝，digit 接受） | ✅ 单语句场景正确（v130 MEDIUM-1 回归 probe 通过） |
| 边界校验只查 `kw[-1]` 单字符 | ⚠️ LOW-1 — `kw == rest`（行首命中）排除在校验外 |
| 3 新单测（var_with_goto_substring / missing_keyword / compact） | ✅ 精准对应 v130 问题，自 init 隔离 |
| `starts_with("ON")` + 空格式兼容 | ✅ LOW-1（v130）修复 |
| TERMERROR_TESTS 显式列表 + 'termerror' 参数 | ✅ LOW-3（v130）收窄正确 |
| **关键字搜索/目标解析作用整行 text（无语句边界）** | ⚠️ **MEDIUM-1** — 多语句行/REM 污染 |
| targets[64] | ℹ️ 保留观察（LOW-2 v130 未修，接受） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| ON..GOTO 单语句（1-based/越界/0/紧凑/变量名字串/SNERR） | ✅ 覆盖完整 |
| **多语句行（冒号）+ ON..GOTO** | ❌ **盲区 → MEDIUM-1** |
| **REM 注释含 GOTO 字样 + ON..GOTO** | ❌ **盲区 → MEDIUM-1** |
| 变量名以关键字开头 + 缺真关键字 | ❌ **盲区 → LOW-1** |
| 注册一致性 | ✅ 786=786，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：ON 分支引入语句边界（按冒号提取当前语句或跳过 REM 段）；补 3 个多语句/注释单测
2. **LOW-1**：`kw == rest` 一律拒绝 → SNERR（缺表达式/行首关键字变量名均非法），补测试

### 中期（M2 规划）
3. 验证 ON..GOSUB 在多语句行的行为（修复后同步覆盖）
4. eval_numeric 宽松解析观察项（与 RENUM 共用；MEDIUM-1 修复后再评估是否需收紧）
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
6. ASAN 构建
7. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: v130 修复在单语句场景正确（回归 probe + 3 新单测全绿）—— 变量名含 GOTO 子串不再截断、紧凑写法支持、terminating-error 收窄；**但 ON 关键字解析仍在整行 text 上无语句边界概念（1 MEDIUM）**：多语句行 `ON X GOTO 100 : GOTO 500` 误跳 500、REM 注释含 GOTO 时语句失效，均为 GW-BASIC 标准行内特性，需修复。另 1 LOW（`kw==rest` 跳过校验 → 静默接受）。
**关键发现**: **1 MEDIUM**（语句边界缺失）、**2 LOW**（kw==rest 校验缺口、eval 宽松解析观察）。
**建议**: ON 分支按语句边界（冒号/REM 段）解析，`kw==rest` 拒绝，补对应单测；修复后 M2 继续推进。

---

## 六、指标追踪

| 指标 | v130 | v131 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3072 | **3075** | ✅ +3（+3 测试函数） |
| 定义数（唯一函数） | 783 | **786** | +3 |
| 注册数（run_test 调用） | 783 | **786** | +3 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72（首次全绿） | **72/72** | 保持 |
| ON..GOTO 关键字解析（变量名字串） | ❌ MEDIUM-1（v130） | ✅ 修复 | 关闭 |
| ON..GOTO **语句边界**（冒号/REM） | — | ❌ **MEDIUM-1** | 新发现 |
| M2 进度 | 1/N | **1/N + 修复轮** | — |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3075 → 套件 72/72 → 孤儿 786=786 → 4 项 probe（1 回归 + 3 新场景，已清理）
- MEDIUM-1 与 v130 同根（关键字解析在整行文本上），v130 修复了"字符串内部"（变量名）但未覆盖"语句边界"（冒号/注释）
- **M2 状态**: 第 1 批 ON..GOTO/GOSUB 主体完成；语句边界缺陷修复后进入下一批