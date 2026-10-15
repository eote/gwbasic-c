# GW-BASIC C Port - Code Review v137

**日期**: 2026-10-15
**审查范围**: commit `8cad67d`（feat: M2 字符串函数一致性审计 — CHR/LEFT/RIGHT/MID/SPACE/STRING ASM 语义 + 错误传播 + handler-FATAL 规则）→ 工作区
**测试状态**: 单元测试 **3123/3123 通过**（0 失败，0 警告）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过）** ✅ | 孤儿 **816=816** ✅
**工作区状态**: clean（v136 报告已随 41e6877 入库）
**总体评分**: B- — 字符串函数语义/错误传播/handler-FATAL 审计主体正确（9 项 probe 验证多数通过），但发现 **2 MEDIUM**（STRING$ 字符串变量参数、数字上下文字符串函数静默为 0）

---

## 一、概述

本次审查 M2 字符串函数一致性审计提交 `8cad67d`（按 v136 建议推进，7 文件，+345/-159）。

### 变更内容

| 项 | 说明 |
|----|------|
| CHR/LEFT/RIGHT/MID/SPACE/STRING 范围校验 | n 0..255 或 1..255（ASM PREAM/GETBYT 语义），越界/负 → FCERR |
| MID$ 2-arg 形式补齐 | `MID$(s, start)` 返回到末尾（ASM MID$ L763-796），len=0x7FFF clamp |
| 错误传播统一 | dispatch `pos<0/fail → set_error(5)`；assign_value：str_val NULL 且 error → propagate（不再回退字面量） |
| handler-FATAL 规则 | handler 内再出错 → 打印 `Error at line N` 终止（防 OM 无限循环）；was_in_handler 快照区分 ERROR 语句 |
| 字符串堆耗尽 → OM | parse_string_expression 字面量分配失败 → set_error(14)（v129 中期项落地） |
| 新单测 | test_gweval_strfuncs.c（10 测试，816 注册） |

### v136 建议落实情况

| v136 建议 | 状态 |
|-----------|------|
| 字符串函数一致性审计（9 函数同型检查） | ✅ 主体完成（范围校验/错误传播）；⚠️ 2 个类型判定缺口见问题清单 |
| 字符串堆 OM 传播 | ✅ 一并落地 |
| handler-FATAL 无限循环防护 | ✅ 一并落地（probe 实证） |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3123/3123 PASS**（805 → 816，+11） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过** |
| 孤儿检测（唯一函数名） | ✅ **816 定义 = 816 注册，0 孤儿 / 0 多余** |
| probe：CHR$(256) 语句级 FCERR | ✅ `Error at line 10` |
| probe：PRINT CHR$(256) | ✅ `Error at line 10`（字符串上下文报错正常） |
| probe：MID$("ABCDE",2) 2-arg | ✅ `BCDE` |
| probe：LEFT$("AB",5) n>LEN | ✅ `AB`（截断返回全串） |
| probe：STRING$(3,65) 数字形式 | ✅ `AAA` |
| probe：handler-FATAL（字符串赋值） | ✅ `before` → `Error at line 500` 终止（无 RESUME） |
| probe：**STRING$(3, S$)（S$="B"，字符串变量）** | ❌ `LEN= 0`（应 "BBB"/LEN=3）— **MEDIUM-1 实证** |
| probe：**Y = CHR$(256)（数字变量）** | ❌ `Y= 0` 静默（应 SNERR）— **MEDIUM-2 实证** |
| probe：**X = LEFT$("AB",1)（数字变量）** | ❌ `X= 0` 静默（应 SNERR）— **MEDIUM-2 普遍性** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **STRING$ 第 2 参为字符串变量时结果错误**：`STRING$(n, S$)`（S$="B"）→ 空串（应 S$ 首字符重复 n 次） | `src/gweval.c`（STRING$ dispatch） |
| 2 | **数字表达式上下文中字符串函数静默为 0**：`Y = CHR$(256)` / `X = LEFT$("AB",1)` → 静默 0（GW-BASIC 应为类型不匹配 SNERR） | `src/gweval.c`（数字表达式解析器） |

**详情**:

1. **MEDIUM-1 — STRING$ 参数类型判定缺口**：
   ```c
   /* STRING$ dispatch: 第 2 参类型判定 */
   if (first2 == '"') { /* 字符串字面量 → 首字符 */ }
   else { /* 数字 → 0..255 校验 */ }
   ```
   字母开头的**字符串变量**（`STRING$(10, S$)`）未处理：走数字分支 → `parse_expression("S$, ...")` 数字解析失败静默 → code=0 → 结果空串。**与 INSTR 的字母歧义同型，但此处未实现回退**（INSTR 在 v134 已加 probe 回退）。GW-BASIC `STRING$(n, str$)` 合法且常用。**修复**：第 2 参字母开头回退完整字符串解析 probe（同 INSTR 三级判定），补单测 `STRING$(3, S$)`。

2. **MEDIUM-2 — 数字上下文字符串函数静默**：
   ```
   10 Y = CHR$(256)     → 实际 Y= 0（无错误）
   10 X = LEFT$("AB",1) → 实际 X= 0（无错误）
   ```
   字符串函数（CHR$/LEFT$/RIGHT$/MID$/STR$/SPACE$/STRING$/UCASE$/LCASE$）出现在**数字表达式上下文**（数字赋值、算术、IF 条件）时应报类型不匹配（SNERR），当前数字表达式解析器不识别字符串函数名 → **静默回退为 0**。字符串上下文（`A$ = CHR$(256)`、`PRINT CHR$(256)`）报错正常（probe 实证）—— 缺陷仅在数字路径。**修复**：数字解析器遇到已知字符串函数名 → 显式 SNERR（类型不匹配），补单测 `Y = CHR$(256)` / `X = LEFT$(...)` / `IF CHR$(65) THEN`。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无新增。

