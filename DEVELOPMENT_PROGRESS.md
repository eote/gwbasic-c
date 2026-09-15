# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 |
| **当前模块** | GWDATA (核心层第 1 个) |
| **依赖顺序** | GWDATA → GWRAM → GWINIT → GWMAIN → ... |
| **TDD 工作流步骤** | 7 步标准化流程 |

---

## PLAN_FRESH.md 里程碑对照

| 里程碑 | 名称 | 包含模块 | 状态 |
|--------|------|----------|------|
| **M0** | 基础设施 | GWDATA, GWRAM, GWINIT, GWMAIN | GWDATA ✅ 进行中 |
| **M1** | 核心引擎 | GWSTS, GWEVAL, BIMISC | 待开始 |
| **M2** | 程序与字符串 | GWLIST, GENGRP, BISTRS | 待开始 |
| **M3** | I/O 完整 | 10 个 I/O 模块 | 待开始 |
| **M4** | 高级语言 | 数学/数组/PRINT USING/文件 | 待开始 |
| **M5** | OEM 与平台 | 7 个 OEM 模块 | 待开始 |
| **M6** | 语言扩展 | DEF FN / ON ERROR/RESUME | 待开始 |
| **M7** | 质量收敛 | 全测试/警告/文档 | 待开始 |

---

## GWDATA 模块 - 7 步 TDD 工作流执行记录

| 步骤 | PLAN_FRESH.md 描述 | 执行状态 | 产出物 |
|------|-------------------|----------|--------|
| 1 | 读对应 .ASM 理解行为 | ✅ 完成 | 已读 GWDATA.ASM (1565 行) |
| 2 | **根据 ASM 代码逻辑编写单测** (tests/test_<module>.c) - 先红 | ✅ 完成 | tests/test_gwdata.c (2121 断言) |
| 3 | **根据 ASM 行为编写 .bas 集成测** (tests/bas/test_<feature>.bas) | ✅ 完成 | tests/bas/test_gwdata.bas (7 场景) |
| 4 | 实现 src/<module>.c - 至绿，严格复刻 ASM 逻辑 | ✅ 完成 | src/gwdata.c 核心功能 |
| 5 | 运行 mingw32-make test - 全绿 | ✅ 完成 | 2121/2121 PASS |
| 6 | 交互验证 ./gwbasic.exe 跑 .bas | ✅ 完成 | test_gwdata.bas 全绿 |
| 7 | 提交 + CODE_REVIEW 等待批准 | ⏳ 待提交 | - |

---

## 核心实现细节 (对应 GWDATA.ASM 关键数据结构)

| ASM 结构 | C 实现 | 验收测试 |
|----------|--------|----------|
| 变量表 (256 桶哈希) | `g_state->var_table[256]` | 2121 断言全绿 |
| 字符串堆 (16KB bump alloc) | `g_state->str_heap` | 字符串分配/GC 测试通过 |
| 程序缓冲区 (32KB) | `g_state->prog_buf` | 程序读写测试通过 |
| 错误消息表 (ERRTAB) | `gwdata_get_error_message()` | 错误码 1/6 等验证通过 |
| 变量类型 (INTEGER/SINGLE/DOUBLE/STRING) | `var_type_t` enum | 4 种类型赋值测试通过 |
| 数组支持 (多维) | `dim_count`, `dims[8]`, `array_data` | 1D/2D 数组测试通过 |
| 大小写不敏感 | `hash_name()` 大写 + `strcasecmp` | 大小写混合查找通过 |

---

## 验收结果汇总

| 测试类型 | 命令 | 结果 | 说明 |
|----------|------|------|------|
| 单元测试 | `mingw32-make test` | **2121/2121 PASS** | 零失败 |
| 集成测试 | `./gwbasic.exe tests/bas/test_gwdata.bas` | **7/7 PASS** | 全场景绿灯 |
| 核心回归 | test_cond/test_on_error/test_def_fn | **全绿** | 无退化 |
| 编译警告 | `mingw32-make clean && mingw32-make` | **0 条** | 零警告门槛达成 |
| 孤儿测试 | 声明 vs 注册一致性 | **0 个** | 709=709 |

---

## 变更文件清单 (本轮)

| 文件 | 变更类型 | 说明 |
|------|----------|------|
| `src/gwbasic.h` | 修改 | 补充声明、错误码、math.h、数组结构体字段 |
| `src/gwdata.c` | 修改 | 核心实现 (变量表、字符串堆、数组、错误消息、大小写不敏感) |
| `src/scnedt.c` | 修改 | 消除 #pragma 警告抑制 (CSRX/CSRY 改为 int) |
| `tests/test_gwdata.c` | 新增 | 2121 个单元测试断言 (9 个测试函数) |
| `tests/bas/test_gwdata.bas` | 新增 | 7 个集成测试场景 |
| `tests/test_runner.c` | 修改 | 注册 GWDATA 新测试 |

---

## 下一步 (按 PLAN_FRESH.md 依赖顺序)

| 模块 | 依赖 | 预估步骤 |
|------|------|----------|
| **GWRAM** | GWDATA | 读 GWRAM.ASM → test_gwram.c → test_gwram.bas → src/gwram.c |
| **GWINIT** | GWDATA, GWRAM | ... |
| **GWMAIN** | GWINIT, GWSTS | ... |

---

## 更新时间
2026-09-15

---

**原则**: 严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行，每步有产出物可验证，单模块一提交。