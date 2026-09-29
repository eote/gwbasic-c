# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 3/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / RENUM .bas+expected ✅) |
| **当前阶段** | CODE_REVIEW v123 确认 (v122 4 LOW 修复验证通过; 唯一 LOW-1 报告入库已修复)；DELETE 语句分支待做 |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → .bas ✅ → DELETE 语句分支) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v123 确认 + LOW-1 修复 (报告入库)

### v123 复核结果

v122 4 LOW 修复确认（全部验证通过）— 0 CRITICAL / 0 HIGH / 0 MEDIUM / 1 LOW

| # | v122 问题 | 修复 (b01128f) | v123 独立验证 |
|---|-----------|----------------|--------------|
| LOW-1 | test_renum.bas 未收录套件 | IMPLEMENTED_TESTS 添加 | ✅ 套件 68 项自动运行 |
| LOW-2 | 无 expected 基线 | test_renum.expected (10 行) | ✅ 输出校验 PASS |
| LOW-3 | 草稿数字 2998≠2989 | 统一 test_runner 口径 | ✅ 2989 权威一致 |
| LOW-4 | v122 日志噪音 | 删除 4 个日志 | ✅ 工作区干净 |

### v123 唯一遗留: LOW-1 报告入库 — 本轮修复

- CODE_REVIEW.md (v123 报告) 随本提交一并入库，恢复文档单一事实源
- HEAD 中 CODE_REVIEW.md 此前停在 v106 — v107-v123 结论现全部入文档

### 验证结果 (独立执行)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 套件集成 | bash run_bas_tests.sh | **66/68 PASS** (0 失败 + 2 预期跳过), test_renum 输出校验 ✅ |
| 注册测试 | mingw32-make test | **2989/2989 PASS** |
| 完整测试 | ./tests/test_gwbasic.exe | **2989/2989 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 定义 754 = 注册 754 | **0** ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

---

## 下一步 (M1 剩余, v123 建议)

| 优先级 | 任务 |
|--------|------|
| 1 | **DELETE 语句分支** (gwsts.c 接入 gwlist_delete_range — 同 RENUM 模式) + 语句级 .bas 集成测试 (含 expected) |
| 2 | 为其余 65 个 .bas 测试补充 expected 基线 (当前 ⚠️ rc-only) |
| 3 | nodes[256] 上限确认 (ASM 64K 程序) → M1 完成后 M2+ |

## 更新时间
2026-09-30

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。