（注：MEDIUM-2 影响面 = 全部 9 个字符串函数 × 数字上下文；严重度高于单个函数 bug，属解析器级类型安全缺口。）

---

## 三、架构评估

### 审计质量（8cad67d）

| 决策 | 评价 |
|------|------|
| 范围校验 0..255 / 1..255（PREAM/GETBYT 语义） | ✅ 正确（probe CHR$(256) → Error） |
| MID$ 2-arg 补齐 + len clamp（0x7FFF） | ✅ 正确（probe BCDE；bistrs_mid clamp 复核） |
| assign_value 错误传播（不再回退字面量） | ✅ 正确（字符串上下文 FCERR 语句级生效） |
| handler-FATAL（was_in_handler 快照区分 ERROR 语句） | ✅ 正确（probe_fatal3：handler 内错误 → Error at line 500 终止） |
| 字符串堆 OM → set_error(14) | ✅ 正确（代码复核） |
| **STRING$ 第 2 参类型判定** | ⚠️ **MEDIUM-1** — 仅 quote 判定，无字母回退 |
| **数字解析器对字符串函数的处理** | ⚠️ **MEDIUM-2** — 静默 0（历史遗留，非本批引入，但审计应覆盖） |
| 新单测（10：CHR/LEFT/RIGHT/MID/SPACE/STRING 各边界） | ✅ 隔离正确；缺 STRING$ 变量参数与数字上下文类型错误用例 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| CHR$/LEFT$/RIGHT$/MID$/SPACE$/STRING 范围（FCERR） | ✅ 单测 + probe |
| MID$ 2-arg / LEFT$ n>LEN / STRING$ quote+数字 | ✅ |
| handler-FATAL / assign 错误传播 / OM | ✅ 单测 + probe |
| **STRING$(n, S$) 字符串变量** | ❌ 盲区 → MEDIUM-1 |
| **数字上下文字符串函数（类型错误）** | ❌ 盲区 → MEDIUM-2 |
| 注册一致性 | ✅ 816=816，0 孤儿 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：STRING$ 第 2 参字母开头回退字符串解析 probe（复用 v134 INSTR 三级判定模式）；补 `STRING$(3, S$)` / `STRING$(3, LEFT$(...))` 单测
2. **MEDIUM-2**：数字解析器遇字符串函数名 → 显式 SNERR；补 `Y = CHR$(256)` / `X = LEFT$...` / `IF CHR$(65) ...` 单测
3. 扩展审计：其余字符串函数（ASC/VAL/INSTR 已在）与数字上下文全文扫描（grep 字符串函数 dispatch 在 parse_expression 数字路径的覆盖）

### 中期（M2 规划）
4. .bas 套件补 STRING$ 场景（字面量/变量/数字）
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）
6. 数字解析器类型系统审计（数字函数与字符串函数所有交互形式）

### 长期
7. ASAN 构建
8. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: M2 字符串函数一致性审计主体高质量完成 — 范围校验（CHR$/LEFT$/RIGHT$/MID$/SPACE$/STRING）、MID$ 2-arg、assign_value 错误传播、handler-FATAL（防 OM 无限循环，probe 实证 `Error at line 500` 终止）、字符串堆 OM 传播全部正确且语句级生效；单测 3123/3123、套件 72/72、0 警告、0 孤儿。**但发现 2 MEDIUM 静默错误缺陷**：STRING$ 第 2 参字符串变量（同 INSTR 字母歧义，未做回退）与数字上下文字符串函数静默 0（解析器级类型安全缺口，覆盖全部 9 个字符串函数）。
**关键发现**: **2 MEDIUM**、0 HIGH、0 CRITICAL。
**建议**: STRING$ 参数回退 probe + 数字解析器字符串函数 SNERR，补单测；修复后 M2 进入第 2 批（GENGRP 深化 PSET/PRESET/LINE）。

---

## 六、指标追踪

| 指标 | v136 | v137 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3102 | **3123** | ✅ +21（+11 测试函数） |
| 定义数（唯一函数） | 805 | **816** | +11 |
| 注册数（run_test 调用） | 805 | **816** | +11 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72 | **72/72** | 保持 |
| 字符串函数范围校验 | ⏳ | ✅（CHR/LEFT/RIGHT/MID/SPACE/STRING） | 完成 |
| handler-FATAL / OM 传播 | — | ✅ | 新增 |
| **STRING$ 字符串变量参数** | — | ❌ **MEDIUM-1** | 新发现 |
| **数字上下文字符串函数静默 0** | — | ❌ **MEDIUM-2** | 新发现 |
| M2 进度 | 2/3 | **2/3 + 审计批**（INSTR 闭环 + 字符串审计主体） | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3123 → 套件 72/72 → 孤儿 816=816 → 9 项 probe（含 MEDIUM-1/2 实证与 handler-FATAL 确认，已清理）
- probe_fatal2 首版（`Y = CHR$(256)`，数字变量）暴露的"handler 未终止"表象实为 MEDIUM-2（数字上下文静默）导致未进 handler；修正为字符串变量后 handler-FATAL 规则实证正确
- **M2 状态**: 字符串审计主体完成；2 MEDIUM 修复后进入第 2 批（GENGRP PSET/PRESET/LINE）