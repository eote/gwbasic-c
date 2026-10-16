# GW-BASIC C Port - Code Review v138

**日期**: 2026-10-16
**审查范围**: commit `5bf4584`（fix: CODE_REVIEW v137 — MEDIUM-1 STRING$ 变量回退 + MEDIUM-2 数字上下文 SNERR + IF 串比较真语义）→ 工作区
**测试状态**: 单元测试 **3138/3138 通过**（0 失败，0 警告）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过）** ✅ | 孤儿 **823=823** ✅
**工作区状态**: clean
**总体评分**: B — v137 的 2 个 MEDIUM 修复确认（probe 双重实证），IF 简单串比较真语义正确；但发现 **2 个 IF 相关 MEDIUM**（字符串复合条件 AND/OR 逻辑错误、IF..THEN..ELSE 的 ELSE 分支未实现）

---

## 一、概述

本次审查开发者对 v137 修复提交 `5bf4584`（7 文件，+362/-71）。

### v137 修复确认（全部验证通过）

| v137 问题 | 修复方式（5bf4584) | 独立验证 |
|-----------|-------------------|----------|
| 🟨 MEDIUM-1 STRING$ 变量参数 | 第 2 参字母开头完整 parse probe（v134 INSTR 三级模式）：probe 成功 → 字符串形式；失败 → rewind + 数字形式；quote 直走 | ✅ probe `STRING$(3, S$)`（S$="B"）→ `LEN1= 3 ASC1= 66`（'B'）；`STRING$(3, LEN("AB"))` → `LEN2= 3 ASC2= 2`（数字回退）|
| 🟨 MEDIUM-2 数字上下文静默 | 数字 parse_factor 遇字符串函数 → consume 参数到匹配 `)` + `set_error(2)` SNERR | ✅ probe `Y = CHR$(256)` → `Error at line 10`（不再静默 Y=0）|
| IF 串比较真语义（新）| 顶层比较运算符（引号/括号感知）切分 lhs/rhs → 双方 `gweval_eval_string` 成功 → strcmp 真比较；否则回退数字求值 | ✅ 简单串比较（字面量 `"A"="A"` → T2 / 变量 `A$="X"` → T3）正确；数字条件回退（`IF 5` → T4）正确；**修复了之前"字符串比较静默当 0 比较的 accidental-true bug"** |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3138/3138 PASS**（816 → 823，+7） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过** |
| 孤儿检测（唯一函数名） | ✅ **823 定义 = 823 注册，0 孤儿 / 0 多余** |
| probe：STRING$ 变量 / LEN 回退 | ✅ ASC1=66 / ASC2=2 |
| probe：数字上下文 SNERR | ✅ `Error at line 10` |
| probe：IF 简单串比较（字面量/变量）| ✅ T2 / T3（真语义） |
| probe：IF 数字条件（`IF 5`）| ✅ T4 |
| probe：数字复合条件（`1=2 AND 2=2` / `1=1 AND 2=2`）| ✅ false（无输出）/ AND-Y（eval_numeric 支持比较与逻辑运算符）|
| probe：**IF 字符串复合条件**（`"A"="A" AND "B"="C"`）| ❌ `AND-Y`（应 AND-N）— **MEDIUM-1（新）实证** |
| probe：**IF..THEN..ELSE**（`IF 1=2 THEN T ELSE F`）| ❌ F 不打印 — **MEDIUM-2（新）实证** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **IF 字符串复合条件（AND/OR）逻辑错误**：`IF "A" = "A" AND "B" = "C" THEN` → 错误 true | `src/gwsts.c`（IF 串比较分支）|
| 2 | **IF..THEN..ELSE 的 ELSE 分支未实现**：条件 false 时 ELSE 语句静默不执行（既有缺口，非本次引入）| `src/gwsts.c`（IF 分支）|

**详情**:

1. **MEDIUM-1 — 字符串复合条件**：
   ```
   10 IF "A" = "A" AND "B" = "C" THEN PRINT "AND-Y" ELSE PRINT "AND-N"
   ```
   实测：`AND-Y`（GW-BASIC 语义：true AND false = **false** → 应 `AND-N`）。
   根因：串比较分支只取**第一个顶层比较运算符**（`"A" = "A"` 的 `=`）→ lhs=`"A"`，rhs=`"A" AND "B" = "C"`（包含 AND 后全部文本）→ `gweval_eval_string(rhs)` **部分解析出** `"A"`（AND 后忽略）→ `strcmp("A","A")=0` → true。**AND/OR 组合逻辑被丢弃**。对照：数字复合条件正常（eval_numeric 支持比较/逻辑运算符，`1=1 AND 2=2` → 正确）。修复方向：串比较分支同样需要**完整条件表达式求值**（字符串比较结果与 AND/OR 组合），或解析 AND/OR 分隔各子条件逐项比较后按逻辑合并。补单测：`IF "A"="A" AND "B"="C" THEN`（false）、`IF "A"="A" OR "B"="C" THEN`（true）。

2. **MEDIUM-2 — IF..THEN..ELSE 未实现**：
   ```
   10 IF 1 = 2 THEN PRINT "T" ELSE PRINT "F"
   ```
   实测：无输出（应为 `F`）。`else { result = SUCCESS; }` — **ELSE 语句从不执行**（既有行为，旧代码即如此，此前未被探测）。GW-BASIC 单行 `IF <cond> THEN <stmt> ELSE <stmt>` 是标准高频语法。修复方向：then 分支未取时解析并执行 ELSE 后的语句；补测试与 .bas 集成。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无新增。

