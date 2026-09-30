# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 3/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / RENUM .bas+expected ✅) |
| **当前阶段** | CODE_REVIEW v124 复核确认 (0 问题) 完成；DELETE 语句分支待做 (短期建议-1) |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → .bas ✅ → DELETE 语句分支) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v124 复核确认 (0 问题)

### v124 复核结果

0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW — 复核确认轮（自 v123 后无代码变更，仅文档更新 60f0979）

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **2989/2989 PASS** |
| 完整测试 | ./tests/test_gwbasic.exe | **2989/2989 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 定义 754 = 注册 754 (唯一函数名口径) | **0** ✅ |
| 套件集成 | bash run_bas_tests.sh | **66/68 PASS** (0 失败 + 2 预期跳过), test_renum 输出校验 ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### 里程碑状态

| 里程碑 | 进度 |
|--------|------|
| M0 | **4/4 完成** ✅ |
| M1 | **3/N** (RANDOMIZE ✓, RESEQ ✓, DELETE range API ✓, RENUM 语句 ✓, RENUM .bas+expected ✓) |
| CODE_REVIEW.md 入库 | ✅ 90514a3 |

---

## 下一步 (M1 剩余, v124 建议)

| 优先级 | 任务 |
|--------|------|
| 1 | **DELETE 语句分支** (gwsts.c 接入 gwlist_delete_range — 同 RENUM 模式) + 语句级 .bas 集成测试 (含 expected; 用例含 start>end/越界/空程序) |
| 2 | 为其余 65 个 .bas 测试补充 expected 基线 (当前 ⚠️ rc-only) |
| 3 | nodes[256] 上限确认 (ASM 64K 程序) → M1 完成后 v125 review → M2+ |

## 更新时间
2026-10-01

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。