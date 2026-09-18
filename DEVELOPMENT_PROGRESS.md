# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / **GWINIT ✅** / GWMAIN 待) |
| **当前阶段** | GWINIT 模块 TDD 7 步完成，待提交等待 review |
| **依赖顺序** | GWDATA → GWRAM → GWINIT ✅ → GWMAIN (M0 最后一个) |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: GWINIT 模块 (TDD 7 步完成)

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 GWINIT.ASM (456 行) | ✅ INIT 主流程/INITFG/CMDERR/栈计算/KBUF 结构分析 |
| 2 | 扩展 test_gwinit.c 单测 (RED) | ✅ 3→9 测试: initfg/idempotent/kbuf_colon/stack_calc/cmderr/reinit_no_crash — RED 确认 (链接失败) |
| 3 | .bas 集成测试 | ✅ 判定无需独立 .bas — GWINIT 无独立运行时行为，由 gwmain 启动流程 + 全部 .bas 回归覆盖 |
| 4 | 实现 src/gwinit.c (GREEN) | ✅ g_initfg/g_kbuf (KBUF_SIZE 256)/g_stack_size (free/8 min 512)/gwinit_cmderr |
| 5 | mingw32-make test 至绿 | ✅ **3132/3132 PASS, 0 警告** |
| 6 | 完整 exe + 集成回归 | ✅ test_gwbasic 3141/3141; gwram 4/4, gwdata 8/8, on_error ERR=11, cond/def_fn 全绿 |
| 7 | 提交 + 等待 review | ⏳ 待提交 |

### ASM 依据 (GWINIT.ASM)

| ASM 位置 | 结构 | C 实现 |
|----------|------|--------|
| L426-427 | `KEYSOF: MOV AL,LOW 377O / MOV BYTE PTR INITFG,AL` — INITFG=0xFF 初始化完成 | `g_initfg = -1` (0xFF), gwinit_get_initfg() |
| L133-134 | `MOV BX,OFFSET KBUF-1 / MOV BYTE PTR [BX],LOW ":"` — 冒号初始化 direct input restart | `g_kbuf[0] = ':'`, gwinit_get_kbuf() |
| L359-364 | 栈大小 = total_free/8, min 512 (`CMP AL,LOW 2 / JB SMLSTK / MOV BX,512D`) | g_stack_size = max(STRING_HEAP_SIZE/8, 512) |
| L432-449 | CMDERR: 标题+错误消息+`JMP SYSTME` 退出 OS | gwinit_cmderr() 返回 ERROR (-1), main 处理输出 |
| L105-108 | PROFLG/MSWFLG/CSWFLG 清零 | itsa86_init 已覆盖 ✅ |
| L175-270 | 命令行开关 /M: /F: /S: /C: | itsa86_mapclc 已覆盖 (/M ✅ /C flag ✅, /F /S 忽略值=ASM 行为) ✅ |
| L281 | MAPCLC 内存映射 | itsa86_mapclc 1:1 ✅ |

### 变更文件

| 文件 | 变更 |
|------|------|
| `src/gwbasic.h` | KBUF_SIZE 256 宏；gwinit_get_initfg/get_kbuf/get_stack_size/cmderr 声明 |
| `src/gwinit.c` | INITFG/KBUF 冒号/栈计算/CMDERR 实现 (4 个新函数) |
| `tests/test_gwinit.c` | 3→9 测试 (TDD RED→GREEN) |
| `tests/test_runner.c` | 注册 6 个新 gwinit 测试 (719 声明 = 719 注册) |

### 验证结果

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **3132/3132 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **3141/3141 PASS** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 声明=注册=719 | **0** ✅ |
| 集成回归 | gwram/gwdata/on_error/cond/def_fn | 全绿 ✅ |

---

## 上一轮: 孤儿清理 (commit 0b96e49, 已完成)

4 个 gwram 孤儿测试注册 (713=713)，单测 3124/3124。

## 历史: CODE_REVIEW v110 修复 (commit bb2bba3 ✅)

fiveo_chain mode 0 → FM_INPUT (HIGH 级)。v109 (dda68de ✅): 零警告 + ERRTAB 1:1。
GWRAM (195c4c6 ✅) / GWDATA (ceabce9 ✅)。

---

## 下一步

1. 提交 GWINIT → 等待 review
2. 之后: **GWMAIN** 模块 (M0 最后一个) — 读 GWMAIN.ASM → test → 实现 → 提交
3. M0 完成后进入 M1 (GWSTS/GWEVAL/BIMISC 深化)

## 更新时间
2026-09-19

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。