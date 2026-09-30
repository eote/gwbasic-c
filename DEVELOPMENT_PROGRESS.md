# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 4/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / RENUM .bas ✅ / **DELETE 语句分支 ✅**) |
| **当前阶段** | M1 第 3 批 TDD 7 步完成 (DELETE 语句 + .bas 集成 + expected)；nodes[256] 上限确认待做 |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → DELETE ✅ → nodes[256]) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M1 第 3 批 — DELETE 语句分支 (v124 短期建议-1)

### TDD 7 步

| 步 | 内容 | 状态 |
|----|------|------|
| 1 | 查 ASM (IBMRES.ASM R2 DELETE → GWMAIN SCCLIN/SCCPTR 删除路径) | ✅ |
| 2 | 查已有: gwlist_delete_range API 已有 (test_gwlist_m1.c), 语句分支无 | ✅ |
| 3 | tests/test_gwsts_delete.c 8 测试 (API 5 + 语句级 3) — RED 确认 (链接错误 + rc=-1) | ✅ |
| 4 | src/gwsts.c DELETE 分支 — 文本解析行号 ('-' 为区间分隔符非算术) | ✅ |
| 5 | 注册 8 测试 (5 gwlist_delete_* + 3 gwsts_delete_*) + Makefile | ✅ |
| 6 | mingw32-make test 绿 | ✅ |
| 7 | 双回归 + .bas 集成 | ✅ |

### 关键修复: 行号文本解析

- 初版用 gweval_eval_numeric("20-30") → 算术求值 -10 → DELETE 20-30 误删 line 10
- ASM 语义: '-' 是 RANGE 分隔符，行号按文本解析 (atoi) — 已修复
- DELETE 90-50 → start>end → ERROR (ASM FCERR)

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3013/3013 PASS** (+24: gwlist_delete_range 3 旧 + gwsts_delete 8 新) |
| 完整测试 | ./tests/test_gwbasic.exe | **3013/3013 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **67/69 PASS** (0 失败 + 2 预期跳过), test_delete/test_renum 输出校验 ✅ |
| test_delete.bas | DELETE 单行/range/程序继续 | **rc=0 全绿** ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | nodes[256] 上限确认 (ASM 64K 程序 — 动态扩容或提升上限) |
| 2 | 为其余 64 个 .bas 测试补充 expected 基线 (当前 ⚠️ rc-only) |
| 3 | M1 完成后 v125 review → M2+ |

## 更新时间
2026-10-01

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。