# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 **全部完成** ✅ (GWDATA/GWRAM/GWINIT/GWMAIN) |
| **当前阶段** | CODE_REVIEW v115 验证确认 (0 CRITICAL/HIGH/MEDIUM/LOW, M0 4/4) — 无待办修复；下一里程碑 M1 |
| **依赖顺序** | M0 ✅ → M1 (GWSTS 语句执行深化 / GWLIST 行管理) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v115 验证记录

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2840/2840 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2840/2840 PASS** |
| 编译警告 | 干净重建 (生产+测试) | **0 条** |
| 孤儿 | 声明=注册=728, diff 空 | **0** ✅ |
| 重复注册 | uniq -d | **0** ✅ |
| 集成回归 | gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 | ✅ |

### 问题清单
**0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW** — M0 全部完成确认 (GWDATA 9 + GWRAM 8 + GWINIT 9 + GWMAIN 9 = 35 测试)。

### M0 里程碑状态: 4/4 完成 ✅

| 模块 | 提交 | 测试 | ASM 依据 |
|------|------|------|----------|
| GWDATA | ceabce9 | 9 | GWDATA.ASM ERRTAB/变量表/字符串堆 |
| GWRAM | 195c4c6 | 8 | GWRAM.ASM 内存管理 (含 CRITICAL 修复) |
| GWINIT | 413b307 | 9 | GWINIT.ASM INITFG/KBUF/栈计算/CMDERR |
| GWMAIN | 2d55e62 | 9 | GWMAIN.ASM MAIN/CURLIN/FNDLIN/NEWSTT |

---

## 下一步 (M1 里程碑)

| 优先级 | 任务 | 说明 |
|--------|------|------|
| 1 | **M1: GWSTS 语句执行深化** | 已有 gwsts_exec_statement 基础 (FOR/NEXT/GOTO/GOSUB/IF/PRINT/INPUT/DIM/CLEAR)；扩展: WHILE/WEND, SELECT, ON..GOSUB, RANDOMIZE |
| 2 | **M1: GWLIST 行管理深化** | 已有 add/delete/list/find/load_file；扩展: RENUM/RESEQ 行号重排, DELETE range |
| 3 | .bas 集成测试补充 | v115 建议: 提升 65/67 → 覆盖更多语句 |

## 更新时间
2026-09-23

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。