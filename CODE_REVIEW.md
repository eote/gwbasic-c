# GW-BASIC C Port - Code Review v133

**日期**: 2026-10-11
**审查范围**: commit `9b31fbd`（fix: CODE_REVIEW v132 — 引号感知冒号截断 + INSTR 函数实现，BISTRS.ASM L841-949）→ 工作区
**测试状态**: 单元测试 **3088/3088 通过**（0 失败）✅ | .bas 集成套件 **72/72 全 PASS（0 跳过）** ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean（v132 报告已随 9b31fbd 入库）
**总体评分**: B- — v132 MEDIUM-1 修复确认（回归 probe b2）+ INSTR 实现主体正确（ASM INSTRC 1:1、9 新单测），但发现 **1 MEDIUM**（INSTR start 范围校验缺失）及 2 LOW

---

## 一、概述

本次审查开发者对 v132 修复提交 `9b31fbd`（9 文件，+297/-109）。提交不仅修复引号感知冒号截断，还**连带实现了 INSTR 函数**（印证 v132 probe 失败的双重原因：引号截断 + INSTR 未实现）。

| 变更 | 说明 |
|------|------|
| 引号感知冒号截断 | ON 分支 in_string 翻转（`'"'`）扫描，字符串字面量内冒号跳过（GW-BASIC 无转义，`""` 仍是字面量内容 — ASM 语义） |
| `bistrs_instr`（BISTRS.ASM L841-949 INSTRC 1:1） | start=0→FCERR；s1 空→0；start>LEN→0；s2 空→return start；found→1-based 首现（strstr + GOTSTR）；not found→0 |
| gweval dispatch | probe+rewind 形式判定：首参字符串 → 2-arg（start=1）；首参数字 → 3-arg；`is_str_numeric` 组加入 INSTR |
| 赋值分支排除 bare ON | `starts_with("ON")` 加入赋值排除（同 OPEN/FIELD 模式），`ON (A$='X:Y')...` 不再被劫持 |
| 9 新单测 | INSTR API 7（test_bistrs.c）+ string_colon + instr_function（test_gwsts_ongoto.c） |

### v132 修复确认（验证通过）

| v132 问题 | 修复 | 独立验证 |
|-----------|------|----------|
| 🟨 MEDIUM-1 引号感知 | in_string 翻转扫描 | ✅ probe `ON INSTR(A$, ":") GOTO 100, 200` → **`b2`**；单测 string_colon 通过 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3088/3088 PASS**（789 → 798，+9 测试） |
| .bas 集成套件（72 项） | ✅ **72/72 全 PASS，0 跳过** |
| 孤儿检测（唯一函数名） | ✅ **798 定义 = 798 注册，0 孤儿 / 0 多余** |
| probe：`ON INSTR(A$, ":") GOTO`（回归） | ✅ `b2`（引号感知 + INSTR 均正确） |
| probe：`INSTR(-1, "ABC", "B")` | ❌ `B= 0`（应 FCERR）— **MEDIUM-1（新）实证** |
| probe：`INSTR(300, "ABC", "B")` | ❌ `B= 0`（应 FCERR）— **MEDIUM-1（新）实证** |
| probe：`INSTR(A$)`（1-arg 缺 S2） | ❌ `B= 0` 静默（应报错）— **LOW-1 实证** |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **INSTR start 参数范围校验缺失**：负值 / >255 → 返回 0，GW-BASIC 手册明确 "start must be in range 1 to 255, otherwise Illegal function call (FCERR)" | `src/bistrs.c`（bistrs_instr）、`src/gweval.c`（dispatch） |

**详情**:

```c
int bistrs_instr(const char *s1, const char *s2, int start) {
    if (start == 0) return -1;                /* 仅校验 ==0 ✅ */
    if (!s1 || !*s1) return 0;
    size_t len1 = strlen(s1);
    if ((size_t)start > len1) return 0;       /* start<0 → (size_t) 巨大 → 0 ❌ */
    ...
}
```

**实证**：`INSTR(-1, "ABC", "B")` → `B= 0`；`INSTR(300, "ABC", "B")` → `B= 0`（均应为 FCERR 报错）。

**根因**：`start == 0` 单独处理；负值经 `(size_t)` 转换落入 `> len1` → 0；`start>255`（字符串 ≤255 时必然 `> len1`）→ 0。均未映射 FCERR。

**修复建议**：`bistrs_instr` 前置校验 `if (start < 1 || start > 255) return -1;`（ASM INSTRC 对 start 的 FCERR 语义），并补单测 `test_bistrs_instr_negative_start` / `test_bistrs_instr_start_too_large`（>255）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **INSTR 非法形式静默**：`INSTR(A$)`（缺 S2）→ 静默返回 0；`INSTR(A$, 5)`（S2 类型错）同样。注释声称 "falls through to error handling"，实际无错误产生 | `src/gweval.c`（INSTR dispatch） |
| 2 | **观察：dispatch probe 额外分配**：`parse_string_expression` probe（探测后 rewind）每次 INSTR 调用分配首参副本且未释放；与项目字符串函数"参数副本不释放、字符串堆统一管理"模式一致，但 probe 是纯浪费，字符串堆额外增长 | `src/gweval.c`（L548+） |

