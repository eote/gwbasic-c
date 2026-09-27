# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 3/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅) |
| **当前阶段** | CODE_REVIEW v121 复核确认 (0 问题) 完成；RENUM .bas 集成测试已补；DELETE 语句分支待做 |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → .bas ✅ → DELETE 语句分支) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v121 复核 + RENUM .bas 集成测试

### v121 复核结果 (0 问题)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2989/2989 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2989/2989 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 定义 754 = 注册 754 (唯一函数名口径) | **0** ✅ |
| 集成回归 | gwdata/gwram/on_error/cond/def_fn | 全绿 ✅ |

### 本轮新增: test_renum.bas 集成测试 (v121 短期建议-2)

| 测试 | 结果 |
|------|------|
| TEST 1: RENUM 默认 (10,10) | ✅ before/after RENUM |
| TEST 2: RENUM 300 指定起点 | ✅ |
| TEST 3: 程序继续执行 (X preserved) | ✅ PASS |
| TEST 4: RANDOMIZE 回归 | ✅ PASS |
| 整体 | **rc=0 全绿** ✅ |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | **DELETE 语句分支** (gwsts.c 接入 gwlist_delete_range — 同 RENUM 模式) + 测试 |
| 2 | 提交 (test_renum.bas + DELETE 分支) → 等待 review |
| 3 | nodes[256] 上限确认 (ASM 64K 程序 — 动态扩容或提升上限) |

## 更新时间
2026-09-27

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。