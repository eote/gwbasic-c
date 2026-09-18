# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT 进行中 / GWMAIN 待) |
| **当前阶段** | GWINIT 模块 TDD (步骤 2-4 进行中) + 孤儿清理 |
| **依赖顺序** | GWDATA → GWRAM → GWINIT (当前) → GWMAIN |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: GWINIT 模块 (进行中)

### ASM 分析结果 (GWINIT.ASM, 456 行)

| ASM 结构 | 位置 | C 对应状态 |
|----------|------|-----------|
| INIT 主入口 (段寄存器/CPMEXT/栈初始化) | L65-135 | gwinit_init() 简化版 (PI 常量 + math init) — DOS 段寄存器操作不适用于 C |
| PROFLG/MSWFLG/CSWFLG 清零 | L105-108 | itsa86_init 已有 (g_mswflg/g_cswflg) ✅ |
| SCNIPL/GWINI/SNDRST/GIOINI 调用序列 | L125-130 | gwmain.c init_all_modules 已覆盖 (scnedt/oemsnd/gio init) ✅ |
| TOPMEM/MEMSIZ 初始化 | L131-132 | itsa86_mapclc 已有 ✅ |
| KBUF-1 冒号初始化 (direct input restart) | L133-134 | ⏳ 待实现 (gwsts/scnedt 相关) |
| STKINI 临时栈 | L135 | ⏳ 待实现 |
| 命令行开关解析 /M: /F: /S: /C: | L175-270 | itsa86_mapclc 部分覆盖 (/M: ✅, /C: flag ✅, /F: /S: 忽略值=ASM 行为) ✅ |
| MAPCLC 内存映射计算 | L281 | itsa86_mapclc 1:1 ✅ |
| TXTTAB/STKLOW/VARTAB/MEMSIZ/FRETOP/STREND 内存布局 | L318-388 | itsa86_mapclc + gwdata/gwram 已覆盖 ✅ |
| 栈大小计算 (free/8, min 512) | L359-364 | ⏳ 待实现 (itsa86_mapclc 后段) |
| FREFLG 清零 + GETHED 标题打印 | L392-415 | gwmain.c 启动横幅已覆盖 ✅ |
| KEYSW 功能键开关 | L398-425 | ⏳ OEM 部分，oem.c 已有 stub |
| INITFG 初始化完成标志 | L426-427 | ⏳ 待实现 (g_state 或独立标志) |
| CMDERR 初始化期错误处理 | L432-449 | ⏳ 待实现 |

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 GWINIT.ASM | ✅ 完成 (456 行全读，结构表如上) |
| 2 | 扩展 test_gwinit.c 单测 (TDD RED) | 🔄 进行中 — 现有 3 测试 (basic/constants/function_table)；待加: INITFG/开关解析/栈计算/CMDERF |
| 3 | .bas 集成测试 | ⏳ 待定 (GWINIT 无独立运行时行为，由 gwmain 启动流程覆盖) |
| 4 | 实现 src/gwinit.c (GREEN) | ⏳ 待扩展: INITFG/CMDERR/KBUF 冒号/栈计算 |
| 5-7 | 测试至绿/交互验证/提交 | ⏳ |

### 本轮已完成 (GWINIT 开始前的孤儿清理)

| 项 | 状态 |
|----|------|
| 4 个 gwram 孤儿测试注册 (stress/variable_storage/zero_size/realloc_zero) | ✅ test_runner.c 713 声明 = 713 注册 |
| 单测 | **3124/3124 PASS** (2119→3124, +1005? 无 — 3124 = 2119 + 4 gwram + 1001 gwdata/gwram 新断言) |
| 完整 exe | **3133/3133 PASS** |
| 警告 | 0 |
| 集成回归 | gwram 4/4, gwdata 8/8, on_error ERR=11, cond 全绿 ✅ |

---

## 上一轮: CODE_REVIEW v110 修复 (commit bb2bba3, 已完成)

| # | 级别 | 描述 | 状态 |
|---|------|------|------|
| L-1 | 🟩 | 测试计数确认 (断言合并，非丢失) | ✅ |
| 超范围 | 🔴 HIGH | fiveo_chain mode 0 → FM_INPUT (ASM PRGFLI MD_SQI=1) | ✅ |

## M0 GWRAM (commit 195c4c6 ✅) / GWDATA (commit ceabce9 ✅)

7 步 TDD 全流程完成。GWRAM 含 CRITICAL 修复 (字符串堆无效 free)。

---

## 验证基线 (当前)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **3124/3124 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **3133/3133 PASS** |
| 编译警告 | 干净重建 | **0 条** |
| 孤儿 | 声明=注册=713 | **0** ✅ |

---

## 下一步

1. 完成 GWINIT TDD: 扩展 test_gwinit.c (INITFG/CMDERR/栈计算/KBUF) → 实现 gwinit.c → mingw32-make test 至绿
2. 提交 GWINIT → 等待 review
3. 之后: GWMAIN 模块 (M0 最后一个)

## 更新时间
2026-09-19

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。