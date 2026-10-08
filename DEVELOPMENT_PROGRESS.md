# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / v130+v131 修复 ✅) |
| **当前阶段** | CODE_REVIEW v131 修复完成 (1 MEDIUM + 2 LOW)；等待 v132 review → M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v131 修复 (1 MEDIUM + 2 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| MEDIUM-1 | ON 关键字解析无语句边界 — 多语句行 (": GOTO 500") 误跳 500; REM 注释含 GOTO 时语句失效 | ON 分支在首次冒号截断 (strchr ':') — 关键字搜索与目标解析止于语句边界; 冒号后内容不参与解析 (与执行模型无冒号链一致) | probe b100/b100 ✅; 3 新单测 ✅ |
| LOW-1 | kw==rest 跳过边界校验 — "ON GOTO 100" (缺表达式) / "ON GOTOX ..." (变量名前缀) 静默接受 | kw==rest 一律拒绝 → SNERR | probe GOTOX → Error at line 20 ✅ |
| LOW-2 | eval_numeric 宽松解析 (观察项) | 保留 (reviewer: MEDIUM-1 修复后不再暴露; 与 RENUM 共用) | — |

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| ON X GOTO 100 : GOTO 500 (X=1) | 目标 [100] → b100 (冒号后不解析) |
| ON X GOTO 100 : REM GOTO later | 目标 [100] → b100 (REM 不污染) |
| ON GOTOX 100, 200 (缺真关键字) | SNERR (kw==rest 拒绝) |
| ON XGOTO GOTO 100,200 (XGOTO=2) | b2 (v130 修复保持) |
| ON2GOTO 100 (紧凑) | 跳转 (v130 修复保持) |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3078/3078 PASS** (+3 回归) |
| 完整测试 | ./tests/test_gwbasic.exe | **3078/3078 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过) ✅ |
| MEDIUM-1 probe | 多语句行 / REM 注释 | b100 / b100 ✅ |
| LOW-1 probe | ON GOTOX 100, 200 | Error at line 20 (SNERR) ✅ |
| v130 回归 | ON XGOTO GOTO / ON2GOTO | b2 / 跳转 保持 ✅ |
| UAF 回归 | probe 20 DELETE 30 | LINE 30 不执行 ✅ |
| 集成回归 | test_on_goto 3 分支, gwdata 8, on_error ERR=11 | ✅ |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v132 review (v131 修复确认) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) / BISTRS 深化 (按 PLAN_FRESH M2) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现: gwdata_alloc_string NULL 静默降级) |

## 更新时间
2026-10-08

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。