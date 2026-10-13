# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3+审计** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 正式闭环 ✅ / 字符串函数一致性审计 ✅) |
| **当前阶段** | M2 第 2 批第 1 项完成: 字符串函数一致性审计 (v136 计划项 1)；下一步 GENGRP 深化 (PSET/PRESET/LINE) |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M2 第 2 批第 1 项 — 字符串函数一致性审计 (复用 INSTR 模式)

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
2026-10-14

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。
