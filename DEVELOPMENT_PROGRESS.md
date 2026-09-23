# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 全部完成 ✅ → **M1 进行中** (GWSTS 深化 ✅ / GWLIST 行管理深化 ✅) |
| **当前阶段** | M1 第 2 批 TDD 7 步完成，待提交等待 review |
| **依赖顺序** | M0 ✅ → M1 (RANDOMIZE ✅ → RESEQ/DELETE ✅ → .bas 覆盖) → M2+ |
| **TDD 工作流** | 7 步标准化流程 |

---

## 当前轮: M1 第 2 批 — GWLIST 行管理深化 (TDD 7 步完成)

### 步骤状态

| 步骤 | PLAN_FRESH.md 描述 | 状态 |
|------|-------------------|------|
| 1 | 读 ASM (GWMAIN.ASM RESEQ L3102-3222 + SCCLIN/SCCPTR L3222-3325) | ✅ RESEQ 三参数语义 (NN/MM/INC) + 越界/INC=0 错误分析 |
| 2 | 编写测试 (RED) | ✅ test_gwlist_m1.c 新增 10 测试 (reseq x7/delete_range x3) — RED 确认 (链接失败) |
| 3 | .bas 集成验证 | ✅ 通过 test_gwbasic.exe 全量回归覆盖 |
| 4 | 实现 (GREEN) | ✅ gwlist_reseq() + gwlist_delete_range() (gwlist.c) + gwbasic.h 声明 |
| 5 | mingw32-make test 至绿 | ✅ **2976/2976 PASS, 0 警告** |
| 6 | 完整 exe + 集成回归 | ✅ test_gwbasic 2976/2976; gwdata 8/8, gwram 4/4, on_error ERR=11, cond/def_fn 全绿 |
| 7 | 提交 + 等待 review | ⏳ 待提交 |

### ASM 依据 (GWMAIN.ASM)

| ASM 位置 | 结构 | C 实现 |
|----------|------|--------|
| L3102-3111 | `RESEQ [NN[,MM[,INC]]]` — 默认 CX=10 INC, DE=0 全部行 | gwlist_reseq(first=0→10, mm=0→all, inc=0→10) |
| L3140-3142 | `OR DX,DX / JZ / JMP FCERR` — INC=0 非法 | inc==0 && mm>0 → ERROR |
| L3176-3181 | `MOV BX,65529 / CMP / JAE / JMP FCERR` — 行号过大 | new_num > 65529 → ERROR |
| L3107 | LINES LESS THAN MM NOT RESEQUENCED | threshold (mm) 检查 |
| L3236 (RESNX1) | `MOV [BX],DX` — 原位改行号，内容保持 | node->number = new_num (原位) |
| L3222-3325 | SCCLIN/SCCPTR: 行号↔指针转换, "Undefined line " 消息 | gwlist 节点直接定位 (无需转换) |

### 本轮修复

| 问题 | 根因 | 修复 |
|------|------|------|
| RESEQ 后 crash (0xC0000374) | 初版 delete+re-add: gwlist_delete_line 已 free text，再 add 读已释放内存 + gwram_free(text) 双重 free | 原位改行号 (node->number = new_num)，无 delete/free |
| RESEQ(100,0,50) rc=-1 | 重定位把已处理节点移到 curr 之后，保存的 next 链再次遇到已处理节点 → 级联重编号 → 最终溢出 ERROR | 快照法: 先收集合格节点指针数组，再逐个重编号+重定位 (每行只处理一次) |
| gwlist_init undefined | 重写时不小心删除了 gwlist_init 定义 | 恢复定义 |
| gwlist_reseq/delete_range implicit declaration | gwbasic.h 无声明 | 补充声明 |

### 变更文件

| 文件 | 变更 |
|------|------|
| `src/gwlist.c` | gwlist_reseq + gwlist_delete_range 实现 (ASM RESEQ 1:1) |
| `src/gwbasic.h` | gwlist_reseq/gwlist_delete_range 声明 |
| `tests/test_gwlist_m1.c` | 新增 10 测试 |
| `tests/test_runner.c` | 注册 10 个测试 (747 声明 = 747 注册) |
| `tests/Makefile` | 加入 test_gwlist_m1.c |

---

## 验证结果 (本轮)

| 测试类型 | 命令 | 结果 |
|----------|------|------|
| 注册测试 | `mingw32-make test` | **2976/2976 PASS** |
| 完整测试 | `./tests/test_gwbasic.exe` | **2976/2976 PASS** |
| 编译警告 | 干净重建 (生产+测试) | **0 条** |
| 孤儿 | 声明=注册=747 | **0** ✅ |
| 重复注册 | uniq -d | **0** ✅ |
| 集成回归 | gwdata/gwram/on_error/cond/def_fn/math | 全绿 ✅ |

---

## M1 剩余 (下轮)

| 优先级 | 任务 |
|--------|------|
| 1 | .bas 集成测试补充 (提升覆盖) |
| 2 | RESEQ/DELETE 的 .bas 集成测试 (RENUM 命令语句分支) |

## 更新时间
2026-09-23

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe。