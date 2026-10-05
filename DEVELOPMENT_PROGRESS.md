# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ (M1) / .bas 基线补齐 ✅ / ON..GOTO/ON..GOSUB 语句 ✅) |
| **当前阶段** | M2 第 1 批完成 (TDD 7 步)；等待 CODE_REVIEW → M2 剩余 (GENGRP 深化 / BISTRS 深化) |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 (GWLIST ✅ → 基线 ✅ → ON..GOTO ✅) → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M2 第 1 批 — .bas 基线补齐 + ON..GOTO/ON..GOSUB 语句 (TDD 7 步)

### 完成记录

| # | 任务 | 实现 | 验证 |
|---|------|------|------|
| 1 | .bas 基线补齐 (v129 M2 建议) | 审计发现全部 68 个已实现 .bas 均有 .expected + 输出校验 (v129 "65 个 rc-only" 备注有误); 仅 test_gwdata/test_gwram 缺失 → 补 .expected (15/14 行) + run_bas_tests.sh 注册 | 套件 70/72 ✅ |
| 2 | ON..GOTO/ON..GOSUB 语句 (ASM NTONG1/LOOPON L2446-2460) | gwsts.c ON 分支 (ON ERROR 排除): 表达式求值 (keyword 前文本) + 逗号分隔目标解析 (64 上限) + 1-based 选择; 越界/0 → SUCCESS (ASM L2453-2456 RET 列表耗尽); ON..GOSUB → next86_gosub_to_line (SETGSB push 语义, 从 next86_gosub 重构共享) | 6 单测 RED→GREEN; test_on_goto.bas 3 分支正确 ✅ |
| 3 | test_on_goto/test_onerr_off 转正 (v129 长期建议-5) | run_bas_tests.sh: 新增 terminating-error 校验路径 (rc=127 + "Error at line" 但 .expected 精确匹配 → PASS — ON ERROR GOTO 0 终止错误是正确 GW-BASIC 语义); 两测试移入 IMPLEMENTED; NOT_IMPLEMENTED 清空 | 72/72 全 PASS, 0 跳过 ✅ |

### ASM 语义确认

- **NTONG1/LOOPON (GWMAIN.ASM L2446-2460)**: GETBYT → 1-based 索引; LOOPON DEC CL 逐个跳过行号 (LINGT2), 逗号分隔; 列表耗尽 → RET → 下一语句 (无分支) — 与 GW-BASIC 文档一致
- **ONGOTO (L2357)**: ON ERROR 分支 (SYNCHR $GOTO + LINGET + FNDLN1 + ONELIN); ON..GOSUB → ISGOSU/SETGSB push
- **SETGSB**: push 返回地址 (下一行) → 跳转目标 — next86_gosub_to_line 1:1 复刻

### 修复过程中新增基础设施

1. **next86_gosub_to_line(int line_num)** (next86.c): 从 next86_gosub 重构 — GOSUB <line> 与 ON..GOSUB 共享 push+jump 逻辑
2. **run_bas_tests.sh terminating-error 校验路径**: .expected 精确匹配的 rc=127 测试可 PASS (错误场景基线从此可校验)

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3072/3072 PASS** (+6 ongoto) |
| 完整测试 | ./tests/test_gwbasic.exe | **3072/3072 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过 — 首次全绿) ✅ |
| test_on_goto.bas | 3 分支正确跳转 | ✅ |
| test_onerr_off.bas | ON ERROR GOTO 0 终止错误语义 | ✅ (output match) |
| 集成回归 | gwdata 8, gwram 4, on_error ERR=11, cond/def_fn 全绿, probe UAF 全绿 | ✅ |

---

## 下一步 (M2 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 CODE_REVIEW (M2 第 1 批) |
| 2 | M2 剩余: GENGRP 深化 (PSET/PRESET/LINE 图形) / BISTRS 深化 |
| 3 | 中期: 字符串堆耗尽 → OM 错误 (ERR=7) 传播 (本轮发现: gwdata_alloc_string NULL 静默降级; DIM 大数组静默成功) |

## 更新时间
2026-10-06

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。