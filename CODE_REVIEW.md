# GW-BASIC C Port - Code Review v128

**日期**: 2026-10-05
**审查范围**: commit `20b0fc2`（feat: M1 收尾 — nodes[256] 动态化 + 链版本计数器，v127 2 LOW）→ 工作区
**测试状态**: 单元测试 **3064/3064 通过**（0 失败）✅ | .bas 集成 **68/70**（0 失败 + 2 预期跳过）✅ | probe 复测全绿 ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean（v127 报告已随 20b0fc2 入库）
**总体评分**: B- — M1 收尾功能与 v127 2 LOW 修复验证通过，但发现 **1 个 CRITICAL 内存泄漏**（RESEQ 成功路径未释放动态快照）及 1 LOW 观察

---

## 一、概述

本次审查开发者 M1 收尾提交 `20b0fc2`（8 文件，+224/-198）：

| 变更 | 说明 |
|------|------|
| nodes[256] 动态化 | `gwlist_reseq` 快照改动态分配（两遍计数+收集）；ASM 64K 程序 ~21K 行不再截断；FCERR 错误路径释放 |
| 链版本计数器 `g_link_version` | delete/add/init 递增；gwmain 两处循环快照版本，仅链变化时 liveness walk（O(1) fast path，消除 O(n²)） |
| v127 LOW-1 | test_gwlist_delete_range_to_end 改 `(20,65530)` + 补 "20 gone" 断言 |
| v127 LOW-2 | 版本计数器实现（见上） |
| 新测试 | `test_gwlist_reseq_large_program`（300 行，可捕获旧 256 截断） |

### v127 2 LOW 修复确认（验证通过）

| v127 问题 | 修复 | 独立验证 |
|-----------|------|----------|
| LOW-1 假通过测试 | `gwlist_delete_range(20, 65530)`（ASM ALLLST 哨兵）+ `find_line(20)==NULL` "20 gone" 断言 | ✅ 代码复核：start<end 正常删除，断言真实敏感 |
| LOW-2 O(n²) | 版本计数器 fast path：版本不变 → `curr=saved_next`（O(1)），链变化 → liveness walk | ✅ probe 复测：删除场景走 walk、RENUM/普通语句走 fast path 均正确（见下） |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3064/3064 PASS**（775 → 776，+1 test_gwlist_reseq_large_program） |
| .bas 集成套件（70 项） | ✅ **68/70**（0 失败 + 2 预期跳过），test_renum/test_delete/test_delete_exec 均输出校验 PASS |
| 孤儿检测（唯一函数名） | ✅ **776 定义 = 776 注册，0 孤儿 / 0 多余** |
| probe：删除紧邻下一行（fast path 分支） | ✅ `20 DELETE 30` → `start / after`（行 30 不执行） |
| probe：运行中 RENUM（fast path） | ✅ `20 RENUM 100,10,10` → `start / after renum`（继续执行重编号后下一行） |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 |
|---|------|------|
| 1 | **`gwlist_reseq` 成功路径泄漏动态快照 `nodes`** —— 每次 RENUM/RESEQ 泄漏 `count×8` 字节（300 行 ≈ 2.4KB；21K 行最大 ≈ 168KB），反复 RENUM 累积 | `src/gwlist.c`（L103 附近） |

**详情**:

```c
/* src/gwlist.c — gwlist_reseq */
struct program_line **nodes = gwram_alloc((size_t)count * sizeof(struct program_line *));
...
for (int i = 0; i < count; i++) {
    if (new_num > 65529) {
        gwram_free(nodes);        /* 错误路径：有释放 ✅ */
        return ERROR;
    }
    ...
}
return SUCCESS;                   /* 成功路径：无 gwram_free(nodes) ❌ 泄漏 */
```

- **根因**：FCERR 错误路径正确释放，但 `return SUCCESS` 前遗漏释放 —— 开发者释放意图明确，属笔误级遗漏
- **影响**：确定性内存泄漏，每次 RESEQ（含 RENUM 语句与直接命令）泄漏 `count×8` 字节；同一会话多次 RENUM 累积；大程序（数千行）单次即几十 KB
- **修复建议**：`return SUCCESS;` 前补 `gwram_free(nodes);`（一行）。可加 `test_gwlist_reseq_no_leak`（用返回计数/hook 断言 gwram 分配平衡，或上 ASAN）

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **链版本计数器未覆盖 reseq/clear**：`g_link_version++` 仅 3 处（init/add_line/delete_line），`gwlist_reseq` 与 `gwlist_clear`（L238）未递增，与提交消息 "delete/add/reseq/clear/init 递增" 不符 | `src/gwlist.c`、提交消息 |

**详情**:

1. **LOW-1 — 版本计数器语义不完整**：reseq（摘链重插）与 clear（清空链表）都会修改链结构但版本不变。**实测无行为影响**：fast path（版本不变 → `curr=saved_next`）与 liveness walk 分支 1（两者均 alive → `curr=saved_next`）在当前逻辑下等价，probe（运行中 RENUM）实证正确。但"版本不变 ⇒ 链未变"的 fast path 前提在 reseq/clear 下不成立，属防御性语义缺口；且提交消息与实现不符。**建议**：`gwlist_reseq` 与 `gwlist_clear` 补 `g_link_version++`，修正提交消息，保持"版本 = 链结构版本"单一语义。

