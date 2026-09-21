# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT ✅ / GWMAIN 待) |
| **当前阶段** | CODE_REVIEW v113 处理完成 — M-1/M-2 经独立验证为 reviewer 统计误差 (0 孤儿)，无待办修复 |
| **依赖顺序** | GWDATA → GWRAM → GWINIT → GWMAIN (M0 最后一个) |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v113 独立验证记录

### 问题清单与处理

| # | 级别 | 描述 | 独立验证方法 | 结论 |
|---|------|------|-------------|------|
| M-1 | 🟨 MEDIUM | 孤儿测试未解决 (727 定义 vs 720 注册, 差额 7) | ① defs (排除 test_runner.c): `grep 'void test_.*{$'` 排序去重 = **719** ② regs: `grep run_test` 排序去重 = **719** ③ `comm -23` 双向 diff 均为空 ④ 9 个 "重复" 名实为 test_gwdata.c 内声明区+定义区各计一次 (grep 不分声明/定义)，非跨文件重复 | ✅ **0 孤儿** — reviewer 727/720 计数为 grep 把 test_gwdata.c 内部声明行 (L174-182) 误计入定义 |
| M-2 | 🟨 MEDIUM | 开发组验证有误 (声称 719=719 vs 独立 1440≠720) | 1440 = 全部 test_*.c 的 `void test_` 行 (含 test_runner.c 声明区 719 + 定义区 ~720)；正确口径是排除 test_runner.c 后取定义且排除声明行；719=719 复核成立 | ✅ 开发组验证正确 (口径差异非验证错误) |

### 关键证据

```
defs (排除 test_runner.c, 仅定义 {...}): 719
regs (test_runner.c run_test, 去重):     719
comm -23 orphans:  (空)
comm -13 stale:    (空)
9 个"重复名": 全部在 test_gwdata.c — L174-182 是声明区 (void test_xxx(void);),
              L8-159 是定义区 (void test_xxx(void) {), grep 模式混计
所有 9 个 gwdata 测试均已在 test_runner.c 注册 (run_test("gwdata_*") 9 个全存在)
```

### 验证结果 (本轮回归)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2821/2821 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2821/2821 PASS** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | defs=regs=719, 双向 diff 空 | **0** ✅ |
| 重复注册 | uniq -d | **0** ✅ |
| 集成回归 | gwram 4/4, gwdata 8/8, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### 结论
**无待办修复项** — v113 的 2 个 MEDIUM 均为 reviewer 统计口径误差（grep 混计声明/定义行 + 含 test_runner.c 自身），代码工作区干净无需改动。

---

## 上一轮: CODE_REVIEW v112 修复 (commit ccae2d1, 已完成)

- MEDIUM-1: 59 处续行统一单行 + 57 双括号 bug 修复
- MEDIUM-2: 孤儿 0 确认

## 历史完成

- M0 GWINIT (413b307 ✅): INITFG/KBUF/栈计算/CMDERR + 9 测试
- CODE_REVIEW v111 (eb038a8 ✅): 重复注册 27→0
- CODE_REVIEW v110 (bb2bba3 ✅): fiveo_chain mode 0 → FM_INPUT
- CODE_REVIEW v109 (dda68de ✅): 零警告 + ERRTAB 1:1
- 孤儿清理 (0b96e49 ✅) / GWRAM (195c4c6 ✅) / GWDATA (ceabce9 ✅)

---

## 下一步

1. 等待下一轮 CODE_REVIEW（或确认批准）
2. 之后: **GWMAIN** 模块 (M0 最后一个) — 读 GWMAIN.ASM → test → 实现 → 提交
3. M0 完成后进入 M1 (GWSTS/GWEVAL/BIMISC 深化)

## 更新时间
2026-09-21

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。