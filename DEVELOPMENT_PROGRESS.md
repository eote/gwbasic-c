# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 全部完成 ✅** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / DELETE 语句+UAF ✅ / nodes[256] 动态化 ✅) |
| **当前阶段** | CODE_REVIEW v128 修复完成 (1 CRITICAL + 1 LOW)；等待 v129 复核 → M2 规划 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v128 修复 (1 CRITICAL + 1 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| CRITICAL-1 | gwlist_reseq 成功路径泄漏动态快照 nodes (每次 RESEQ 泄漏 count×8 字节, 反复 RENUM 累积) | return SUCCESS 前补 gwram_free(nodes) (一行) + gwram 分配/释放计数器 (g_alloc_count/g_free_count + getter) + test_gwlist_reseq_no_leak (100 次 RESEQ × 300 行 alloc==free 平衡断言) | 3066/3066 ✅ 泄漏平衡 PASS |
| LOW-1 | 链版本计数器未覆盖 reseq/clear (提交消息 "delete/add/reseq/clear/init 递增" 与实现不符) | gwlist_reseq / gwlist_clear 补 g_link_version++ (语义单一: 版本 = 链结构版本) | 3066/3066 ✅ |

### 修复过程中新增基础设施

1. **gwram 分配/释放计数器** (gwram.c): g_alloc_count/g_free_count + gwram_get_alloc_count/gwram_get_free_count getter; gwram_init 重置; alloc/realloc/calloc/free 全部递增 — 长期可用于 CI 泄漏检测 (reviewer 建议的"分配平衡断言"落地)

### 修复后语义总表

| 场景 | 行为 |
|------|------|
| RESEQ 成功 (任意行数) | 快照释放 (gwram_free(nodes)) + 版本递增 |
| RESEQ 溢出 (new_num > 65529) | FCERR + 快照释放 (原已正确) |
| CLEAR | 版本递增 (链结构变化) |
| 泄漏断言 | 100 次 RESEQ × 300 行: alloc == free ✅ |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3066/3066 PASS** (+2: no_leak + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3066/3066 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **68/70 PASS** (0 失败 + 2 预期跳过) ✅ |
| test_renum/test_delete_exec | RENUM/DELETE 回归 | rc=0 全绿 ✅ |
| CRITICAL-1 probe | 20 DELETE 30 / 20 RENUM 100,0,10 | 被删行不执行 / RENUM 后继续 ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

---

## 下一步

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v129 复核 (CRITICAL-1/LOW-1 修复确认) |
| 2 | M2 规划 (按 PLAN_FRESH.md) |
| 3 | 中期: 其余 65 个 .bas 补充 expected 基线 |

## 更新时间
2026-10-05

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。