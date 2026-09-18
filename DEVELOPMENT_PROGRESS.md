# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 - 基础设施 (GWDATA ✅ / GWRAM ✅ / GWINIT 待 / GWMAIN 待) |
| **当前阶段** | CODE_REVIEW v110 修复轮 (LOW-1 测试计数确认 + 超范围 HIGH 级 CHAIN 修复) |
| **依赖顺序** | GWDATA → GWRAM → (review 修复) → GWINIT → GWMAIN |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: CODE_REVIEW v110 修复记录

### 问题清单与处理

| # | 级别 | 描述 | 处理 | 状态 |
|---|------|------|------|------|
| L-1 | 🟩 LOW | 测试计数 2119 vs v108 2121 (-2 断言) | 调查: gweval_synchr 失配断言 error_after==2 + pointer 检查合并所致，孤儿 0 (声明=注册=709)，非丢失 | ✅ 已确认 |
| 超范围 | 🔴 HIGH | **fiveo_chain() 永远失败 (rc=-1)** — 独立运行 test_gwbasic.exe 发现 5 个 CHAIN 测试 FAIL (mingw32-make test 只跑 2119 个注册测试未覆盖)；根因: giodsk_open(filename, filenum, **mode=0**, 0)，dskcom_open switch 只接受 FM_INPUT(1)..FM_BINARY(5)，mode 0 → default: return ERROR | 按 ASM DSKCOM.ASM PRGFLI (CHAIN → CHNENT → PRGFLI 用 MD_SQI=1 顺序输入模式打开) 修复: mode 0 → FM_INPUT；验证 chain rc=-1→0，test_gwbasic.exe **2128/2128 全 PASS** (5 个 CHAIN FAIL 清零) | ✅ |

### ASM 依据
- DSKCOM.ASM L290: `PRGFLI: MOV DH,LOW OFFSET MD_SQI` — 顺序输入模式
- MSDOSU L60: `MD_SQI=1D` — sequential input = 1 (对应 C FM_INPUT=1)
- FIVEO.ASM L636-639: CHAIN MERGE → OKGETM / 非 MERGE → CHNENT (跳转 LOAD 代码，文件保持打开)
- DSKCOM.ASM L332: `CHNENT: MOV AL,LOW 201O` — A=LEAVE FILES OPEN, RUN

### 关键发现: mingw32-make test 与 test_gwbasic.exe 计数差异
- `mingw32-make test` → 2119 (只跑注册的 run_test)
- `./tests/test_gwbasic.exe` → 2128 (含 main() 内额外直接调用断言)
- **5 个 CHAIN FAIL 只在 exe 直跑暴露** — make test 是不完整回归，后续应同时跑两者

---

## 上一轮: CODE_REVIEW v109 修复 (commit dda68de, 已完成)

| # | 级别 | 描述 | 状态 |
|---|------|------|------|
| M-1 | 🟨 | 测试编译警告 52→0 (const 指针化 + %lld + unused (void)) | ✅ |
| M-2 | 🟨 | gwdata.c total_size + ERRTAB [12]/[14] 重复 | ✅ |
| 超范围 | 🔧 | ERRTAB 1:1 ASM 重排 (30 非磁盘 + 25 磁盘 50-74) + ERRNF..ERRSO 宏 | ✅ |
| L-1 | 🟩 | 测试计数 2120 确认 | ✅ |

---

## M0 GWRAM 模块 (commit 195c4c6, 已完成)

7 步 TDD 全流程 + CRITICAL 修复: CLEAR/NEW/RUN/GET 4 处 gwram_free(字符串堆指针) 无效 free 移除。

## M0 GWDATA 模块 (commit ceabce9, 已完成)

7 步 TDD 全流程。

---

## 变更文件清单 (本轮 v110 修复)

| 文件 | 变更 |
|------|------|
| `src/fiveo.c` | fiveo_chain: mode 0 → FM_INPUT (ASM PRGFLI MD_SQI=1 对应)，修复 5 个 CHAIN 测试 FAIL |
| `DEVELOPMENT_PROGRESS.md` | 记录 v110 进度 + make test vs exe 直跑差异发现 |

---

## 验证结果 (本轮)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2119/2119 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2128/2128 PASS** (5 CHAIN FAIL 清零) ✅ |
| 编译警告 | 干净重建 | **0 条** (生产+测试) |
| 集成回归 | gwram 4/4, gwdata 8/8, on_error ERR=11, cond/def_fn 全绿 | ✅ |
| 孤儿 | 声明=注册=709 | **0** ✅ |

---

## 下一步 (按 PLAN_FRESH.md 依赖顺序)

| 模块 | 依赖 | 说明 |
|------|------|------|
| **GWINIT** | GWDATA ✅, GWRAM ✅ | 读 GWINIT.ASM → test_gwinit 扩展 → src/gwinit.c → 提交等待 review |
| **GWMAIN** | GWINIT, GWSTS | 后续 |

## 更新时间
2026-09-19

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。