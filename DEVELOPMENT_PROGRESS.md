# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3+审计** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 正式闭环 ✅ / 字符串函数一致性审计 ✅) |
| **当前阶段** | M2 第 2 批进行中: 图形语句批 (SCREEN/PSET/PRESET/LINE/POINT 接线) 完成；下一步 GRAPHIC 深化 (DRAW/PAINT/CIRCLE) 或类型系统审计收尾 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

---

---

---

## 当前轮: M2 第 2 批 — 图形语句接线 (SCREEN/PSET/PRESET/LINE/POINT)

### ASM 依据 (GENGRP.ASM L149-186 + ADVGRP)

PSET (X,Y)[,attr] 默认 FORCLR; PRESET 默认 BAKCLR; 越界静默 (JAE PSTNOT);
LINE [(x1,y1)]-(x2,y2)[,attr[,B[F]]] 省略首坐标用 graphics cursor;
POINT(x,y) 越界返回 -1。坐标/属性为表达式 (ASM SCAND/FRMEVL)。

### 实现

| # | 内容 | 文件 |
|---|------|------|
| 1 | gengrp_line_coords() 核心重构 (坐标已解析版, cursor=(x1,y1)→执行→(x2,y2)) | gengrp.c |
| 2 | gengrp_preset 加 color 参数 (-1=默认 BAKCLR) + accessor get_forclr/bakclr | gengrp.c + gwbasic.h |
| 3 | gwsts 语句分支: SCREEN/PSET/PRESET/LINE (表达式坐标, LINE 排除 LINE INPUT 遮蔽) | gwsts.c |
| 4 | gweval POINT(x,y) 数字函数 dispatch (越界 -1) | gweval.c |
| 5 | test_gwsts_graphics.c (10 测试) + test_graphics.bas/.expected + 注册 | tests/ + run_bas_tests.sh |

### 实施中修的 2 个接线 bug

1. LINE 分支遮蔽 LINE INPUT (starts_with "LINE " 先于 LINE INPUT 分支命中)
   → file_seq 挂死 (LINE INPUT 失效, WHILE NOT EOF 恒真) — 加
   !starts_with("LINE INPUT ") 排除
2. gengrp_line_coords 旧代码用 g_gxpos 而非 x1 画线 (首坐标设置不同步) —
   统一 g_gxpos=x1; 非框线 draw(x1,y1,x2,y2)

### 新单测 (+30 → 3177)

gfx_screen / pset_point / pset_default / preset_default / pset_oob /
line_simple / line_bf / line_expr / line_default_start / point_oob

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3177/3177 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **74/74**（+test_graphics；test_file_seq 等 LINE INPUT 回归全绿） |
| 孤儿检测 | ✅ **836 = 836** |
| .bas 实证 | PSET-POINT/PSET-DEF/PRESET-DEF=1/1/0, OOB-SILENT, LINE-P1/P2/BF/EXPR/DEF=1, POINT-OOB=-1 |

---
## 上一轮: CODE_REVIEW v139 (复核确认轮, 0 问题) — IF 语义正式闭环

### 复核确认

| v138 修复 | v139 独立验证 |
|----------|--------------|
| MEDIUM-1 IF 复合串条件 (AND/OR 拆分 + AND 优先折叠) | ✅ probe R1N/R2Y/R3Y + 优先级 P1Y/P2N |
| MEDIUM-2 IF..THEN..ELSE | ✅ probe R4N (else 执行) / R5Y |
| 根因: 拼接链丢尾部 (S$=AX) | ✅ |
| 根因: 类型后缀变量不可读 (B=6/C=5) | ✅ |
| 根因: 自赋值清值 | ✅ |

### 独立验证结果（当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3147/3147 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 集成套件 | ✅ **73/73 全 PASS, 0 跳过** |
| 孤儿检测 | ✅ **826=826** |

### 观察项（不计问题, 后续批次）

1. IF atom 上限 16 — 超限降级数字回退（实际条件极少超此规模）
2. `IF <cond> THEN <line#>` 行号跳转路径 — M2 IF 深化时验证
3. 后缀与数组/字段组合（`A%()`、`A!$`）+ 无括号字符串函数调用 — 类型系统审计收尾

