# GW-BASIC C Port - Code Review v79

**日期**: 2026-08-23
**审查范围**: commit `e86ecd7` 完整代码审查 — 详细问题记录（现存：C1 未修复）
**测试状态**: 1085/1085 单测 ✅（不校验输出）| .bas exit 0（输出层回归未被捕获）
**进度**: 44/44 模块 (100%)
**总体评分**: C- — 解析器重构存在多重回归（1 CRITICAL + 2 HIGH + 2 MEDIUM + 4 LOW），相互掩盖，仅凭 exit code 无法发现

---

## 〇、当前状态确认

- HEAD = `e86ecd7`（重构提交）；工作区仅 CODE_REVIEW.md 变更
- **C1 未修复**：`gweval.c:701-702` 仍为 `/* Default: empty string */ char *result = gwdata_alloc_string(1);`（应恢复 d68248e 的 `return NULL`）
- 下文全部问题按"当前代码"状态记录；修复后需重新验证

---

## 一、🟥 CRITICAL-1：`gweval_eval_string` 默认返回空串 — 数值输出全断

### 问题代码（gweval.c 696-702，函数尾部）
```c
    /* Default: empty string */
    char *result = gwdata_alloc_string(1);
    if (result) result[0] = '\0';
    return result;      /* ← 对任何未识别为字符串的表达式返回 ""（非 NULL）*/
}
```

### 回归链（git 证据）
| 提交 | 状态 | 代码 |
|------|------|------|
| d68248e（v70 后修复）| ✅ | `return NULL;  /* Not a recognized string expression - let caller try numeric eval */` |
| e86ecd7（v76 提交）| ❌ 回退 | 重构 gweval 时复制旧代码，恢复"默认空串"（丢失 NULL 修复）|

### 影响链（调用方 gwsts.c 4 处 `if (str_val)` 模式 — 行 248/308/782/808）
```
PRINT I          → eval_string("I")     → ""（非 NULL）→ printf("")       → 值空
PRINT 5+1        → eval_string("5+1")   → ""           → 空
PRINT SIN(3.14)  → ""（且参数被布局吃掉）→ "SIN() = "  → 参数与值全空
PRINT #1, 数值    → dskcom_print(fd, "") → 文件写入空数据
数值回退分支 (eval_numeric) 永不执行 — str_val 恒非 NULL
```

### 实测证据（输出内容级）
| 测试 | 输出 | 预期 |
|------|------|------|
| test_expr2.bas | `"I = \nI+1 = \nAfter I = \n"` | `I = 1` 等 |
| test_for3.bas | 100 行 `"\t = "` | `1 11 = 12` |
| test_math.bas | `"SIN() = \n"`×12 | `SIN(3.14159) = 2.5e-06` |

### 修复建议
```c
/* gweval.c 尾部 */
return NULL;   /* 恢复 d68248e 语义 — 非字符串表达式交由调用方走 eval_numeric */
```

---

## 二、🟧 HIGH-1：多维数组读取回退 — `A(I,J)` 返回 0/垃圾

### 问题代码（gweval.c parse_factor 数组分支）
```c
        /* Check for array reference: A(5) */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            float index_val = parse_expression(p);   /* ← 只解析一个索引（遇 ',' 停止）*/
            int index = (int)index_val;
            if (index < 0) index = 0;
            if (parser_match(p, ')')) {              /* ← "A(I,J)" 时当前字符是 ',' → 不匹配 */
                return biptrg_get_array_element(name, index);   /* ← 跳过（不返回）*/
            }
        }
        /* fall through → 标量检查 → 返回 0 / 数组描述符垃圾 */
        struct variable *v = gwdata_find_variable(name);
        if (v && v->type == VT_SINGLE) return v->value.f;   /* 数组变量 A 的 type 若为 VT_SINGLE → 描述符垃圾 */
        return 0.0f;
```

