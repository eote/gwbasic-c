# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 |
| **当前模块** | GWRAM (核心层第 2 个) |
| **依赖顺序** | GWDATA ✅ → GWRAM (进行中) → GWINIT → GWMAIN |
| **TDD 工作流步骤** | 7 步标准化流程 |

---

## PLAN_FRESH.md 里程碑对照

| 里程碑 | 名称 | 包含模块 | 状态 |
|--------|------|----------|------|
| **M0** | 基础设施 | GWDATA, GWRAM, GWINIT, GWMAIN | GWDATA ✅ / GWRAM 进行中 |
| **M1** | 核心引擎 | GWSTS, GWEVAL, BIMISC | 待开始 |
| **M2** | 程序与字符串 | GWLIST, GENGRP, BISTRS | 待开始 |
| **M3** | I/O 完整 | 10 个 I/O 模块 | 待开始 |
| **M4** | 高级语言 | 数学/数组/PRINT USING/文件 | 待开始 |
| **M5** | OEM 与平台 | 7 个 OEM 模块 | 待开始 |
| **M6** | 语言扩展 | DEF FN / ON ERROR/RESUME | 待开始 |
| **M7** | 质量收敛 | 全测试/警告/文档 | 待开始 |

---

## GWRAM 模块 - 7 步 TDD 工作流执行记录

| 步骤 | PLAN_FRESH.md 描述 | 执行状态 | 产出物 |
|------|-------------------|----------|--------|
| 1 | 读对应 .ASM 理解行为 | ✅ 完成 | 已读 GWRAM.ASM (466 行: R/RINIT 宏、DSEG 变量布局) |
| 2 | 根据 ASM 编写单测 - 先红 | ✅ 完成 | tests/test_gwram.c (8 个测试函数) |
| 3 | 根据 ASM 行为编写 .bas 集成测 | ✅ 完成 | tests/bas/test_gwram.bas (6 场景) |
| 4 | 实现 src/gwram.c - 至绿 | ✅ 完成 | gwram_alloc/realloc/calloc/free + FRE 函数 |
| 5 | mingw32-make test - 全绿 | ✅ 完成 | 2120/2120 PASS |
| 6 | 交互验证 ./gwbasic.exe | ✅ 完成 | test_gwram.bas 全场景通过 |
| 7 | 提交 + CODE_REVIEW 等待批准 | ⏳ 待提交 | - |

## 本轮发现并修复的 CRITICAL bug

| 问题 | 根因 | 修复 |
|------|------|------|
| **CLEAR 后崩溃 (rc=127)** | 字符串指针指向静态字符串堆 `str_heap.data`，但 CLEAR/NEW/RUN 调用 `gwram_free(s.ptr)` → 无效 free | 移除 3 处 `gwram_free(v->value.s.ptr)` (bimisc.c CLEAR/NEW/RUN + advgrp.c GET)，堆通过 `str_heap.used=0` 整体重收 |
| **FRE() 未实现** | BISTRS.ASM FRE 函数 (L1097: FREFAC + GARBA2 + STREND-FRETOP) 无 C 对应 | 新增 `bistrs_fre()` (返回 str_heap 空闲字节)，gweval.c 注册 FRE() 函数调用 |
| **gwsts.c CLEAR 语句缺失** | gwsts_exec_statement 无 CLEAR 分支，语句报错 | 新增 CLEAR 分支 (解析 [,max_mem][,stack_size] → bimisc_clear_with_args) |

---

## GWDATA 模块记录 (上轮，已完成)

| 步骤 | 状态 |
|------|------|
| 1-7 TDD 全流程 | ✅ 完成 (commit ceabce9) |

---

## 验收结果汇总

| 测试类型 | 命令 | 结果 | 说明 |
|----------|------|------|------|
| 单元测试 | `mingw32-make test` | **2120/2120 PASS** | 零失败 |
| 集成测试 | `./gwbasic.exe tests/bas/test_gwram.bas` | **6/6 场景通过** | FRE/CLEAR/大数组全绿 |
| 核心回归 | test_cond/test_on_error/test_def_fn/test_gwdata | **全绿** | 无退化 |
| 编译警告 | `mingw32-make clean && mingw32-make` | **0 条** | 零警告保持 |
| CRITICAL bug | CLEAR 字符串崩溃 | **已修复** | 3 处无效 free 移除 |

---

## 变更文件清单 (本轮 GWRAM)

| 文件 | 变更类型 | 说明 |
|------|----------|------|
| `src/gwbasic.h` | 修改 | 新增 `bistrs_fre` 声明 (double 返回) |
| `src/bistrs.c` | 修改 | 新增 `bistrs_fre()` 实现 (ASM FRE 对应) |
| `src/gweval.c` | 修改 | 注册 FRE() 函数 (is_str_numeric + 调用分支) |
| `src/bimisc.c` | 修改 | 修复 CLEAR/NEW/RUN 无效 free (CRITICAL)；gwsts CLEAR 分支调用路径验证 |
| `src/advgrp.c` | 修改 | 修复 GET 无效 free |
| `src/gwsts.c` | 修改 | 新增 CLEAR 语句分支 |
| `tests/test_gwram.c` | 修改 | 8 个测试函数 (移除 main，供 test_runner 注册) |
| `tests/test_runner.c` | 修改 | 注册 gwram 新测试 + gwinit_constants 声明修复 |
| `tests/bas/test_gwram.bas` | 新增 | 6 个集成测试场景 |

---

## 下一步 (按 PLAN_FRESH.md 依赖顺序)

| 模块 | 依赖 | 预估步骤 |
|------|------|----------|
| **GWINIT** | GWDATA, GWRAM | 读 GWINIT.ASM → test_gwinit 扩展 → src/gwinit.c |
| **GWMAIN** | GWINIT, GWSTS | ... |

---

## 更新时间
2026-09-17

---

**原则**: 严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位实现位置；添加测试/实现前先查是否已存在。