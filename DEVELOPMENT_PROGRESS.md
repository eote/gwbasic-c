# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 4/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / DELETE 语句+UAF 完整修复 ✅) |
| **当前阶段** | CODE_REVIEW v126 修复完成 (2 CRITICAL + 2 MEDIUM + 2 LOW 全修复)；等待 v127 review |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → DELETE ✅ → nodes[256]) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v126 修复 (2 CRITICAL + 2 MEDIUM + 2 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| CRITICAL-1 | 删除紧邻下一行 (20 DELETE 30) → 分支 1 用已 free 的 saved_next 快照, 被删行仍执行 (两条执行路径实证) | gwlist_is_alive() 指针身份 liveness 检查 (gwlist.c); 两条执行循环重定位: curr+saved_next 均 alive (RENUM) → 物理下一行 (ASM SAVTXT); curr alive + saved_next 删除 (DELETE) → number 扫描; curr freed → number 扫描 | probe (20 DELETE 30): LINE 30 不执行 ✅; test_delete_exec S5/S6/S7 ✅ |
| CRITICAL-2 | 提交缺失 tests/test_gwmain_uaf.c → HEAD 无法独立构建 | git add test_gwmain_uaf.c (3 测试: delete_line 返回 next / delete_range 返回 anchor / 删到末尾 NULL) | HEAD 独立构建恢复 ✅ |
| MEDIUM-1 | isdigit 拒绝尾随空白 (DELETE 40<space> → SNERR; ASM LINGET 空白是合法终止符) | 数字扫描 + 跳过尾随空白 + 内容再检查 (garbage → SNERR) | test_gwsts_delete_trailing_space ✅ |
| MEDIUM-2 | 分支 2 读已 free 节点字段 (saved_next->number UB) | gwlist_is_alive() 只比较指针不解引用 | 3057/3057 全绿 ✅ |
| LOW-1 | test_delete_exec.bas 未覆盖删除紧邻下一行 | +S5 (520 DELETE 510 后 520 不执行) / S6 (620-625 range) / S7 (720-740 range) 3 场景 + expected 更新 | 套件 68/70 输出校验 ✅ |
| LOW-2 | CODE_REVIEW.md 报告未入库 | v126 报告随本提交一并入库 | 文档同步 ✅ |

### 修复过程中发现并修复的新 bug

1. **RENUM 后跳回重排旧行**: find_line(旧行号) 判定 curr "被删" 错误 — RENUM 重编号当前行但它仍 alive。改用 gwlist_is_alive() 指针身份判定 (test_renum.bas 回归暴露)

### 修复后语义总表

| 场景 | 重定位 |
|------|--------|
| 语句删除紧邻下一行 | curr 存活 → saved_next 已删 → number 扫描 → 被删行不执行 |
| 语句删除当前行 | curr freed → number 扫描 (> line_num) |
| RENUM (所有行存活, 行号全变) | curr+saved_next alive → 物理下一行 (ASM SAVTXT) |
| DELETE 40<space> | 尾随空白 → 合法终止符 → 成功 |
| DELETE 40-60<space> | 同上 (range 形式) |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3057/3057 PASS** (+16: uaf 3 + trailing/next-line 4 + 原有) |
| 完整测试 | ./tests/test_gwbasic.exe | **3057/3057 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **68/70 PASS** (0 失败 + 2 预期跳过), test_delete_exec 输出校验 ✅ |
| test_delete_exec.bas | 7 场景 (含紧邻下一行 S5/S6/S7) | **rc=0 全绿** ✅ |
| test_renum.bas | RENUM 回归 (RENUM 后物理下一行) | rc=0 ✅ |
| CRITICAL-1 probe | 20 DELETE 30 | LINE 30 不执行 ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v127 review (CRITICAL-1/2 修复确认) |
| 2 | nodes[256] 上限确认 (ASM 64K 程序) → M1 完成 → M2+ |

## 更新时间
2026-10-03

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。