**详情**:

1. **LOW-1**：2-arg 分支 `parser_match(p, ',')` 不匹配（1-arg）或 S2 解析失败（类型错）时，无 return → 不设置错误 → parse_factor 默认路径返回 0 → `B = 0` 静默。GW-BASIC 期望 SNERR/类型错误。建议：失败路径显式设置错误（与其他字符串函数错误处理一致）。
2. **LOW-2**：probe 用完整 `parse_string_expression`（分配字面量/变量副本）后 `p->pos = save_pos` 丢弃 — 每次 INSTR 调用字符串堆多一份首参拷贝。建议：probe 改为轻量类型探测（`peek()` 判断 `"` 或字母/`$` 后缀），避免完整解析与分配。

---

## 三、架构评估

### 修复质量（9b31fbd）

| 决策 | 评价 |
|------|------|
| 引号感知冒号截断（in_string 翻转，GW-BASIC 无转义语义） | ✅ 正确 — v132 MEDIUM-1 修复实证（probe b2） |
| `bistrs_instr` 语义（start>LEN→0 / s2 空→start / 1-based 首现 / not found→0） | ✅ 主体正确（7 API 单测）；⚠️ start 范围校验缺失（MEDIUM-1） |
| dispatch probe+rewind 形式判定（字符串首参→2-arg；数字→3-arg） | ✅ 判定逻辑正确（test_gweval_instr_function 2/3-arg 均过）；⚠️ 非法形式静默（LOW-1） |
| `is_str_numeric` 组加入 INSTR | ✅ 正确（返回值类型判定） |
| 赋值分支排除 bare ON | ✅ 正确（同 OPEN/FIELD 模式） |
| 9 新单测（INSTR 7 + 语句级 2） | ✅ 隔离正确、断言明确；缺 start 范围/非法形式用例 |
| code review 流程（报告随提交入库） | ✅ 工作区 clean |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| INSTR 2-arg / 3-arg / start / not-found / 空 S2 / 首现 / start=0 FCERR | ✅ 7 单测 |
| 引号感知冒号截断（语句级） | ✅ string_colon + probe |
| **start <1 / >255 → FCERR** | ❌ **盲区 → MEDIUM-1** |
| **1-arg / S2 类型错误** | ❌ **盲区 → LOW-1** |
| 注册一致性 | ✅ 798=798，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：`bistrs_instr` 补 `start < 1 || start > 255 → -1`，补负值/超 255 单测
2. **LOW-1**：INSTR dispatch 失败路径设置错误（1-arg / S2 类型错），补用例
3. **LOW-2**：probe 改轻量类型探测，避免完整解析分配

### 中期（M2 规划）
4. INSTR 集成验证完成（ON..GOTO/GOSUB + INSTR 组合已实证）→ 继续 M2 下一批语句/函数
5. 其余字符串函数与 INSTR 一致性审计（range 校验/非法形式错误处理）
6. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
7. ASAN 构建
8. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: v132 修复确认（引号感知冒号截断 -> probe `b2`）+ INSTR 实现主体正确（ASM INSTRC 1:1 语义、9 新单测、dispatch 形式判定合理）—— 套件 72/72 保持全绿、0 孤儿。**但 INSTR start 参数范围校验缺失（1 MEDIUM）**：负值/超 255 静默返回 0，违反 GW-BASIC 手册 "start 必须 1-255 否则 FCERR"；另 1-arg 非法形式静默（LOW）、probe 额外分配（LOW）。
**关键发现**: **1 MEDIUM**（start 范围校验）、**2 LOW**（非法形式静默、probe 分配浪费）。
**建议**: 补 start<1/start>255 → FCERR + 对应单测；dispatch 失败路径报错；probe 轻量化。修复后 M2 继续推进。

---

## 六、指标追踪

| 指标 | v132 | v133 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3078 | **3088** | ✅ +10（+9 测试函数） |
| 定义数（唯一函数） | 789 | **798** | +9 |
| 注册数（run_test 调用） | 789 | **798** | +9 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 72/72 | **72/72** | 保持 |
| 引号感知冒号截断 | ❌ MEDIUM-1（v132） | ✅ **修复**（probe b2） | 关闭 |
| INSTR 函数 | ⏳ 未实现（probe 失败部分原因） | ✅ 实现（语义主体正确） | 新增 |
| **INSTR start 范围校验** | — | ❌ **MEDIUM-1（新）** | 新发现 |
| M2 进度 | 1/N + 2 修复轮 | **1/N + 3 修复轮** | — |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3088 → 套件 72/72 → 孤儿 798=798 → 4 项 probe（1 回归 + 3 边界，已清理）
- v132 probe 失败为双重原因（引号截断 + INSTR 未实现）；本次提交同时闭合
- start 范围校验（1-255）与 start>LEN→0 需并存：start 在 1-255 但 >LEN → 0；start<1 或 >255 → FCERR
- **M2 状态**: ON..GOTO/GOSUB + INSTR 主体完成；start 范围校验修复后进入下一批