（备注：`X = CHR$`（无括号）等"字符串函数无括号调用"在数字/字符串上下文仍静默（走变量解析路径），边缘观察项，随类型系统审计一并处理。）

---

## 三、架构评估

### 修复质量（5bf4584）

| 决策 | 评价 |
|------|------|
| STRING$ 字母回退（v134 INSTR 三级模式复用）| ✅ 正确 — probe 双重实证（变量 66 / LEN 回退 2）|
| 数字上下文 consume+SNERR | ✅ 正确 — 参数消费保持解析位置（`Y = CHR$(256)` → Error）|
| IF 串比较顶层运算符切分（引号/括号感知，最长匹配优先）| ✅ 简单场景正确（字面量/变量真比较）；⚠️ 复合条件 rhs 含 AND 文本（MEDIUM-1）|
| `gweval_eval_string` 双端求值 + strcmp | ✅ 简单串比较真语义（修复 accidental-true）|
| 错误传播（lhs/rhs 失败 → 数字回退）| ✅ 数字条件/数字复合保持正确 |
| **ELSE 分支** | ⚠️ MEDIUM-2 — 未实现（既有缺口，本次审计未覆盖）|
| 新单测（+7：STRING$ 变量/数字 SNERR/IF 串比较等）| ✅ 注册完整（823=823）；缺 IF 复合条件（MEDIUM-1）与 ELSE（MEDIUM-2）用例 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| STRING$ 字符串变量 / 数字回退 | ✅ 修复 + 新单测 |
| 数字上下文字符串函数 → SNERR | ✅ 修复 + 新单测 |
| IF 简单串比较（字面量/变量）| ✅ 真语义 + 新单测 |
| IF 数字条件 / 数字复合 | ✅ 保持（probe 验证）|
| **IF 字符串复合（AND/OR）** | ❌ 盲区 → MEDIUM-1 |
| **IF..THEN..ELSE（ELSE 分支）** | ❌ 盲区 → MEDIUM-2 |
| 注册一致性 | ✅ 823=823，0 孤儿 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：串比较分支支持 AND/OR 复合（按逻辑运算符切分各子条件逐项比较后合并，或表达式级字符串比较求值）；补 true/false 复合单测与 .bas 集成
2. **MEDIUM-2**：实现 IF..THEN..ELSE 的 ELSE 分支（then 未取时解析 `ELSE` 后的语句执行）；补单测 + .bas

### 中期（M2 规划）
3. IF 多行/嵌套边缘（ELSE 后串比较、与 GOTO/语句组合）
4. 无括号字符串函数调用静默项（类型系统审计收尾）
5. .bas 套件补 IF 场景（简单/复合/ELSE）
6. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）
7. 数字/字符串类型系统全文审计（parse_expression 与 parse_string_expression 全部交互）

### 长期
8. ASAN 构建
9. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: v137 的 2 个 MEDIUM 修复确认（STRING$ 变量回退 ASC1=66/LEN 回退 ASC2=2；数字上下文 SNERR → Error），IF 简单串比较真语义正确（字面量/变量 T2/T3，修复 accidental-true）；单测 3138/3138、套件 72/72、0 警告、0 孤儿。**但 IF 语句审计暴露 2 个 MEDIUM**：字符串复合条件（AND/OR）逻辑被丢弃（`"A"="A" AND "B"="C"` 错误 true）、IF..THEN..ELSE 的 ELSE 分支未实现（既有缺口）。
**关键发现**: **2 MEDIUM**（IF 复合串比较、ELSE 未实现）、0 HIGH、0 CRITICAL。
**建议**: 实现 IF 复合条件与 ELSE 分支（均需单测 + .bas 集成）；修复后 M2 进入第 2 批（GENGRP 深化）。

---

## 六、指标追踪

| 指标 | v137 | v138 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3123 | **3138** | ✅ +15（+7 测试函数） |
| 定义数（唯一函数） | 816 | **823** | +7 |
| 注册数（run_test 调用） | 816 | **823** | +7 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72 | **72/72** | 保持 |
| STRING$ 变量参数 | ❌ MEDIUM-1（v137）| ✅ **修复** | 关闭 |
| 数字上下文 SNERR | ❌ MEDIUM-2（v137）| ✅ **修复** | 关闭 |
| IF 简单串比较 | ❌ accidental-true | ✅ **真语义** | 修复 |
| **IF 字符串复合（AND/OR）** | — | ❌ **MEDIUM-1** | 新发现 |
| **IF..THEN..ELSE（ELSE）** | ⏳ 未实现（既有）| ❌ **MEDIUM-2** | 新发现（审计暴露）|
| M2 进度 | 2/3 + 审计批 | **2/3 + 2 修复轮** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3138 → 套件 72/72 → 孤儿 823=823 → 10 项 probe（4 修复/回归 + 6 边界，已清理）
- 数字复合条件正常（eval_numeric 支持比较与逻辑运算）；缺陷仅限字符串复合路径
- MEDIUM-2（ELSE）为既有缺口，`5bf4584` 之前的 IF 分支即 `else SUCCESS`，本轮 probe 首次暴露
- **M2 状态**: IF 语句审计暴露问题待修复；修复后进入第 2 批（GENGRP PSET/PRESET/LINE）