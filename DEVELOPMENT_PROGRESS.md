# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 正式关闭 ✅** (RANDOMIZE / RESEQ / DELETE / RENUM / UAF 修复 / nodes[256] 动态化 / 泄漏关闭, 7 项全部完成) |
| **当前阶段** | CODE_REVIEW v129 修复确认轮 0 问题 — M1 关闭；等待下一步 M2 规划 |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v129 (修复确认轮, 0 问题) — M1 正式关闭

### 复核确认

| v128 问题 | 修复 (0497172) | 独立验证 |
|-----------|--------------------|----------|
| CRITICAL-1 RESEQ 成功路径泄漏 | gwram_free(nodes) + gwram 计数器 + test_gwlist_reseq_no_leak (100×RESEQ alloc==free) | ✅ 代码复核 + balance 断言 PASS |
| LOW-1 版本计数器未覆盖 reseq/clear | reseq/clear 补 g_link_version++ | ✅ 代码复核 + RENUM probe 无回归 |

### 独立验证结果（当前 checkout 全量重跑）

| 验证项 | 命令 | 结果 |
|----------|------|------|
| 干净重建 | mingw32-make clean && mingw32-make | ✅ exit 0, **0 警告** |
| 单元测试 | mingw32-make test | ✅ **3066/3066 PASS** |
| 完整测试 | ./tests/test_gwbasic.exe | ✅ **3066/3066 PASS, rc=0** |
| 套件集成 | bash run_bas_tests.sh | ✅ **68/70 PASS** (0 失败 + 2 预期跳过) |
| 孤儿检测 | 定义 vs 注册 | ✅ **777 = 777, 0 孤儿 / 0 多余** |
| CRITICAL-1 probe | 20 DELETE 30 | ✅ 被删行不执行 |
| RENUM probe | 20 RENUM 100,0,10 | ✅ 程序正确继续 |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### M1 关闭总览 (7 项)

| 项 | 状态 |
|----|------|
| RANDOMIZE 语句 (gwsts.c) | ✅ |
| RESEQ/DELETE range API (gwlist.c) | ✅ |
| RENUM 语句分支 (gwsts.c) | ✅ |
| DELETE 语句分支 (gwsts.c) | ✅ |
| 执行指针 UAF 修复 (gwmain.c/gwlist.c) | ✅ |
| nodes[256] 动态化 + 链版本计数器 | ✅ |
| RESEQ 泄漏关闭 + gwram 计数器回归防护 | ✅ |

### 新基础设施 (可复用)

- **gwram 分配/释放计数器** (gwram.c): gwram_get_alloc_count/gwram_get_free_count — 支持未来所有模块的泄漏回归测试 (reviewer 建议的分配平衡断言落地)

---

## 下一步 (M2 规划, 按 v129 建议)

| 优先级 | 任务 |
|--------|------|
| 1 | 65 个 .bas 补充 expected 基线 (当前 rc-only 校验, 输出未断言) — 最大收益项 |
| 2 | ON ERROR/GOTO 语句分支 (2 个预期跳过项 → 实现后转 PASS) |
| 3 | CI 门禁: 孤儿检测 + 零警告 + fresh clone 构建 + .bas 自动回归 + 计数器平衡断言 |
| 4 | AddressSanitizer 构建 (泄漏/UAF 双保险) |

## 更新时间
2026-10-06

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。