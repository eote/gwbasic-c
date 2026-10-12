# GW-BASIC C Port - Code Review v135

**日期**: 2026-10-12
**审查范围**: commit `c85284e`（fix: CODE_REVIEW v134 — MEDIUM-1 FCERR 语句级传播 + MEDIUM-2 形式判定回退）→ 工作区
**测试状态**: 单元测试 **3102/3102 通过**（0 失败,0 警告）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过）** ✅ | 孤儿 **805=805** ✅
**工作区状态**: clean（v134 报告已随 c85284e 入库）
**总体评分**: A（优秀）— v134 的 2 个 MEDIUM 全部修复验证通过，6 项 probe 全绿，**0 新问题**

---

## 一、概述

本次审查开发者对 v134 修复提交 `c85284e`（5 文件，+258/-134）。

### v134 修复确认（全部验证通过）

| v134 问题 | 修复方式（c85284e） | 独立验证 |
|-----------|--------------------|----------|
| 🟨 MEDIUM-1 FCERR 未传播 | dispatch 3-arg 分支 `pos<0 → gwdata_set_error(5)`（Illegal function call）+ return 0 | ✅ probe `X = INSTR(-1, "ABC", "B")` → **`Error at line 10`**（不再静默 X=-1） |
| 🟨 MEDIUM-2 形式判定歧义 | 三级分类：`"` → 2-arg（廉价无歧义）；`digit/'('/'-'/'+'` → 3-arg（廉价无歧义）；**字母开头 → 回退完整 parse_string_expression probe**（成功→2-arg，失败→rewind+3-arg） | ✅ probe `INSTR(N, ...)` → `X= 2`；`INSTR(LEN("BC")+1, "ABC", "C")` → `X= 3`（数字变量/函数 start 均恢复） |
| 3 新单测 | numeric_var_start / numeric_func_start / stmt_fcerr；start_range 更新为 ERR=5 断言 | ✅ 注册完整（805=805） |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3102/3102 PASS**（802 → 805，+3 测试） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过** |
| 孤儿检测（唯一函数名） | ✅ **805 定义 = 805 注册，0 孤儿 / 0 多余** |
| probe：`INSTR(-1, ...)`（MEDIUM-1） | ✅ `Error at line 10`（FCERR 传播） |
| probe：`INSTR(N, "ABC", "B")`（N=2，数字变量 start） | ✅ `X= 2`（3-arg 恢复） |
| probe：`INSTR(LEN(S$), "ABC", "B")`（数字函数 start） | ✅ `X= 0`（3-arg 路径，结果正确：start=3 搜 "B"(位置 2) → 找不到） |
| probe：`INSTR(LEN("BC")+1, "ABC", "C")`（显式非零） | ✅ `X= 3`（start=3 搜 "C" → 位置 3；数字函数 3-arg 完整确认） |
| probe：`ON INSTR(A$, ":") GOTO`（回归） | ✅ `b2`（引号感知 + 2-arg 保持） |
| probe：`INSTR(A$)` 1-arg（SNERR 回归） | ✅ `Error at line 20` |

### 修复质量细节（代码复核）

```c
/* v134 MEDIUM-2: 三级形式判定 */
if (first == '"')                        { /* 2-arg: 无歧义廉价 */ }
else if (isdigit(first) || first=='(' ||
         first=='-' || first=='+')       { /* 3-arg: 无歧义廉价 */ }
else { /* 字母/$/_ 开头: 歧义 → 完整 probe */
    char *probe_s1 = parse_string_expression(p);
    if (probe_s1) { ... 2-arg (probe_s1 直接复用, 无多余分配) ... }
    else { p->pos = save_pos; ... 3-arg 重新解析 ... }
}
/* v134 MEDIUM-1: 两处 3-arg 分支统一 */
int pos = bistrs_instr(str_val, s2, (int)start);
if (pos < 0) { gwdata_set_error(5, ...); return 0.0f; }  /* FCERR */
```
- 字母分支 probe **成功时直接复用 probe_s1**（无重新分配浪费）；失败时 NULL 无分配 → rewind 干净 ✅
- 数字变量 `N` probe 失败路径确认：`parse_string_expression` 变量分支仅 `VT_STRING` 才 return，数字变量 fall-through 返回 NULL → 3-arg ✅
- 数字函数 `LEN(...)` 经"未识别字符串函数 → 数组引用尝试 → `biptrg_get_array_string` 返回 NULL"→ probe 失败 → 3-arg ✅（probe_len2 实证正确结果 3）

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无。

