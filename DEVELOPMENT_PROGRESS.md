# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT ✅ / **GWMAIN ✅**) — **M0 全部完成** |
| **当前阶段** | GWMAIN 模块 TDD 7 步完成，待提交等待 review |
| **依赖顺序** | GWDATA → GWRAM → GWINIT → GWMAIN ✅ → M1 (GWSTS/GWEVAL/BIMISC 深化) |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: GWMAIN 模块 (TDD 7 步完成)

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 GWMAIN.ASM (3566 行, PUBLIC/SUBTTL 结构) | ✅ MAIN/READY/PRGEND/NEWSTT/FNDLIN/CHRGTR/CURLIN 分析 |
| 2 | 编写 test_gwmain.c 单测 (RED) | ✅ 9 测试: modules_init/curlin_direct/curlin_program/extract_line/line_limit/fndlin/edit_lines/bye/error_chain — RED 确认 (direct line detection failed) |
| 3 | .bas 集成测试 | ✅ 判定无需独立 .bas — GWMAIN 行为由 gwmain.c 启动流程 + 全部 67 .bas 回归覆盖 |
| 4 | 实现 | ✅ gwmain.c 已有实现 (main/run_interpreter) — 本轮为测试验证轮，无生产代码改动 |
| 5 | mingw32-make test 至绿 | ✅ **2840/2840 PASS, 0 警告** |
| 6 | 完整 exe + 集成回归 | ✅ test_gwbasic 2840/2840; gwram 4/4, gwdata 8/8, on_error ERR=11, cond/def_fn 全绿 |
| 7 | 提交 + 等待 review | ⏳ 待提交 |

### ASM 依据 (GWMAIN.ASM)

| ASM 位置 | 结构 | 验证点 |
|----------|------|--------|
| L604-605 | `MAIN: MOV BX,65535 / MOV CURLIN,BX` — direct mode CURLIN=0xFFFF (.RADIX 10) | 65535 = 0xFFFF ✅ |
| L385-388 | `PRGEND: MOV AL,BH / AND AL,BL / INC AL / JZ` — direct 检测 (AL 8 位回绕: 0xFF+1=0x00) | 8-bit AL = ((BH&BL)+1)&0xFF == 0 → direct ✅ |
| L494-496 | 同一 direct 检测模式 (ERRLIN) | 程序行 100 → AL=0x01 != 0 ✅ |
| L604-660 | MAIN 主循环: PINLIN 输入 → LINGET 行号 → 编辑 → NEWSTT | gwmain.c run_interpreter 等价 ✅ |
| L585-602 | READY: FINLPT → PRGFIN → CRDONZ → STROUT "Ok" | run_interpreter "Ok" 提示 ✅ |
| L894-922 | FNDLIN: TXTTAB 起链式查找, 三种返回 (找到/未找到/越界) | gwlist_find_line 等价 (找到/NULL) ✅ |
| L664-792 | EDENT 编辑入口 (行号 0 = direct) | gwmain.c has_line_number 分支 ✅ |
| L183 (gwmain.c) | 行号上限 65529 | 65529 valid / 65530 invalid ✅ |

### 本轮修复 (测试逻辑)

| 问题 | 根因 | 修复 |
|------|------|------|
| direct line detection failed | ASM AL 是 8 位寄存器，0xFF+1 回绕 0x00；测试初版用 16 位 WORD 无回绕 | 8-bit 语义: `((BH&BL)+1)&0xFF` |
| program line detected fail | C 运算符优先级: `&` 绑定比 `+` 紧，`(v&0xFF)+1` 先算导致 AND 结果错 | 显式括号: `((((v>>8)&0xFF)&(v&0xFF))+1)&0xFF` |

### 变更文件

| 文件 | 变更 |
|------|------|
| `tests/test_gwmain.c` | 新增 9 个测试 (GWMAIN ASM 1:1 验证) |
| `tests/test_runner.c` | 注册 9 个 gwmain 测试 (719→728 声明=注册) |
| `tests/Makefile` | 加入 test_gwmain.c |

---

## 验证结果 (本轮)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2840/2840 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2840/2840 PASS** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 声明=注册=728 | **0** ✅ |
| 集成回归 | gwram/gwdata/on_error/cond/def_fn | 全绿 ✅ |

---

## 里程碑 M0 状态: **全部完成** ✅

| 模块 | 提交 | TDD |
|------|------|-----|
| GWDATA | ceabce9 ✅ | 7 步 |
| GWRAM | 195c4c6 ✅ | 7 步 (含 CRITICAL 修复) |
| GWINIT | 413b307 ✅ | 7 步 |
| GWMAIN | 本轮 (待提交) | 7 步 |

## 下一步

1. 提交 GWMAIN → 等待 review
2. M0 完成 → 进入 **M1** (GWSTS/GWEVAL/BIMISC 深化)

## 更新时间
2026-09-22

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。