# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中 2/N** (GWSTS 深化 ✅ / GWLIST 行管理 ✅ / RENUM 语句分支 ✅) |
| **当前阶段** | M1 第 3 批 (v118 MEDIUM-1 RENUM 语句分支) TDD 7 步完成，待提交等待 review |
| **依赖顺序** | M0 ✅ → M1 (RANDOMIZE ✅ → RESEQ/DELETE ✅ → RENUM 语句 ✅ → .bas 覆盖) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M1 第 3 批 — RENUM 语句分支 (v118 MEDIUM-1, TDD 7 步完成)

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 ASM (GWMAIN.ASM RESEQ L3102-3222) | ✅ (上轮已完成三参数语义分析) |
| 2 | 编写测试 (RED) | ✅ test_gwsts_renum.c 新增 7 测试 (default/start/start_mm/full_args/inc_zero/content_kept/randomize_regress) — RED 确认 (带参数 REENUM got -1) |
| 3 | .bas 集成验证 | ✅ RENUM 100 → rc=0 (此前 Error at line 30) |
| 4 | 实现 (GREEN) | ✅ gwsts.c RENUM 分支 (解析 NN[,MM[,INC]] → gwlist_reseq)；gwlist_reseq INC 语义修正 (-1=absent, 0=explicit FCERR) |
| 5 | mingw32-make test 至绿 | ✅ **2989/2989 PASS, 0 警告** |
| 6 | 完整 exe + 集成回归 | ✅ test_gwbasic 2989/2989 rc=0; gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 |
| 7 | 提交 + 等待 review | ⏳ 待提交 |

### ASM 依据

| ASM 位置 | 结构 | C 实现 |
|----------|------|--------|
| GWMAIN L3102-3111 | `RESEQ [NN[,MM[,INC]]]` — 默认 CX=10 | gwsts RENUM: 无参数 → first=0/mm=0/inc=-1 |
| GWMAIN L3113-3120 | LINSPC 新 NN | first = gweval_eval_numeric |
| GWMAIN L3140-3142 | `OR DX,DX / JZ / JMP FCERR` — INC=0 非法 | gwlist_reseq: inc==0 → ERROR (explicit)；inc<0 → default 10 (absent) |
| GWMAIN L3107 | LINES LESS THAN MM NOT RESEQUENCED | mm 阈值 |

### 本轮修复

| 问题 | 根因 | 修复 |
|------|------|------|
| RENUM 100,20 got -1 | gwlist_reseq API 无法区分 "INC 缺省" (0=默认 10) 与 "INC 显式 0" (FCERR)；parser 缺省传 0 触发错误检查 | INC 语义: **-1=absent (default 10), 0=explicit FCERR**；parser 缺省传 -1，尾部逗号传 -1 |
| gwlist_m1 测试 (0,0,0) 语义 | 同上 — 原用 0 表示缺省 | 更新 3 处调用为 (0,0,-1)；inc_zero 测试拆分 (absent→default, explicit→FCERR) |

### 变更文件

| 文件 | 变更 |
|------|------|
| `src/gwsts.c` | RENUM 语句分支 (解析 NN[,MM[,INC]]，缺省 inc=-1) |
| `src/gwlist.c` | gwlist_reseq INC 语义修正 (-1=absent/0=FCERR/>0=use) |
| `tests/test_gwsts_renum.c` | 新增 7 测试 |
| `tests/test_gwlist_m1.c` | 3 处调用更新 + inc_zero 测试语义修正 |
| `tests/test_runner.c` | 注册 7 个测试 (754 声明 = 754 注册) |
| `tests/Makefile` | 加入 test_gwsts_renum.c |

---

## 验证结果 (本轮)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2989/2989 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2989/2989 PASS, rc=0** |
| 编译警告 | 干净重建 (生产+测试) | **0 条** |
| 孤儿 | 声明=注册=754 | **0** ✅ |
| 集成回归 | gwdata/gwram/on_error/cond/def_fn + RENUM/RANDOMIZE .bas | 全绿 ✅ |

---

## M1 剩余 (下轮)

| 优先级 | 任务 |
|--------|------|
| 1 | .bas 集成测试补充 (RESEQ/DELETE/RANDOMIZE/RENUM 实际运行) |
| 2 | nodes[256] 上限确认 (ASM 64K 程序 — 动态扩容或提升上限) |
| 3 | DELETE 语句分支 (gwsts.c 接入 gwlist_delete_range — 同 RENUM 模式) |

## 更新时间
2026-09-25

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。