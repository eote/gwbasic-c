# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 4/N** (RANDOMIZE ✅ / RESEQ/DELETE ✅ / RENUM 语句 ✅ / DELETE 语句+UAF 修复 ✅) |
| **当前阶段** | CODE_REVIEW v125 修复完成 (1 CRITICAL + 2 MEDIUM + 3 LOW 全修复)；等待 v126 review |
| **依赖顺序** | M0 ✅ → M1 (RENUM ✅ → DELETE ✅ → nodes[256]) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v125 修复 (1 CRITICAL + 2 MEDIUM + 3 LOW)

### 修复记录

| # | 问题 | 修复 | 验证 |
|---|------|------|------|
| CRITICAL-1 | DELETE 删当前/后续行 → 执行指针 UAF (悬垂解引用, 被删行仍执行) | gwlist_delete_line 返回 next 存活节点; gwlist_delete_range 返回 range 后 anchor (ASM DEL/FNDLIN L791-793); gwmain.c 两处执行循环 (RUN L162+ / .bas L260+) 执行后按 node 身份重定位 (saved_next 存活→物理下一行, 被删→number 扫描), 不再解引用已释放节点 | test_delete_exec.bas: line 40 不执行 ✅, 自删/range 删除后安全继续 ✅ |
| MEDIUM-1 | DELETE 40- 开放区间 → start>end ERROR | ASM SCNLIN ALLLST L856: DX=65530 → "40-" 删到末尾合法 | test_gwsts_delete_open_range ✅ |
| MEDIUM-2 | DELETE ABC 非数字静默 atoi=0 误删行 0 | isdigit 全量校验 → SNERR (ASM LINSPC/LINGET MORLIN → SNERR L426) | test_gwsts_delete_non_numeric ✅ |
| LOW-1 | 裸 DELETE 未测试 | rc=ERROR (语法错误) + 测试 | test_gwsts_delete_bare ✅ |
| LOW-2 | 尾部多余静默接受 | "DELETE 10-20 30" → SNERR + 测试 | test_gwsts_delete_tail_garbage ✅ |
| LOW-3 | 测试盲区 | +6 边界单测 (bare/open_range/from_start/non_numeric/tail_garbage/line_zero) + test_delete_exec.bas | 3041/3041 ✅ |

### 修复过程中发现并修复的新 bug

1. **SNERR rc 覆盖**: dash 分支 `rc=ERROR` 后被无条件 `rc=(start>end)?...` 覆盖 (start/end 仍 0 → SUCCESS) — range check 移入 valid 分支内
2. **重定位语义初版错误**: 按 number 扫描在 RENUM 后跳回已重排旧行 (test_renum.bas 回归暴露) — 改为 node 身份优先 (saved_next 存活→物理下一行, ASM SAVTXT 语义), number 扫描仅当 saved_next 被删 (DELETE 不重排, number 序稳定)
3. **TEST_ASSERT NULL 解引用**: anchor=NULL 时 `TEST_ASSERT_EQ_INT(anchor->number,...)` segfault — 测试期望修正 ([10,20,30] 删 20-30 无 anchor)

### ASM 语义确认 (v125 建议-2)

| 形式 | ASM 位置 | 语义 |
|------|----------|------|
| DELETE 40 | SCNLIN SNGLIN | 单行 |
| DELETE 40-60 | SCNLIN | 区间 |
| DELETE 40- | SCNLIN ALLLST L856 (DX=65530) | 开放区间到末尾 |
| DELETE -40 | SCNLIN L842 (DX=0) | 从头到 40 |
| start>end | GWLIST FCERRG L805 | FCERR |
| 非数字 | LINSPC/LINGET MORLIN → SNERR L426 | 语法错误 |
| DELETE Ok | GWLIST L807 STROUT REDDY | 直接模式打印 Ok |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | mingw32-make test | **3041/3041 PASS** (+28: uaf 3 + 边界 6 + 原有 3013) |
| 完整测试 | ./tests/test_gwbasic.exe | **3041/3041 PASS, rc=0** |
| 编译警告 | 干净重建 | **0 条** |
| 套件集成 | bash run_bas_tests.sh | **68/70 PASS** (0 失败 + 2 预期跳过), test_delete/test_delete_exec 输出校验 ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

---

## 下一步 (M1 剩余)

| 优先级 | 任务 |
|--------|------|
| 1 | 等待 v126 review (CRITICAL-1 修复确认) |
| 2 | nodes[256] 上限确认 (ASM 64K 程序) → M1 完成 → M2+ |

## 更新时间
2026-10-02

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。