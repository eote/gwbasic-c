# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / 第 1 批 v130 修复 ✅) |
| **当前阶段** | CODE_REVIEW v130 修复完成 (1 MEDIUM + 3 LOW)；等待 v131 review → M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v130 修复 (1 MEDIUM + 3 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| MEDIUM-1 | ON 关键字 strstr 首次匹配 — 变量名含 GOTO/GOSUB 子串 (XGOTO/MYGOSUB) 时表达式被截断, 合法程序行为错误; 连带: 缺关键字语句被静默接受 | 关键字搜索改从后向前 (最后出现) + token 边界校验 (前导字符为 letter/_ /$ → 变量名内, 拒绝 → SNERR; digit → 紧凑索引形式, 接受) | probe ON XGOTO GOTO → b2 ✅; 3 新单测 ✅ |
| LOW-1 | starts_with("ON ") 要求空格 — 紧凑写法 ON2GOTO 不支持 | starts_with("ON") + 跳过空白 (与 RENUM/DELETE 分支一致) | probe ON2GOTO 100 → 跳转 ✅ |
| LOW-2 | targets[64] 上限 — 超出静默忽略 | 保留观察 (reviewer: 实际使用极少超 64, ASM JSY 无硬限制; 修复成本 > 收益) | — |
| LOW-3 | terminating-error 校验路径外延 (FAIL_SYNTAX + expected 匹配 → PASS 对所有测试) | 收窄到显式标记: TERMERROR_TESTS 列表 + run_test 第 3 参数 "termerror"; 仅标记测试启用该路径, 普通测试语法错误仍 FAIL | test_onerr_off ✅ (标记生效) |

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| ON XGOTO GOTO 100,200 (XGOTO=2) | 关键字=最后出现的 GOTO (token 边界) → 第 2 目标 |
| ON XGOTO 100 (无真关键字) | 唯一匹配在变量名内 → SNERR |
| ON2GOTO 100 (紧凑) | starts_with("ON") + digit 边界接受 → 跳转 |
| ON I GOTO 越界/0 | SUCCESS → 下一语句 (ASM L2453-2456) |
| 普通测试语法错误回归 | FAIL (termerror 路径不启用) ✅ |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3075/3075 PASS** (+3 回归) |
| 完整测试 | ./tests/test_gwbasic.exe | **3075/3075 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **72/72 全 PASS** (0 失败 + 0 跳过) ✅ |
| MEDIUM-1 probe | ON XGOTO GOTO (XGOTO=2) | **b2** (不再 fallthrough) ✅ |
| LOW-1 probe | ON2GOTO 100 | 跳转 100 ✅ |
| UAF 回归 | probe 20 DELETE 30 | LINE 30 不执行 ✅ |
| 集成回归 | gwdata 8, on_error ERR=11, 套件 0 失败 | ✅ |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v131 review (MEDIUM-1/LOW 修复确认) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) / BISTRS 深化 (按 PLAN_FRESH M2) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现: gwdata_alloc_string NULL 静默降级) |

## 更新时间
2026-10-06

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。