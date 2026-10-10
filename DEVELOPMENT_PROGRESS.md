# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 完整语义 ✅) |
| **当前阶段** | CODE_REVIEW v133 修复完成 (1 MEDIUM + 2 LOW)；等待 v134 review → M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v133 修复 (1 MEDIUM + 2 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| MEDIUM-1 | INSTR start 范围校验缺失 — 负值 (size_t 转换) / >255 静默返回 0; GW-BASIC 手册: start 1..255 否则 FCERR | bistrs_instr 前置 `start < 1 \|\| start > 255 → -1` (FCERR); 注释说明 ASM 字节语义 (AL) 与 C int 差异 — ASM ≤255 字节串下 start>255 不可能出现在有效区间, port 允许更长串故按手册封顶 | probe B=-1 ✅; test_bistrs_instr_negative_start/start_too_large ✅ |
| LOW-1 | INSTR 非法形式静默 — INSTR(A$) (1-arg) / S2 解析失败 → 静默 0, 注释声称 "falls through to error handling" 实无 | dispatch 失败路径显式 gwdata_set_error(SNERR=2) + return 0 | probe Error at line 20 ✅; test_gweval_instr_one_arg_snerr ✅ |
| LOW-2 | dispatch probe 额外分配 — parse_string_expression 探测后 rewind, 每次 INSTR 浪费一份字符串堆拷贝 | 轻量类型探测: peek 首个非空白字符 ('"' → 字符串字面量; letter/_/$ → 字符串变量/函数 → 2-arg; digit/'(' → 3-arg) | 3094/3094 ✅ |

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| INSTR(-1, A$, "B") | FCERR (-1) |
| INSTR(300, A$, "B") | FCERR (-1) |
| INSTR(0, ...) | FCERR (-1) (v132 保持) |
| INSTR(1..255, S1$, S2$) | 1-based 首现 / not found 0 / S2 空 → start |
| INSTR(A$) (1-arg) | SNERR (ERR=2), 不再静默 |
| ON INSTR(A$, ":") GOTO | b2 (v132 保持) |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3094/3094 PASS** (+6: 范围 2 + dispatch 2 + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3094/3094 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过) ✅ |
| MEDIUM-1 probe | INSTR(-1,...) | **B=-1** (FCERR) ✅ |
| LOW-1 probe | INSTR(A$) 1-arg | **Error at line 20** (SNERR) ✅ |
| v132 回归 | ON INSTR(A$, ":") GOTO | b2 保持 ✅ |
| 孤儿检测 | 定义 vs 注册 | **802 = 802, 0 孤儿** ✅ |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v134 review (v133 修复确认) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) / BISTRS 深化 (其余字符串函数与 INSTR 一致性审计: range 校验/非法形式错误处理) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现) |

## 更新时间
2026-10-11

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。