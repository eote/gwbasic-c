# GW-BASIC C Port - Code Review v134

**日期**: 2026-10-12
**审查范围**: commit `11ea7cf`（fix: CODE_REVIEW v133 — MEDIUM-1 INSTR start 范围校验 + 2 LOW）→ 工作区
**测试状态**: 单元测试 **3094/3094 通过**（0 失败,0 警告）✅ | 孤儿 **802=802**（4 新测试全部注册）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过,本轮重跑确认）** ✅
**工作区状态**: clean（v133 报告已随 11ea7cf 入库）
**总体评分**: B- — API 层修复（start 校验/SNERR/probe 轻量化）与 1-arg SNERR 语句级行为正确，但发现 **2 MEDIUM**（FCERR 未传播到语句级、probe 轻量化分类歧义破坏数字首参 3-arg 形式）

---

## 一、概述

本次审查开发者对 v133 修复提交 `11ea7cf`（7 文件，+199/-98）。

### v133 修复确认

| v133 问题 | 修复方式（11ea7cf） | 独立验证 |
|-----------|--------------------|----------|
| 🟨 MEDIUM-1 start 范围 | `bistrs_instr` 前置 `start<1 || start>255 → -1`（注释说明 ASM 字节语义与 port 长串差异） | ⚠️ **API 层正确**（单测 + 代码复核），但**语句级 FCERR 未传播** — 见新 MEDIUM-1 |
| 🟩 LOW-1 非法形式静默 | dispatch 失败路径显式 `gwdata_set_error(2)`（1-arg / S2 解析失败） | ✅ probe `INSTR(A$)` → `Error at line 20`（SNERR 生效，不再静默 0） |
| 🟩 LOW-2 probe 分配浪费 | 轻量化 peek（首非空白字符：quote→2-arg，digit/'('→3-arg） | ⚠️ 消除分配浪费，但**引入首字符分类歧义** — 见新 MEDIUM-2 |
| 4 新单测 | negative_start / start_too_large / one_arg_snerr / start_range | ✅ 全部注册（802=802），断言 API/error-code 正确 |

### 验证结果（独立执行，当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0，**0 编译警告** |
| 单元测试 | ✅ **3094/3094 PASS** |
| 孤儿检测（grep 式核验） | ✅ 新增 4 测试 extern+run_test 均注册（v133 798 + 4 = 802，0 孤儿） |
| probe：`ON INSTR(A$, ":")`（回归） | ✅ `b2`（引号感知 + 2-arg 正常） |
| probe：`INSTR(A$)` 1-arg（LOW-1 回归） | ✅ `Error at line 20`（SNERR） |
| probe：`X = INSTR(-1, "ABC", "B")` | ❌ `X=-1`（无错误消息）— **MEDIUM-1（新）实证** |
| probe：`X = INSTR(N, "ABC", "B")`（N=2，数字变量 start） | ❌ `Error at line 20` — **MEDIUM-2 实证** |
| probe：`X = INSTR(LEN(S$), "ABC", "B")`（数字函数 start） | ❌ `Error at line 20` — **MEDIUM-2 实证** |

*.bas 集成套件本轮已重跑确认：**72/72 全 PASS（0 失败,0 跳过）**，与 v133 基线一致，无回归。

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **start 非法时 FCERR 未传播到语句级**：`X = INSTR(-1, ...)` / `INSTR(300, ...)` 静默赋 -1，无 "Error at line"（应报 Illegal function call） | `src/gweval.c`（INSTR 3-arg dispatch） |
| 2 | **probe 轻量化分类歧义（反修复回归）**：字母开头的 token 一律判为"字符串 2-arg"，数字变量/数字函数 start 的 3-arg 形式被错判 → SNERR | `src/gweval.c`（peek 分类） |

**详情**:

1. **MEDIUM-1 — FCERR 传播缺失**：
   ```c
   /* 3-arg 分支 */
   int pos = bistrs_instr(str_val, s2, (int)start);
   return (float)pos;              /* -1 → 返回 -1 数值，但未 gwdata_set_error! */
   ```
   `bistrs_instr` 返回 -1（FCERR）后 dispatch 直接返回数值；与 LOW-1 修复（显式 `gwdata_set_error(2)`）不对称。**实证**：`X = INSTR(-1, "ABC", "B")` → 输出 `X=-1`，无 `Error at line`。单测 `test_gweval_instr_start_range` 只断言 dispatch 返回值 -1（API 层），未覆盖语句级错误传播。**修复**：dispatch 检测 `pos < 0` 时 `gwdata_set_error(5)`（FCERR），补语句级单测（eval 后断言 error code + 行为）。

2. **MEDIUM-2 — peek 首字符分类歧义**：
   ```c
   if (first == '"' || isalpha(first) || first == '$' || first == '_') => 2-arg
   else => 3-arg
   ```
   `isalpha(first)` 无法区分"字符串变量 `A$`/字符串函数 `LEFT$(...)`"与"数字变量 `N`/数字函数 `LEN(...)`"。**实证**：
   - `X = INSTR(N, "ABC", "B")`（N=2，GW-BASIC 中循环动态 start 的**常用写法**）→ SNERR（应 X=2）
   - `X = INSTR(LEN(S$), "ABC", "B")` → SNERR（应 X=2）
   v132 的完整 `parse_string_expression` probe 本可正确区分（字符串解析成功→2-arg；失败→3-arg）；本轮 LOW-2 轻量化在该场景回退。**修复建议**（三选一）：
   - ① 字母开头时回退完整 parse probe（仅失败时零分配，多数调用无浪费）；或
   - ② 细粒度 token 分类：变量名看 `$` 后缀；函数名查字符串函数表（LEFT$/MID$/RIGHT$/CHR$/STR$/SPACE$/STRING$/UCASE$/LCASE$/LTRIM$/RTRIM$ → 字符串，其余 → 数字）；或
   - ③ 按 2-arg 解析失败后自动回退 3-arg 重试（代价最小时通用）。
   补单测：`INSTR(N, S1$, S2$)`（数字变量 start）、`INSTR(LEN(S$), ...)`（数字函数 start）、`INSTR(2, ...)`（数字字面量，现有）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无新增。

