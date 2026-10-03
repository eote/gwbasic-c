# GW-BASIC C Port - Code Review v127

**日期**: 2026-10-04
**审查范围**: commit `91c9b82`（fix: CODE_REVIEW v126 — CRITICAL-1 UAF + CRITICAL-2 缺文件 + 2 MEDIUM + 2 LOW）→ 工作区
**测试状态**: 单元测试 **3057/3057 通过**（0 失败）✅ | .bas 集成 **68/70**（0 失败 + 2 预期跳过）✅ | probe 复测全绿 ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: clean（v126 报告已随 91c9b82 入库）
**总体评分**: A- — v126 全部 6 项修复验证通过（含 4 项边界复测 + 2 条执行路径），新发现 2 LOW（测试断言有效性、性能观察）

---

## 一、概述

本次审查开发者对 v126 全部 6 项的修复提交 `91c9b82`。

### v126 6 项修复确认（全部验证通过）

| v126 问题 | 修复方式（91c9b82） | 独立验证 |
|-----------|--------------------|----------|
| 🔴 CRITICAL-1 UAF 残留 | 重定位改 `gwlist_is_alive()` 指针身份 liveness（RENUM 重编号当前行仍 alive，`find_line(旧号)` 判定错误 — test_renum.bas 回归暴露）；curr+saved_next 均 alive → 物理下一行（ASM SAVTXT）；否则 number 扫描 | ✅ **4 项 probe 复测 + 2 条执行路径全绿**（见下） |
| 🔴 CRITICAL-2 提交缺文件 | `tests/test_gwmain_uaf.c` 已提交（+58），Makefile/test_runner 引用闭合 | ✅ HEAD 独立构建恢复（全量重建 exit 0） |
| 🟨 MEDIUM-1 尾随空白 | isdigit 校验兼容尾随空白（ASM LINGET 终止符）+ garbage 再检查 | ✅ probe `20 DELETE 30 ` → 成功且行 30 不执行 |
| 🟨 MEDIUM-2 分支 2 UB 读 | `gwlist_is_alive()` 仅指针比较，不读已 free 节点字段 | ✅ 实现复核通过 |
| 🟩 LOW-1 测试盲区 | test_delete_exec.bas +S5/S6/S7（删紧邻下一行/至末尾/区间）+ expected | ✅ 套件输出校验 PASS |
| 🟩 LOW-2 报告未入库 | CODE_REVIEW.md（v126）随提交入库 | ✅ 工作区 clean |

### CRITICAL-1 修复复测（决定性，独立 probe，已清理）

| 场景 | probe | 实测输出 | 结论 |
|------|-------|----------|------|
| 删除紧邻下一行（.bas 路径） | `20 DELETE 30` | `start` / `after`（**无** "LINE 30 SHOULD NOT RUN"） | ✅ 修复 |
| 删除紧邻区间（.bas 路径） | `20 DELETE 30-50` | `start` / `after`（L30/L40/L50 均不执行） | ✅ 修复 |
| 删除到末尾 | `20 DELETE 30-` | `start`（30/40/200 全删，无后续行） | ✅ 修复 |
| 删除紧邻下一行（**RUN 交互路径**） | 交互 `20 DELETE 30` + `RUN` | `start` / `after`（行 30 不执行） | ✅ 修复 |
| 尾随空白 | `20 DELETE 30 ` | `start` / `after`（成功，行 30 不执行） | ✅ 修复 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译，含已提交 test_gwmain_uaf.c） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3057/3057 PASS**（771 → 775，+4 新测试） |
| .bas 集成套件（70 项） | ✅ **68/70**（0 失败 + 2 预期跳过），test_renum / test_delete / test_delete_exec（S5-S7）均输出校验 PASS |
| 孤儿检测（唯一函数名） | ✅ **775 定义 = 775 注册，0 孤儿 / 0 多余** |

**修复质量细节**（代码复核）：

```c
/* gwmain.c 重定位逻辑 — 两处执行路径一致 */
if (gwlist_is_alive(curr) && gwlist_is_alive(saved_next)) {
    curr = saved_next;              /* 均存活 → 物理下一行（ASM SAVTXT；= 原 curr->next 语义） */
} else {
    curr = gwlist_get_first_line(); /* 任一被删 → number 扫描 > line_num 的第一存活行 */
    while (curr && curr->number <= line_num) curr = curr->next;
}
```
- 场景覆盖：删下一行 / 删自己 / 删自己+后续 / 删区间含两者 / 删到末尾 / RENUM 重编号 → 全部正确（含 S5-S7 与 RENUM 回归）
- `gwlist_is_alive()` 仅遍历链表做指针比较（`struct program_line *p = head; while (p) { if (p == node) return 1; p = p->next; }`），不读节点字段 ✅

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **test_gwlist_delete_range_to_end 为"假通过"测试**：调用 `gwlist_delete_range(20, 0)`，因 `start>end` 提前返回 NULL，**实际未删除任何行**；断言不敏感，测试意图（"20-末尾"）未被验证 | `tests/test_gwmain_uaf.c` |
| 2 | **性能观察**：执行循环每个语句后调用 2 次 `gwlist_is_alive()`（O(n) 链表遍历）→ 整程序 O(n²)；n=数千行时整体可感知，非阻塞 | `src/gwmain.c`、`src/gwlist.c` |

**详情**:

