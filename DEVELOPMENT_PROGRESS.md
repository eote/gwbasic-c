# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 全部完成 ✅** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / DELETE 语句+UAF ✅ / nodes[256] 动态化 ✅) |
| **当前阶段** | CODE_REVIEW v127 修复完成 (2 LOW) + nodes[256] 收尾 — M1 完成；等待 v128 review → M2+ |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v127 修复 (2 LOW) + nodes[256] 收尾 — M1 完成

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| LOW-1 | test_gwlist_delete_range_to_end 假通过 — (20,0) 提前 start>end 返回, 未验证 delete-to-end | 改 (20, 65530) (ASM ALLLST 哨兵) + 补 "20 gone" 断言 | 3064/3064 ✅ |
| LOW-2 | 每语句 2×O(n) liveness 遍历 → O(n²) | 链版本计数器 g_link_version (gwlist.c): delete/add/reseq/clear/init 递增; gwmain 两处循环快照版本, 仅链表变化时 liveness walk (O(1) fast path) | 3064/3064 全绿 ✅ |
| nodes[256] | RESEQ >256 行静默截断 (fixed array) | 动态分配 (两遍: 计数 + 收集, gwram_alloc); 错误路径 gwram_free; ASM 64K 程序 ~21K 行不再截断 | test_gwlist_reseq_large_program (300 行) ✅ |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3064/3064 PASS** (+7: to_end 修断言 + large_program + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3064/3064 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **68/70 PASS** (0 失败 + 2 预期跳过) ✅ |
| test_renum/test_delete_exec | RENUM/DELETE 回归 | rc=0 全绿 ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### M1 完成总览

| 项 | 状态 |
|----|------|
| RANDOMIZE 语句 (gwsts.c) | ✅ (v119) |
| RESEQ/DELETE range API (gwlist.c) | ✅ (v116) |
| RENUM 语句分支 (gwsts.c) | ✅ (v119) |
| DELETE 语句分支 + UAF 修复 (gwsts.c/gwmain.c) | ✅ (v125/v126) |
| nodes[256] 动态化 + 链版本计数器 | ✅ (v127) |

---

## 下一步

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v128 review (M1 收尾确认) |
| 2 | M2+ (按 PLAN_FRESH.md) |

## 更新时间
2026-10-04

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。