---
## 上一轮: CODE_REVIEW v138 修复 — 2 MEDIUM + 暴露的 2 个既有根因

### 修复内容

| # | 问题 | 修复 | 文件 |
|---|------|------|------|
| MEDIUM-1 | IF 串复合条件 `"A"="A" AND "B"="C"` 错误 true（旧串比较只取第一个比较符，AND/OR 尾被丢弃） | 条件按顶层 AND/OR 分词（引号/括号感知, GW-BASIC AND 优先于 OR），逐 atom 求值（双侧串 → strcmp 真比较，否则数值），AND 组先折叠再 OR 合并 | gwsts.c |
| MEDIUM-2 | IF..THEN..ELSE 的 ELSE 分支静默不执行（既有缺口） | 引号/括号感知扫描顶层 " THEN "/" ELSE "，else_stmt 分支执行；was_in_handler 快照不影响 | gwsts.c |
| 暴露-1 | **自赋值 `S$ = S$ + "X"` 丢旧值**（probe 实证 LEN=1）— 既有根因 | assign_value 字符串路径先求值 RHS 再 alloc_variable（alloc 对已存在串变量清 ptr=NULL，旧顺序自引用读到 NULL）；数值路径本就先求值 | gwsts.c |
| 暴露-2 | **类型后缀变量 `A%`/`A!`/`A#` 表达式不可读**（parse 在 % 截断 → find_variable("A") 落空 → 恒 0）— 既有根因 | parse_factor 名字字符集补 %/!/# | gweval.c |
| 附带 | .expected 3 个文件系旧 bug 产物（test_gwdata 缺 A% 行/40-char 名字长度不等、test_int_var/let 记录 0、test_gwram 缺 concat PASS） | 按修正后真值更新 + test_gwdata.bas 变量名长度 bug 修正（28≠32） | tests/bas/ |
| 新增 | test_if_else.bas+.expected（ELSE/复合/函数比较 6 场景）+ run_bas_tests.sh 注册 | tests/bas/ + run_bas_tests.sh |

### 新单测 (+9, 3147 总)

if_compound_str_and / if_compound_str_or / if_else_both (+v137 轮 7 个 IF/STRING$ 用例后总计 18)

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3147/3147 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **73/73**（+test_if_else；test_gwdata/let/int_var/gwram 真值修正后全 PASS） |
| 孤儿检测 | ✅ **826 = 826** |
| probe 实证 | ✅ AND(真/假)/OR(假/真)/优先级 (T AND T) OR F=T；ELSE 双分支；自赋值 LEN 1→2→3；A%=100 IF/赋值 |

---
## 上一轮: M2 第 2 批第 1 项 — 字符串函数一致性审计 (复用 INSTR 模式, 8cad67d)

---

## 当前轮: CODE_REVIEW v137 修复 — 2 MEDIUM (类型判定/类型安全)

### 修复内容

| # | 问题 | 修复 | 文件 |
|---|------|------|------|
| MEDIUM-1 | STRING$(3, S$) 字符串变量参数 → 空串 | 第 2 参字母/引号开头 → 完整字符串解析 probe (v134 INSTR 三级判定模式); 失败回退数字形式 (STRING$(3, LEN("XY")) 仍可用) | gweval.c |
| MEDIUM-2 | 数字上下文字符串函数静默 0 (Y = CHR$(256)) | parse_factor: is_str_func → 消费参数至匹配 ')' → set_error(2) SNERR (类型不匹配); 覆盖全部 9 个字符串函数 | gweval.c |
| 附带 | IF 字符串比较假语义 (两侧 accident-0 → 恒真) | gwsts IF: 顶层比较符扫描 (引号/括号感知) + 双侧字符串 → strcmp 真语义; 否则数值路径 | gwsts.c |
| 附带 | test_gwdata.expected 缺 3 行 (LEFT$/RIGHT$/MID$ 比较旧行为被吞) | .expected 补真值 (真实串比较全 PASS) | .expected |

