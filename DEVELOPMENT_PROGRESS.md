# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT 待 / GWMAIN 待) |
| **当前阶段** | CODE_REVIEW v109 修复轮 (MEDIUM-1/2 + LOW-1 + 超范围 1:1 修正) |
| **依赖顺序** | GWDATA → GWRAM → (review 修复) → GWINIT → GWMAIN |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v109 修复记录

### 问题清单与处理

| # | 级别 | 描述 | 处理 | 状态 |
|---|------|------|------|------|
| M-1 | 🟨 MEDIUM | 测试编译警告 ~5 条 (实际干净重建 52 条) | 修复全部: call86/next86/gweval_helpers 指针类型 const 化、test_framework.h %d→%lld、unused 变量 (void) 化、itsa86 type-limits 断言重写 | ✅ 52→0 |
| M-2 | 🟨 MEDIUM | gwdata.c 257/284 警告 (统计误差) | 调查发现真实问题: total_size unused + ERRTAB [12]/[14] 重复定义 — 均已修复 | ✅ |
| L-1 | 🟩 LOW | 测试计数 2120 vs 2121 (-1) | 确认为 v109 轮 gwdata ERRTAB 重排后 gweval_synchr 断言语义变化所致，非丢失 (孤儿 0: 声明=注册=709 一致) | ✅ 已确认 |
| 超范围 | 🔧 1:1 | **ERRTAB 错误码表顺序与 ASM 不一致** — 调查 M-2 时发现: 当前 [1]="Syntax error" 但 ASM DCL 宏顺序 [1]="NEXT without FOR"、[11]="Division by zero" (当前 [6])；ERRSN/ERRDV0 宏值错误 (当前 1/6，ASM 实际 2/11) | 按 GWDATA.ASM DCL 宏 1:1 重排全部 30 条非磁盘错误 + 补全 25 条磁盘错误 (50-74)，gwbasic.h 补全 ERRNF..ERRSO 宏定义 | ✅ |

### ASM 依据 (GWDATA.ASM ERRTAB)
- DCL 宏: `QQ=QQ+1` + DCUL 文本 + `DB 0` — 1-indexed
- 非磁盘错误 1..30: NF(1) SN(2) RG(3) OD(4) FC(5) OV(6) OM(7) UL(8) BS(9) DD(10) DV0(11) ID(12) TM(13) SO(14) LS(15) ST(16) CN(17) UF(18) NR(19) RE(20) UE(21) MO(22) BO(23) DT(24) DF(25) FN(26) OP(27) ??(28) WH(29) WE(30)
- `QQ=49D` — 磁盘错误从 50 开始: FO(50) IE(51) BFN(52) FNF(53) BFM(54) FAO(55) ??(56) DIO(57) .. Renamed(74)
- 槽 31..49 原表未用

### 验证链路 (修复后)
- 除零 → gwdata_set_error(11, ...) → ERR 变量=11 → `ERR= 11` 输出 ✅ (与 ASM DV0ERR 一致)
- SYNCHR 失配 → error 2 (Syntax Error) ✅ (与 ASM SNERR→ERRSN=2 一致)
- `mingw32-make clean && mingw32-make test` → **2119/2119 PASS, 0 警告 (生产+测试)** ✅
- 集成: test_gwram 6/6, test_on_error ERR=11, test_gwdata 全 PASS, test_cond/test_def_fn 全绿 ✅
- 孤儿: 声明 709 = 注册 709 = 0 孤儿 ✅

---

## 上一轮: M0 GWRAM 模块 (已完成, commit 195c4c6)

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 GWRAM.ASM | ✅ |
| 2 | test_gwram.c 单测 | ✅ |
| 3 | test_gwram.bas 集成 | ✅ |
| 4 | 实现 gwram.c + bistrs_fre + gweval FRE + gwsts CLEAR | ✅ |
| 5 | mingw32-make test 全绿 | ✅ |
| 6 | 交互验证 | ✅ |
| 7 | 提交 ceabce9/195c4c6 + 等待 review | ✅ |

含 CRITICAL 修复: CLEAR/NEW/RUN/GET 中 4 处 gwram_free(字符串堆指针) 无效 free 移除。

---

## 变更文件清单 (本轮 v109 修复)

| 文件 | 变更 |
|------|------|
| `src/gwbasic.h` | ERRTAB 错误码宏 1:1 修正 + 补全 ERRNF..ERRSO；bimisc_get_trace_flag 声明 |
| `src/gwdata.c` | ERRTAB 表 1:1 重排 (30 非磁盘 + 25 磁盘)；total_size (void)；[12]/[14] 重复修复 |
| `tests/test_framework.h` | TEST_ASSERT_EQ_INT %d→%lld (size_t 兼容) |
| `tests/test_call86.c` | char* → const char* (8 处) |
| `tests/test_next86.c` | char* → const char* (18 处) |
| `tests/test_gweval_helpers.c` | char* → const char*；duplicate const 修复；error_before (void) |
| `tests/test_gengrp.c` | unused attr/a1/a2/a3 (void) 化 (text_mode 断言保留) |
| `tests/test_gwsts.c` | unused rc (void) 化 (2 处) |
| `tests/test_itsa86.c` | type-limits 断言重写 (3 处) |
| `tests/test_oem.c` | unused bit (void) 化 |

---

## 下一步 (按 PLAN_FRESH.md 依赖顺序)

| 模块 | 依赖 | 说明 |
|------|------|------|
| **GWINIT** | GWDATA ✅, GWRAM ✅ | 读 GWINIT.ASM → test_gwinit 扩展 → src/gwinit.c → 提交等待 review |
| **GWMAIN** | GWINIT, GWSTS | 后续 |

## 更新时间
2026-09-18

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。