# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 2/N** |
| **当前阶段** | CODE_REVIEW v117 验证确认 (0 CRITICAL/HIGH/MEDIUM/LOW) 完成；发现 RESEQ 语句分支缺失 (同 RANDOMIZE 模式) |
| **依赖顺序** | M0 ✅ → M1 (RANDOMIZE ✅ → RESEQ/DELETE API ✅ → **RENUM 语句分支 待做**) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v117 验证 + RESEQ 语句分支发现

### v117 验证结果 (0 问题)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2976/2976 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2976/2976 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 声明=注册=747 | **0** ✅ |
| 集成回归 | gwdata/gwram/on_error/cond/def_fn | 全绿 ✅ |

### 新发现: RESEQ 语句分支缺失 (同 RANDOMIZE 模式)

| 项 | 说明 |
|----|------|
| 现象 | `RESEQ` 作为语句执行 → "Error at line 30" (gwsts_exec_statement 无 RENUM/RESEQ 分支) |
| 已有 | gwlist_reseq/gwlist_delete_range API 完成 (commit 0cf6470)；ibmres.c 关键字 "RENUM" 0x25 已注册 |
| 待做 | gwsts.c 新增 RENUM 分支: 解析 [NN[,MM[,INC]]] → gwlist_reseq；同 RANDOMIZE 模式 |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | **RENUM 语句分支** (gwsts.c: 解析 → gwlist_reseq/delete_range 接入) + 测试 |
| 2 | .bas 集成测试补充 (RESEQ/DELETE/RANDOMIZE 实际运行) |
| 3 | nodes[256] 上限确认 (ASM 64K 程序可达更多行 — 动态扩容或提升上限) |

## 更新时间
2026-09-23

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。