### 回归链
- v66（f8d2d07）修复：gweval parse_factor 接入 **multi 接口**（`biptrg_get_array_element_multi`）— 多维读取正确
- e86ecd7 重构：回退为单索引（`parse_expression` 一次）→ **多维读取断裂**

### 影响
- `A(I,J)` 读取返回 0 或数组描述符垃圾值（依赖 A 的类型位）
- **为何 test_multi_array 仍 PASS**：见 HIGH-2（断言假阳性 — 两侧都是空/0 恒等）

### 修复建议
```c
/* parse_factor 数组分支需支持逗号多索引并调用 multi 接口 */
float indices[8]; int num_indices = 0;
indices[num_indices++] = parse_expression(p);
while (parser_match(p, ',')) {
    indices[num_indices++] = parse_expression(p);
}
if (parser_match(p, ')')) {
    return biptrg_get_array_element_multi(name, indices, num_indices);
}
```

---

## 三、🟧 HIGH-2：断言假阳性 — CRITICAL-1 使数值比较恒等

### 失真的断言示例
```basic
' test_multi_array.bas line 90
IF A(I, J) <> I * 10 + J THEN GOTO 130
' C1 下：A(I,J)（H1→0/垃圾）与 I*10+J（→""→0）— 但 eval_string 使 PRINT 路径返回空
' 实际比较路径：左侧 biptrg 返回值 vs 右侧表达式（C1 的 ""→eval 0）→ 0<>0 → false → 恒 PASS
```
```basic
' test_cond.bas line 20
IF -1 <> (1 < 2) THEN PRINT "FAIL"
' 字面量 -1 与比较结果 — 右侧 1<2 → eval_string("1<2")? → "" → 0 → 0<>-1 → 真 → FAIL?
' 实测却 PASS — 说明比较路径在 eval_numeric 层（IF 语句分支不走 eval_string）
```

### 结论（推导）
- **PRINT 路径**：C1 使所有数值输出空（已证）
- **IF/WHILE 条件路径**：走 `eval_numeric`（不受 C1 影响）— 但 H1（多维读取）影响数组条件
- **断言有效性**：依赖 PRINT 输出或数组读取的断言（multi_array 用 IF 比较 — 走 eval_numeric 但左侧 A(I,J) 被 H1 污染）→ **multi_array 的 PASS 不可信**（可能是 0<>0 或垃圾<>垃圾 恒等）
- **直接影响**：v76 评估的"38/40 零回归"**作废** — 真实通过数需修复 C1+H1 后重新逐项核对

### WHILE 条件的连带风险
```
WHILE I <= 5（test_while_simple）
I 递增依赖赋值（eval_numeric 路径正常）
但 C1 下若 I 输出为空不影响条件判断
→ WHILE 本身迭代次数实测正常（test_while_wend 5 次）
→ 但 test_while_simple 含 PRINT I（输出空）— 断言若依赖输出则失真
```

---

## 四、🟨 MEDIUM-1：EOF() 参数用 `parse_string_expression` 解析

### 问题代码（gweval.c parse_factor EOF 分支）
```c
else if (strcasecmp_ascii(name, "EOF") == 0) {
    char *str_val = parse_string_expression(p);       /* ← EOF(filenum) 参数是数值，却按字符串解析 */
    if (str_val) {
        if (parser_match(p, ')')) {
            int filenum = (int)gweval_eval_numeric(str_val);  /* ← 再把"字符串"转回数值 */
            return (float)dskcom_eof(filenum);
        }
    }
}
```

### 影响链
```
EOF(1) → parse_string_expression("1")
       → 非引号/非字符串函数/非变量 → 默认返回 ""（C1 同因）或 NULL
       → gweval_eval_numeric("") → 0
       → dskcom_eof(0) → 无文件 → return -1
       → EOF(1) 恒返回 -1（当作"是 EOF"）
WHILE NOT EOF(1) → NOT(-1) → 恒 false → 循环体不执行
→ file_seq/line_input: 读取循环被跳过 → 直接到 line 120 KILL → exit 0
→ **文件内容从未被实际读取验证 — exit 0 是假通过**
```