（v133 的 LOW 均已处理：LOW-1 修复确认；LOW-2 引入的副作用归入 MEDIUM-2。）

---

## 三、架构评估

### 修复质量（11ea7cf）

| 决策 | 评价 |
|------|------|
| `bistrs_instr` 前置 `start<1\|\|start>255 → -1`（ASM 字节语义注释） | ✅ API 层正确 |
| dispatch 失败路径 SNERR（LOW-1） | ✅ 语句级生效（probe `Error at line 20`） |
| **FCERR 传播**（dispatch 未 set_error） | ⚠️ MEDIUM-1 — 与 SNERR 不对称 |
| **peek 轻量化分类**（LOW-2） | ⚠️ MEDIUM-2 — 消除分配浪费但破坏数字首参 3-arg |
| 4 新单测（API 2 + 语句级 2） | ✅ 注册完整（802=802）；⚠️ 缺语句级 FCERR 传播与数字变量 start 用例 |
| 赋值分支排除/引号感知（前序修复）无回归 | ✅ probe 回归 b2 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| INSTR 2-arg / 3-arg（字面量 start）/ start=0 / 范围（API） | ✅ |
| INSTR 1-arg → SNERR（语句级） | ✅ 修复 + 单测 |
| **语句级 FCERR 传播（INSTR(-1) 赋值）** | ❌ MEDIUM-1（单测仅 API 断言） |
| **数字变量 start（INSTR(N,...)）** | ❌ MEDIUM-2 |
| **数字函数 start（INSTR(LEN(S$),...)）** | ❌ MEDIUM-2 |
| 注册一致性 | ✅ 802=802，0 孤儿 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：dispatch 3-arg 分支 `pos < 0 → gwdata_set_error(5)`；补 `X = INSTR(-1,...)` 语句级错误传播单测
2. **MEDIUM-2**：字母开头回退完整 parse probe（或细粒度 token 分类 / 2-arg 失败回退 3-arg）；补 `INSTR(N, S1$, S2$)`、`INSTR(LEN(S$), ...)` 单测

### 中期（M2 规划）
3. 其余字符串函数（LEFT$/MID$/RIGHT$/CHR$/STR$/SPACE$）做同型"首参类型判定/错误传播"一致性审计
4. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）
5. .bas 套件补 INSTR 集成场景（含数字变量 start）

### 长期
6. ASAN 构建
7. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: v133 修复在 API 层正确（start 范围前置校验、SNERR 显式化、probe 轻量化），1-arg 非法形式语句级生效（probe `Error at line 20`），回归 `ON INSTR(A$, ":") → b2` 保持；单测 3094/3094、0 警告、0 孤儿。**但发现 2 MEDIUM**：① FCERR 传播缺失（`INSTR(-1,...)` 静默赋 -1，应报 Illegal function call）；② probe 轻量化的首字符分类歧义破坏数字变量/数字函数 start 的 3-arg 形式（`INSTR(N, S1$, S2$)` 是 GW-BASIC 常用写法）。
**关键发现**: **2 MEDIUM**（FCERR 传播、peek 分类回归）、0 HIGH、0 CRITICAL。
**建议**: dispatch 补 FCERR set_error；字母开头的形式判定回退完整解析（或更细 token 分类）；补对应语句级单测与 .bas 集成。修复后 M2 继续推进。

---

## 六、指标追踪

| 指标 | v133 | v134 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3088 | **3094** | ✅ +6（+4 测试函数） |
| 定义数（唯一函数） | 798 | **802** | +4 |
| 注册数（run_test 调用） | 798 | **802** | +4 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72 | **72/72（本轮重跑确认）** | 保持 |
| INSTR start 范围（API） | ❌ | ✅ **修复** | 关闭 |
| INSTR 1-arg SNERR | ❌ | ✅ **修复** | 关闭 |
| **INSTR 语句级 FCERR 传播** | — | ❌ **MEDIUM-1** | 新发现 |
| **INSTR 形式判定（数字首参）** | — | ❌ **MEDIUM-2** | 新发现（LOW-2 副作用） |
| M2 进度 | 1/N + 3 修复轮 | **1/N + 4 修复轮** | — |

**注释**:
- 独立验证：干净重建 → 单测 3094 → 孤儿核验 802=802 → 5 项 probe（2 回归 + 3 边界，已清理）
- \*.bas 套件本轮已重跑确认（72/72 全 PASS），与 v133 基线一致
- MEDIUM-2 为 LOW-2（probe 轻量化）的副作用：消除每次调用的分配浪费，但首字符无法区分字符串/数字变量与函数
- **M2 状态**: ON..GOTO/GOSUB + INSTR 主体完成；FCERR 传播与形式判定修复后进入下一批