本版为修复确认轮：v134 的 2 个 MEDIUM 已关闭，无新增问题。

**备注（不计问题）**：
1. 字母开头的 3-arg 形式（`INSTR(N, ...)`）走完整 probe 路径，多一次字符串解析尝试（无分配浪费，probe_s1 复用/失败零分配）— 与 v132 成本相当，可接受。
2. INSTR 形式判定强度：现覆盖引号/数字/括号/符号/字母变量/字母函数；字符串字面量含 `%`/`&` 后缀变量等边角未逐一验证，后续 M2 字符串函数批次可一并审计。

---

## 三、架构评估

### 修复质量（c85284e）

| 决策 | 评价 |
|------|------|
| 三级形式判定（无歧义廉价分类 + 歧义完整 probe 回退） | ✅ 正确 — 覆盖全部形式，probe 复用/零分配失败 |
| `pos<0 → set_error(5)`（FCERR 传播，与 SNERR 对称） | ✅ 正确 — 语句级报 Illegal function call |
| 单测更新（start_range ERR=5 断言 + 3 新用例） | ✅ 质量好 — API/语句级/形式全覆盖 |
| probe 失败 rewind（pos 恢复,无泄漏） | ✅ 干净 |
| 回归保持（2-arg / 引号感知 / 紧凑 / SNERR） | ✅ probe 6 项全绿 |
| v134 报告随提交入库 | ✅ 流程闭合，工作区 clean |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| INSTR 2-arg（字面量/变量）+ start=1 | ✅ |
| INSTR 3-arg（字面量 start / 数字变量 N / 数字函数 LEN() / 算术表达式） | ✅ **全形式**（含 probe 复测） |
| INSTR start 范围（0/负/超 255）→ FCERR（API + 语句级） | ✅ 修复 + 双重验证 |
| INSTR 1-arg / 类型错误 → SNERR | ✅ |
| ON..GOTO + INSTR 组合（引号感知） | ✅ 回归 b2 |
| 注册一致性 | ✅ 805=805，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（可选）
1. （备注项）边角形式审计（`%`/`&` 后缀变量等）随 M2 字符串函数批次处理

### 中期（M2 规划）
2. 其余字符串函数做同型"形式判定/错误传播"一致性审计（LEFT$/MID$/RIGHT$/CHR$/STR$/SPACE$/STRING$/UCASE$/LCASE$）
3. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）
4. .bas 套件补 INSTR 集成场景（含数字变量 start）

### 长期
5. ASAN 构建
6. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: v134 的 2 个 MEDIUM 全部修复且独立验证通过 — FCERR 语句级传播（`INSTR(-1,...)` → `Error at line 10`）、三级形式判定（数字变量 `INSTR(N,...)` → 2、数字函数 `INSTR(LEN("BC")+1,...)` → 3，字母分支 probe 复用/零分配），6 项 probe 全绿；单测 3102/3102、套件 72/72、0 警告、0 孤儿。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**。无阻塞性缺陷。INSTR 函数（含 ON..GOTO 组合）实现正式闭环。
**建议**: M2 继续下一批（字符串函数一致性审计 → 下一语句/函数里程碑）。

---

## 六、指标追踪

| 指标 | v134 | v135 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3094 | **3102** | ✅ +8（+3 测试函数） |
| 定义数（唯一函数） | 802 | **805** | +3 |
| 注册数（run_test 调用） | 802 | **805** | +3 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72 | **72/72** | 保持 |
| FCERR 语句级传播 | ❌ MEDIUM-1 | ✅ **修复** | 关闭 |
| 形式判定（数字变量/函数 start） | ❌ MEDIUM-2 | ✅ **修复** | 关闭 |
| INSTR 函数状态 | 主体完成 + 3 修复轮 | **正式闭环** | 完成 |
| M2 进度 | 1/N + 4 修复轮 | **1/N + 5 修复轮（INSTR 闭环）** | — |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3102 → 套件 72/72 → 孤儿 805=805 → 6 项 probe（4 验证 + 2 回归，已清理）
- probe_len 首版结果 X=0 实为正确行为（start=LEN("ABC")=3，搜 "B"(位置 2) → 找不到）；补 probe_len2（`INSTR(LEN("BC")+1, ...)` → 3）作非零显式确认
- **M2 状态**: ON..GOTO/GOSUB + INSTR 闭环完成；下一批待定（字符串函数一致性审计为建议起点）