---

## 三、架构评估

### M1 收尾质量（20b0fc2）

| 决策 | 评价 |
|------|------|
| RESEQ 快照动态分配（两遍计数+收集，O(n)） | ✅ 合理 — 21K 行不再截断；节点指针数组最大 168KB 可接受 |
| FCERR 错误路径释放（`if (new_num > 65529)`） | ✅ 正确 |
| **成功路径释放** | ⚠️ **CRITICAL-1** — 遗漏 `gwram_free(nodes)` |
| 链版本计数器 fast path | ✅ 设计正确 — 消除每语句 O(n) liveness walk（O(n²)→O(1) 常态） |
| fast path 与 walk 行为等价性 | ✅ 已验证（删除→walk、RENUM/普通→fast path，probe 全绿） |
| gwmain 两处循环快照/比对对称 | ✅ 一致 |
| test_gwlist_delete_range_to_end `(20,65530)` + "20 gone" | ✅ LOW-1（v127）修复正确 |
| test_gwlist_reseq_large_program（300 行） | ✅ 断言能捕获旧 256 截断（find_line(3000)==NULL） |
| 版本计数器覆盖范围 | ⚠️ LOW-1 — reseq/clear 未递增（行为无影响，语义/文档问题） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| RESEQ 常规语义（默认/起点/MM/INC=0/溢出/空程序/内容） | ✅ 7 用例 + 语句级 |
| **RESEQ 大程序（>256 行不截断）** | ✅ test_gwlist_reseq_large_program（300 行，新） |
| **RESEQ 成功路径释放** | ❌ 盲区 → CRITICAL-1（无泄漏检测） |
| delete-to-end 语义（20 gone） | ✅ v127 LOW-1 修复 |
| 版本计数器 fast path（普通语句 O(1)） | ✅ probe + 套件回归 |
| 链版本语义完整性（reseq/clear 递增） | ❌ LOW-1（消息承诺 vs 实现） |
| 注册一致性 | ✅ 776=776，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（阻塞项）
1. **CRITICAL-1**：`gwlist_reseq` 成功路径补 `gwram_free(nodes);`（return SUCCESS 前）；建议加泄漏检测（gwram 分配/释放平衡断言或 ASAN）
2. LOW-1：`gwlist_reseq` / `gwlist_clear` 补 `g_link_version++`，修正提交消息（"delete/add/reseq/clear/init 递增"）

### 中期
3. M1 已收尾（RANDOMIZE / RESEQ / DELETE / RENUM / UAF / nodes[256]）→ 进入 M2 规划
4. 其余 65 个 .bas 补充 expected 基线

### 长期
5. CI 门禁：孤儿检测 + 零警告 + fresh clone 构建校验 + .bas 集成回归 + **ASAN（泄漏/UAF）**
6. AddressSanitizer 构建（可捕获 CRITICAL-1 类泄漏与历史 UAF）

---

## 五、总结

**总体评价**: M1 收尾功能正确 —— nodes[256] 动态化解决大程序截断（308 行测试断言可捕获旧 bug）、版本计数器消除 O(n²)（删除走 liveness walk、RENUM/普通语句走 O(1) fast path，probe 实证无回归）、v127 双 LOW 修复到位；单测 3064/3064、套件 68/70、0 警告、0 孤儿。**但 RESEQ 成功路径遗漏 `gwram_free(nodes)`（确定性内存泄漏）必须修复**。
**关键发现**: **1 CRITICAL**（RESEQ 成功路径泄漏快照）、**1 LOW**（版本计数器未覆盖 reseq/clear，消息承诺与实现不符）。M1 宣布全部完成，但泄漏需在 M2 前修复。
**建议**: 补一行 `gwram_free(nodes)`（+建议泄漏断言），reseq/clear 递增版本计数器；修复后 v129 复核，随即进入 M2 规划。

---

## 六、指标追踪

| 指标 | v127 | v128 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3057 | **3064** | ✅ +7（+1 测试函数） |
| 定义数（唯一函数） | 775 | **776** | +1 |
| 注册数（run_test 调用） | 775 | **776** | +1 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 68/70 | **68/70** | 保持 |
| HEAD 独立构建 | ✅ | ✅ | 保持 |
| M1 模块完成 | 4/N | **6/6 全部完成**（RANDOMIZE/RESEQ/DELETE/RENUM/UAF/nodes[256]） | 收尾 |
| RESEQ 内存泄漏 | ⚠️（新引入） | **CRITICAL-1** | 待修复 |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3064 → 套件 68/70 → 孤儿 776=776 → 2 项 probe（删除 fast-path / RENUM fast-path，已清理）
- 泄漏为确定性代码缺陷（成功路径遗漏释放）；`gwram_free` 即标准 `free`，反复 RENUM 累积泄漏
- **M1 状态**: 功能全部完成；CRITICAL-1 修复后 M1 正式关闭，进入 M2