1. **LOW-1 — 假通过测试**：
   ```c
   /* tests/test_gwmain_uaf.c */
   struct program_line *next = gwlist_delete_range(20, 0);   /* '(20-末尾)' 注释 */
   TEST_ASSERT(next == NULL, "delete to end returns NULL");
   TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
   ```
   - `gwlist_delete_range` 的 `if (start > end) return NULL;` 使 `(20, 0)` 提前返回 — **行 20 从未被删除**
   - 注释意图是"删 20 到末尾"，但缺少关键断言 `gwlist_find_line(20) == NULL`（"20 gone"）；两条断言在删除/不删除两种路径下均成立，无法区分实现意图
   - **修复建议**：改为 `gwlist_delete_range(20, 65530)`（与 DELETE `40-` 的 65530 哨兵及 SCNLIN ALLLST 语义一致），并补 `find_line(20) == NULL` 断言（顺带验证 `-40` 起始形式可在语句层覆盖）
2. **LOW-2 — 性能回归**：原实现每语句 O(1)（`curr = curr->next`）；新实现每语句 2×O(n) 遍历。10 万语句 × 1 万行程序 ≈ 2×10⁹ 指针比较（秒级）。**建议**：仅当语句修改了链表才做 liveness 检查 —— 可在 `gwlist.c` 维护全局"链表版本计数器"（每次 delete/add 递增），gwmain 保存执行前版本号，版本变化才重定位；或对 `gwsts_exec_statement` 增加返回值/标识告知"链已修改"。

---

## 三、架构评估

### 修复质量（91c9b82）

| 决策 | 评价 |
|------|------|
| `gwlist_is_alive()` 指针身份 liveness（不读 free 节点字段） | ✅ 正确 — 闭合 UAF 读；MEDIUM-2 解决 |
| 重定位：均 alive → 物理下一行；否则 number 扫描 | ✅ 正确 — 覆盖全部删除/RENUM 场景（S5-S7 + 复测实证） |
| RENUM 回归（重编号当前行仍 alive，勿用 find_line(旧号) 判定） | ✅ 设计要点正确 |
| isdigit 兼容尾随空白 + garbage 再检查（ASM LINGET） | ✅ 正确 — MEDIUM-1 解决 |
| test_gwmain_uaf.c 提交 + Makefile/test_runner 引用闭合 | ✅ 正确 — CRITICAL-2 解决，HEAD 独立构建恢复 |
| test_delete_exec S5/S6/S7 + expected | ✅ 精准对应 v126 盲区 |
| CODE_REVIEW.md 随提交入库 | ✅ 流程闭合 |
| gwlist_delete_range(20, 0) 测试调用 | ⚠️ LOW-1 — 假通过（未验证 delete-to-end 语义） |
| 每语句 2×O(n) liveness 遍历 | ⚠️ LOW-2 — O(n²) 性能观察 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| DELETE API（单行/区间/开放/越界/空程序/返回值） | ✅ 覆盖完整 |
| DELETE 语句语义（单行/区间/开放/头/非数字/裸/尾随/行 0） | ✅ 覆盖完整 |
| **执行指针（删紧邻下一行 / 删自己 / 删区间 / 删到末尾 / RENUM）** | ✅ **S5-S7 + 复测实证，v126 盲区已闭合** |
| delete-to-end API 语义 | ❌ LOW-1（测试假通过，未验证） |
| 注册一致性 | ✅ 775=775，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（非阻塞）
1. **LOW-1**：修 test_gwlist_delete_range_to_end — 改 `(20, 65530)` + 补"20 gone"断言
2. **LOW-2**：链版本计数器或语句返回"链已修改"标志，避免每语句 O(n) liveness 遍历（若性能可感知再实施，n 小可推迟）

### 中期
3. nodes[256] 上限确认（M1 收尾最后一项）
4. 其余 65 个 .bas 补充 expected 基线

### 长期
5. CI 门禁：孤儿检测 + 零警告 + fresh clone 构建校验 + .bas 集成回归
6. AddressSanitizer 构建（本次 UAF 类问题可被 ASAN 捕获，建议纳入 CI 备选）

---

## 五、总结

**总体评价**: v126 的 6 项问题全部修复且独立验证通过 —— 决定性 probe（`20 DELETE 30`）在 .bas 与 RUN 两条执行路径均确认被删行不再执行，`DELETE 30-50`/`DELETE 30-`/尾随空白全部正确；单测 3057/3057、套件 68/70、0 警告、0 孤儿，HEAD 独立构建恢复。执行指针语义（ASM SAVTXT 对照）与测试基础设施已闭合。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM，**2 个 LOW**（test_gwlist_delete_range_to_end 假通过断言、O(n²) 性能观察）。无阻塞性缺陷。
**建议**: 补 LOW-1 断言（20-末尾语义 + "20 gone"）验证 delete-to-end API；M1 最后一项 nodes[256] 上限确认后完成 M1 收尾。

---

## 六、指标追踪

| 指标 | v126 | v127 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3041 | **3057** | ✅ +16（+4 测试函数） |
| 定义数（唯一函数） | 771 | **775** | +4 |
| 注册数（run_test 调用） | 771 | **775** | +4 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 68/70 | **68/70** | 保持（+S5-S7 场景） |
| test_delete_exec.bas | ✅ | ✅（S5-S7 新增） | 保持 |
| HEAD 独立构建 | ❌（CRITICAL-2） | ✅ **恢复** | 修复 |
| DELETE 执行指针 | ❌ UAF 残留 | ✅ **修复** | 修复 |
| M1 模块完成 | 4/N | **4/N** | 保持（还差 nodes[256] 收尾） |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3057 → 套件 68/70 → 孤儿 775=775 → 5 项 probe 复测（2 条执行路径，已清理）
- 91c9b82 为当前 HEAD，工作区 clean；LOW-1/LOW-2 为非阻塞建议，可随 nodes[256] 收尾一并处理
- **M1 进度**: 4/N + 执行指针修复闭合；最后一项 nodes[256] 上限确认