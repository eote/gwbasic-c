# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 2/N** (GWSTS 深化 ✅ / GWLIST 行管理 ✅) |
| **当前阶段** | CODE_REVIEW v117 验证确认 (0 CRITICAL/HIGH/MEDIUM/LOW) — 无待办修复；.bas 集成测试补充待做 |
| **依赖顺序** | M0 ✅ → M1 (RANDOMIZE ✅ → RESEQ/DELETE ✅ → .bas 覆盖) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v117 验证记录

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2976/2976 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2976/2976 PASS, rc=0** |
| 编译警告 | 干净重建 (生产+测试) | **0 条** |
| 孤儿 | 声明=注册=747, diff 空 | **0** ✅ |
| 重复注册 | uniq -d | **0** ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### 问题清单
**0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW** — 无待办修复。
v117 中期建议：.bas 集成测试（RESEQ + DELETE 实际运行）、nodes[256] 数组大小确认。

---

## 里程碑状态

| 里程碑 | 进度 |
|--------|------|
| M0 (GWDATA/GWRAM/GWINIT/GWMAIN) | **4/4 完成** ✅ |
| M1 (GWSTS 深化 + GWLIST 行管理) | **2/N** (RANDOMIZE ✅, RESEQ/DELETE ✅) |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 | 说明 |
|--------|------|------|
| 1 | **RENUM/DELETE 语句分支** | gwsts_exec_statement 无 RENUM 分支 (仅关键字表注册) — 同 RANDOMIZE 模式接入 gwlist_reseq/delete_range |
| 2 | .bas 集成测试补充 | RESEQ/DELETE/RANDOMIZE 实际运行 .bas；提升 65/67 覆盖 |
| 3 | nodes[256] 上限确认 | ASM 程序可达 64K 行数 — 动态扩容或提升上限 |

## 更新时间
2026-09-23

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。