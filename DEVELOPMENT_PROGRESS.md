# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 正式闭环 ✅) |
| **当前阶段** | CODE_REVIEW v135 修复确认轮 0 问题 (INSTR 闭环)；等待下一步 M2 第 2 批 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v135 (修复确认轮, 0 问题) — INSTR 正式闭环

### 复核确认

| v134 问题 | 修复 (c85284e) | 独立验证 |
|-----------|--------------------|----------|
| MEDIUM-1 FCERR 未传播 | dispatch pos<0 → set_error(5) + return 0 | ✅ probe INSTR(-1,...) → Error at line 10 |
| MEDIUM-2 形式判定歧义 | 三级判定: 引号/数字 (廉价无歧义) + 字母开头回退完整 probe (复用/零分配) | ✅ probe INSTR(N,...)→2; INSTR(LEN("BC")+1,...)→3 |

### 独立验证结果（当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建 | ✅ exit 0, **0 警告** |
| 单元测试 | ✅ **3102/3102 PASS** |
| 套件集成 | ✅ **72/72 全 PASS, 0 跳过** |
| 孤儿检测 | ✅ **805 = 805, 0 孤儿** |
| probe 6 项 | ✅ FCERR 传播 / 数字变量 start / 数字函数 start / ON INSTR 回归 / 1-arg SNERR / 引号感知 |
| 集成回归 | gwdata 8, gwram 4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### INSTR 函数闭环总览 (4 轮 review: v132→v135)

| 轮 | 内容 |
|----|------|
| v132 | bistrs_instr 实现 (ASM INSTRC 1:1) + 引号感知冒号截断 + gweval dispatch |
| v133 | start 1..255 范围校验 → FCERR + dispatch 失败 SNERR + probe 轻量化 |
| v134 | FCERR 语句级传播 (set_error(5)) + 三级形式判定 (字母回退完整 probe) |
| v135 | 确认轮 0 问题 — 正式闭环 |

---

## 下一步 (M2 第 2 批)

| 优先级 | 任务 |
|--------|------|
| 1 | 字符串函数一致性审计 (LEFT$/MID$/RIGHT$/CHR$/STR$/SPACE$/STRING$/UCASE$/LCASE$ — 首参类型判定/错误传播同型检查) |
| 2 | M2 第 2 批: GENGRP 深化 (PSET/PRESET/LINE) |
| 3 | 中期: 字符串堆耗尽 → OM 错误传播 (v129 发现); .bas 套件补 INSTR 数字变量 start 集成场景; CI 门禁 |

## 更新时间
2026-10-13

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。