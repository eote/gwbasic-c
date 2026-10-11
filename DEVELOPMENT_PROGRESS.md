# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 完整语义+错误传播 ✅) |
| **当前阶段** | CODE_REVIEW v134 修复完成 (2 MEDIUM)；等待 v135 review → M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v134 修复 (2 MEDIUM)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| MEDIUM-1 | INSTR start 非法时 FCERR 未传播到语句级 — X = INSTR(-1,...) 静默赋 -1, 无 Error at line | dispatch 3-arg 分支检测 pos<0 → gwdata_set_error(5) (Illegal function call) + return 0 | probe Error at line 20 ✅; test_gweval_instr_stmt_fcerr (ERR=5) ✅ |
| MEDIUM-2 | probe 轻量化分类歧义 (v133 LOW-2 副作用) — 字母开头一律判 2-arg, 数字变量 N / 数字函数 LEN() start 的 3-arg 被错判 SNERR | 三级形式判定: 引号 → 2-arg (无歧义, 廉价); digit/'('/')'/-/+ → 3-arg (无歧义, 廉价); 字母开头 → 回退完整 parse_string_expression probe (v132 行为): 成功→2-arg, 失败→rewind+3-arg | probe INSTR(N,...)→X=2 ✅ INSTR(LEN(S$),...)=0 (正确: LEN=3 从 3 起无 B) ✅; test_numeric_var_start/numeric_func_start ✅ |

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| X = INSTR(-1, A$, "B") | FCERR (ERR=5) + Error at line (语句级传播) |
| INSTR(N, A$, "B") (N=2 数字变量) | 3-arg → 2 |
| INSTR(LEN(S$)-3, S$, ":") | 3-arg → 2 (数字函数 start) |
| INSTR(2, ...) 数字字面量 | 3-arg (廉价 peek 路径) |
| INSTR(A$, ":") 字符串字面量 | 2-arg (廉价 peek 路径) |
| INSTR(A$) 1-arg | SNERR (v133 保持) |
| ON INSTR(A$, ":") GOTO | b2 (回归保持) |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3102/3102 PASS** (+8: FCERR 传播/数字 start 3 + start_range 语义更新 + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3102/3102 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过) ✅ |
| MEDIUM-1 probe | X = INSTR(-1, A$, "B") | **Error at line 20** (FCERR 传播) ✅ |
| MEDIUM-2 probe | INSTR(N,...) / INSTR(LEN(S$),...) | X=2 / X=0 (均正确) ✅ |
| 孤儿检测 | 定义 vs 注册 | **805 = 805, 0 孤儿** ✅ |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v135 review (v134 修复确认) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) / 其余字符串函数与 INSTR 一致性审计 (首参类型判定/错误传播同型检查) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现); .bas 套件补 INSTR 数字变量 start 集成场景 |

## 更新时间
2026-10-12

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。