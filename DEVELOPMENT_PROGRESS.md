# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR+引号感知 ✅) |
| **当前阶段** | CODE_REVIEW v132 修复完成 (1 MEDIUM + 连带 INSTR 实现)；等待 v133 review → M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v132 修复 (1 MEDIUM + INSTR 实现)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| MEDIUM-1 | 冒号截断不感知字符串字面量 — INSTR(A$, ":") 等带冒号字符串字面量被 strchr(':') 误截 → SNERR | ON 分支截断改引号感知扫描 (in_string 状态翻转 on '"', GW-BASIC 无转义序列, "" 仍是字面量内容 — ASM 语义) | probe b2 ✅; test_gwsts_on_goto_string_colon ✅ |
| 连带 | INSTR 函数未实现 (reviewer probe 前提; BISTRS.ASM L841-949 INSTRC) | bistrs_instr (s1, s2, start) 1:1 ASM INSTRC: start=0→FCERR(-1); start>LEN→0; S2 空→return start; S1 空→0; found→1-based 首现位置; gweval INSTR dispatch (probe+rewind 形式判定: 首参字符串→2-arg 形式 start=1; 首参数字→3-arg); is_str_numeric 组加入 | 7 API 单测 + test_gweval_instr_function; probe INSTR(A$,":")=2, INSTR(2,A$,":")=2 ✅ |
| 连带 | 赋值分支劫持 ON 语句 (ON (A$="X:Y")... 带 '=' 被劫持 → SNERR) | 赋值分支排除 bare "ON " / "ON" (同 OPEN/FIELD 模式) | 3078→3088 全绿 ✅ |

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| ON INSTR(A$, ":") GOTO 100,200 (INSTR=2) | 引号内冒号不截断 → 第 2 目标 |
| INSTR("X:Y", ":", 1) | 2 (1-based 首现) |
| INSTR(2, "A:B:C", ":") | 2 (从 start 位置起含 start) |
| INSTR start=0 | FCERR (-1) |
| INSTR start>LEN | 0 |
| INSTR S2 空 | return start |
| ON X GOTO 100 : GOTO 500 | b100 (v131 保持) |
| ON GOTOX 100,200 | SNERR (v131 保持) |
| ON (A$="X:Y")... | 不再被赋值分支劫持 |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3088/3088 PASS** (+10: INSTR 7 + string_colon/instr_function + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3088/3088 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过) ✅ |
| MEDIUM-1 probe | ON INSTR(A$, ":") GOTO | **b2** (不再 SNERR) ✅ |
| v131 回归 | 多语句行 ON X GOTO 100 : GOTO 500 | b100 保持 ✅ |
| 孤儿检测 | 定义 vs 注册 | **798 = 798, 0 孤儿** ✅ |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v133 review (v132 修复确认) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) / BISTRS 深化 (INSTR 左侧 MID$/INSTR 语句级剩余) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现); INSTR(S$,":",3) 非法形式 (ASM 仅 2 形式) 返回 0 边缘观察 |

## 更新时间
2026-10-10

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。