### 修复建议
```c
float fn = parse_expression(p);     /* 直接数值解析 */
if (parser_match(p, ')')) {
    return (float)dskcom_eof((int)fn);
}
```

---

## 五、🟨 MEDIUM-2：除零静默

### 问题代码（parse_term）
```c
} else if (c == '/') {
    parser_get(p);
    float right = parse_power(p);
    if (right != 0.0f) left /= right;   /* 除零：左值保留，不报错 */
}
```
GW-BASIC 语义：`X/0` 应报 "Division by zero"（或 Overflow）— 当前 `2/0` → 2（静默）。**错误被吞**。

### 修复建议
```c
if (right == 0.0f) { /* 设置错误标志 / 返回 ERROR 传播 */ }
```

---

## 六、🟩 LOW ×4

| # | 问题 | 代码位置 | 说明 |
|---|------|----------|------|
| L1 | `^` 左结合（GW-BASIC 右结合）| parse_power `while (parser_match(p,'^'))` | `2^3^2` = 64 vs GW-BASIC 512 |
| L2 | RND 忽略参数 | RND 分支 `parse_expression(p); /* consume, ignore */` | RND(0)/RND(1) 语义差异 |
| L3 | 数字解析无指数 | parse_factor 数字循环 `isdigit‖'.'` | `1.5E+10` → 截断为 1.5 |
| L4 | dskcom_eof 32/64 位比较 | `DWORD pos` vs `LARGE_INTEGER size` | >4GB 文件截断（.bas 场景低风险）|

---

## 七、合规确认（非问题 — 本次审查中验证正确的部分）

| 实现 | 结论 |
|------|------|
| KILL "file"（引号解析 + gwram_delete_file）| ✅ 基本正确（字面量形式；变量形式 KILL A$ 不支持 — 已知限用）|
| WHILE 匹配 WEND（行扫描 + 嵌套计数 + fiveo 栈）| ✅ 结构正确（跳转/返回语义与 main 配合）|
| 字符串函数分支（LEFT$/RIGHT$/MID$/... 优先返回）| ✅ 正确 |
| 一元 +/-、括号、比较 -1/0（语法层）| ✅ 正确 |
| parse_term/expression 的 +/-/* 优先级 | ✅ 正确 |

---

## 八、修复顺序与复验要求

1. **C1**（一行 `return NULL`）— 恢复数值输出
2. **复验断言真实化**：multi_array/cond/while 重新评估（此前 PASS 不可信）
3. **H1**（parse_factor 多维读取多索引）— 恢复 A(I,J)
4. **M1**（EOF 参数 parse_expression）— 恢复文件循环真实读取
5. **M2**（除零报错）
6. 全量 .bas **逐项核对输出内容**（或先补输出断言体系）— 真实通过数待修复后重测

---

## 九、总结

**总体评价**: e86ecd7 解析器重构语法方向合理，但**多重回归 + 相互掩盖**：C1（数值输出全断）→ H2（断言假阳性）→ 掩盖 H1（多维读取回退）与 M1（EOF 参数错）。**v76 的"38/40 零回归"在 C1 失真环境下不可信**；file_seq/line_input 的 exit 0 可能因 WHILE NOT EOF 恒假跳过读取（同样不可信）。全部问题在"只查 exit code + 无数值断言"的验证体系下隐身 — 已连续两轮暴露该盲区。

**关键发现**: 3（C1、H1、H2）+ 验证失真 2（M1 文件循环、v76 通过数）

**建议**: 按第八节顺序修复；修复后重跑全量 .bas 并逐项核对输出内容（或先补输出断言）— **在此之前 e86ecd7 不应视为可合并基线**。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-23
**审查版本**: v79 — e86ecd7 详细问题记录（C1 未修复状态）