# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中** (GWSTS 语句执行深化: RANDOMIZE ✅ / ON..GOTO 验证 ✅ / WHILE/WEND 验证 ✅) |
| **当前阶段** | M1 第 1 批 TDD 7 步完成，待提交等待 review |
| **依赖顺序** | M0 ✅ → M1 (GWSTS/RANDOMIZE ✅ → GWLIST RENUM → .bas 覆盖) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M1 第 1 批 — GWSTS 语句执行深化 (TDD 7 步完成)

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 ASM (GWMAIN.ASM RANDOM L3383 / ONGOTO L2357 / WNDSCN L3419) | ✅ RANDOMIZE 语义 + ON..GOTO 索引 + WHILE/WEND 定位分析 |
| 2 | 编写测试 (RED) | ✅ test_math2.c 扩展 7→16 测试 (randomize x2/rnd x3/on_goto x2/on_gosub/while_wend) — RED 确认 (math2_randomize 未定义) |
| 3 | .bas 集成验证 | ✅ RANDOMIZE 42 → "seeded/rnd" rc=0 (此前 rc=127) |
| 4 | 实现 (GREEN) | ✅ math2_randomize() (srand 重置 + last_rnd 清零) + gwsts.c RANDOMIZE 语句分支 (带/无参数) + tests/Makefile 加入 test_math2.c |
| 5 | mingw32-make test 至绿 | ✅ **2949/2949 PASS, 0 警告** |
| 6 | 完整 exe + 集成回归 | ✅ test_gwbasic 2949/2949; gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn/math 全绿 |
| 7 | 提交 + 等待 review | ⏳ 待提交 |

### ASM 依据

| ASM 位置 | 结构 | C 实现 |
|----------|------|--------|
| GWMAIN L3383 | `RANDOM: JZ INPRG` — 无参数从终端询问 | gwsts.c RANDOMIZE 分支: fgets + RANMES 提示 "Random number seed (-32768 to 32767)" |
| GWMAIN L3408-3410 | `STRNDS: MOV RNDX+1,BX / CALL RNDMN2` — 存种子重置序列 | math2_randomize(): srand() + 丢弃首值 + last_rnd=0 |
| GWMAIN L2357-2412 | ONGOTO: 1-based 索引选行; `CMP AL,CL / JNAE / JMP SNERR` 越界错误 | 测试验证: index 1→line 100 ✅; index>count → error ✅ |
| GWMAIN L3419 | WNDSCN: WHILE 匹配 WEND 定位 | 测试验证: WHILE/WEND 行可定位 ✅ |
| IBMRES L81/L470 | RANDOMIZE 关键字 0x30 | 已注册 (tk 语句) ✅ |

### 本轮修复

| 问题 | 根因 | 修复 |
|------|------|------|
| RANDOMIZE 语句 rc=127 | gwsts.c 无 RANDOMIZE 分支 (仅关键字表注册)，语句落入错误处理 | 新增 RANDOMIZE 分支 (带参数→gweval 求值; 无参数→终端询问) |
| last_rnd undeclared | math2_randomize 插入位置在 static last_rnd 声明之前 | 声明提到文件顶部，删除原位置重复 |
| test_math2.c 未链接 | tests/Makefile TEST_SRCS 缺 test_math2.c | 加入 TEST_SRCS |
| 同种子序列断言 | math2_randomize 后 RND 继续新序列 | 丢弃首值 + last_rnd 清零，同种子→同序列确定性成立 |

### 变更文件

| 文件 | 变更 |
|------|------|
| `src/gwbasic.h` | math2_randomize 声明 |
| `src/math2.c` | math2_randomize 实现 (ASM STRNDS 1:1)；last_rnd 声明移顶部 |
| `src/gwsts.c` | RANDOMIZE 语句分支 (带/无参数) |
| `tests/test_math2.c` | 7→16 测试 (M1: RANDOMIZE/ON..GOTO/ON..GOSUB/WHILE) |
| `tests/test_runner.c` | 注册 9 个新测试 (737 声明 = 737 注册) |
| `tests/Makefile` | 加入 test_math2.c |

---

## 验证结果 (本轮)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2949/2949 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2949/2949 PASS** |
| 编译警告 | 干净重建 (生产+测试) | **0 条** |
| 孤儿 | 声明=注册=737 | **0** ✅ |
| 集成回归 | gwdata/gwram/on_error/cond/def_fn/math/RANDOMIZE | 全绿 ✅ |

---

## M1 剩余 (下轮)

| 优先级 | 任务 |
|--------|------|
| 1 | GWLIST 行管理深化: RENUM/RESEQ 行号重排, DELETE range |
| 2 | .bas 集成测试补充 (提升覆盖) |

## 更新时间
2026-09-23

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。