# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 3/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / RENUM .bas+expected ✅) |
| **当前阶段** | CODE_REVIEW v122 修复确认 (4 LOW 全修复)；DELETE 语句分支待做 (短期建议-2) |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → .bas ✅ → DELETE 语句分支) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v122 修复 (4 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| LOW-1 | test_renum.bas 未收录 run_bas_tests.sh | IMPLEMENTED_TESTS 加入 "test_renum.bas" | 套件运行 ✅ |
| LOW-2 | 无 expected 输出基线 | tests/bas/test_renum.expected（10 行全量输出） | 输出校验 ✅ PASS |
| LOW-3 | v121 草稿数字 2998≠2989 | 草稿已被 v122 覆盖；统一采用 test_runner TEST SUMMARY 口径 | 2989 权威 ✅ |
| LOW-4 | v122 日志过程噪音 | 删除 build_v122/test_output_v122/test_run_v122/full_build.log | 工作区干净 ✅ |

### expected 命名约定（重要发现）

- run_bas_tests.sh compare_output 用 `$TEST_DIR/${test_name}.expected`（test_name = basename 去 .bas）
- 即 **test_X.expected**（非 test_X.bas.expected）— 初版命名错误已修正

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 套件集成 | bash run_bas_tests.sh | **66/68 PASS** (0 失败 + 2 预期跳过), test_renum ✅ PASS 输出校验 |
| 注册测试 | mingw32-make test | 2989/2989 PASS |
| 孤儿 | 754 = 754 | 0 ✅ |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | **DELETE 语句分支** (gwsts.c 接入 gwlist_delete_range — 同 RENUM 模式) + 测试 |
| 2 | nodes[256] 上限确认 (ASM 64K 程序 — 动态扩容或提升上限) |
| 3 | M1 完成后 → M2+ |

## 更新时间
2026-09-29

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。