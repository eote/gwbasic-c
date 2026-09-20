# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT ✅ / GWMAIN 待) |
| **当前阶段** | CODE_REVIEW v111 修复轮 (MEDIUM-1/2 重复注册清理) 完成，待提交 |
| **依赖顺序** | GWDATA → GWRAM → GWINIT → (review 修复) → GWMAIN (M0 最后一个) |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v111 修复记录

### 问题清单与处理

| # | 级别 | 描述 | 处理 | 状态 |
|---|------|------|------|------|
| M-1 | 🟨 MEDIUM | test_runner.c 重复注册 — 27 个测试被注册 2 次 (fiveo/ibmres 块行 1227+ 与 1714-1731 重叠，run_test 746 行 vs 719 唯一) | execute_code+Python 脚本: 按名字计数，保留首次注册，删除后续 27 个重复 (含多行 span 共 36 行)；验证 run_test 746→719, 重复 27→0 | ✅ |
| M-2 | 🟨 MEDIUM | 注册数 > 定义数 (747 vs 727, 差额来自重复) | 同上修复；验证 声明 719 = 注册 719 = 孤儿 0 | ✅ |
| L-1 | 🟩 LOW | 测试计数 3132 vs v110 2119 (+1013) | 确认: GWDATA/GWRAM/GWINIT 三模块新增断言 (~1000) + 27 重复注册 (~13 断言)；重复清除后 **2821** (3132-311 重复断言) | ✅ 已确认 |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2821/2821 PASS** (重复清除后) |
| 完整测试 | `./tests/test_gwbasic.exe` | **2821/2821 PASS** |
| 编译警告/错误 | 干净重建 | **0 条** |
| 重复注册 | uniq -d | **0** (was 27) ✅ |
| 孤儿 | 声明=注册=719 | **0** ✅ |
| 集成回归 | gwram 4/4, gwdata 8/8, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### 变更文件

| 文件 | 变更 |
|------|------|
| `tests/test_runner.c` | 删除 27 个重复注册 (36 行)，746→719 行 run_test |
| `DEVELOPMENT_PROGRESS.md` | v111 修复记录 |

---

## 上一轮: M0 GWINIT 模块 (commit 413b307, 已完成)

TDD 7 步: INITFG/KBUF 冒号/栈计算/CMDERR 实现 + 9 测试 (3→9)。

## 历史完成

- CODE_REVIEW v110 (bb2bba3 ✅): fiveo_chain mode 0 → FM_INPUT
- CODE_REVIEW v109 (dda68de ✅): 零警告 + ERRTAB 1:1
- 孤儿清理 (0b96e49 ✅): 4 gwram orphans
- GWRAM (195c4c6 ✅) / GWDATA (ceabce9 ✅)

---

## 下一步

1. 提交 v111 修复 → 等待 review
2. 之后: **GWMAIN** 模块 (M0 最后一个) — 读 GWMAIN.ASM → test → 实现 → 提交
3. M0 完成后进入 M1 (GWSTS/GWEVAL/BIMISC 深化)

## 更新时间
2026-09-20

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。