### 新单测 (+7, 3138 总)

string_vararg / string_funcarg / string_numericfuncarg / numeric_ctx_chr / numeric_ctx_left / numeric_ctx_if / strctx_chr_regression

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3138/3138 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **72/72** (test_gwdata LEFT$/RIGHT$/MID$ 真串比较全 PASS) |
| 孤儿检测 | ✅ **823 = 823** |
| probe 实证 | ✅ STRING$(3,S$)→LEN=3; Y=CHR$(256)→Error@10; X=LEFT$→Error@10; IF CHR$(65)→Error@10; IF A$="X"→RIGHT-FALSE; IF LEFT$(...)=→FUNC-CMP-OK |

---
### ASM 语义依据 (BISTRS.ASM + GWEVAL.ASM)

| 函数 | ASM 证据 | 语义 |
|------|---------|------|
| CHR$ | CONINT (GWEVAL L1615-1621) | 参数 0..255, 高字节非 0 → FCERR |
| LEFT$/RIGHT$ | PREAM/GETBYT | n 为字节 0..255, 越界 → FCERR; n=0 → ""; n>len → 整串 |
| MID$ | L763-796 | start==0 → FCERR ("IT MUST NOT BE 0"); start/len 字节 0..255; 2-arg 形式 = 剩余串 |
| SPACE$/STRING$ | STRNG$ L658/SPACE$ | n 字节 0..255 → FCERR; STRING$ 第二参 GETYPR: 串 → 首字符, 数值 → CONINT |

### 修复内容

| # | 修复 | 文件 |
|---|------|------|
| 1 | CHR$/LEFT$/RIGHT$/MID$/SPACE$/STRING$ dispatch 范围校验 → set_error(5) (FCERR) | gweval.c |
| 2 | MID$ 2-arg 形式支持 (rest of string) | gweval.c |
| 3 | STRING$ 第二参类型 probe (引号 → 串形式取首字符; 否则数值) | gweval.c |
| 4 | 字符串字面量分配失败 → OM (ERR=14) + dispatch NULL → 错误感知 | gweval.c |
| 5 | assign_value: eval NULL + error 已置 → 不再回退字面量 (修复源码文本污染) | gwsts.c |
| 6 | PRINT: eval NULL + error → 中止 PRINT (不再数值回退打 " 0") | gwsts.c |
| 7 | **错误处理器内错误 = FATAL** (was_in_handler 快照区分 ERROR 语句合法进入) — 消灭 OM 无限循环 | gwmain.c + gwdata.c |
| 8 | test_gwram.expected 更新 (TEST 6 OM 传播新行为: Error at line 800) + TERMERROR_TESTS 注册 | .expected + run_bas_tests.sh |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3123/3123 PASS** (+11 strfuncs: chr 256/neg/ok, left neg/zero, right neg, mid start0/2arg, space range, string strarg/range) |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **72/72** (test_gwram → termerror 内容匹配; test_string .expected 修正 STRING$(10,"*")="**********" — 旧 expected 系 bug 产物) |
| 孤儿检测 | ✅ **816 = 816** |
| probe 实证 | ✅ CHR$(256)→Error@10; MID$ 2-arg→BCDE; STRING$(3,"AB")→AAA; OM handler→fatal (无 " 0 2" 死循环); test_error_stmt ERR=11 回归通过 |

### 附带修复 (v129 已知缺口部分闭合)

字符串堆耗尽 OM 传播: 字面量分配 → set_error(14); 错误处理器内再错 → FATAL 终止 (GW-BASIC 规则)。剩余: 非 PRINT 路径的 gwram_alloc NULL 全面审计 (中期项)。

---

## 下一步 (M2 第 2 批续)

| 优先级 | 任务 |
|--------|------|
| 1 | GENGRP 深化: PSET/PRESET/LINE (ASM 对照 + TDD) |
| 2 | 中期: gwram_alloc NULL 全面 OM 审计; INSTR 数字变量 start .bas 场景; CI 门禁 |

## 更新时间
2026-10-18

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。
