# GW-BASIC C Port - Code Review v126

**日期**: 2026-10-02
**审查范围**: commit `4c9845f`（fix: CODE_REVIEW v125 — CRITICAL-1 UAF + 2 MEDIUM + 3 LOW）→ 工作区
**测试状态**: 单元测试 **3041/3041 通过**（0 失败）✅ | .bas 集成 **68/70**（0 失败 + 2 预期跳过）✅ | test_delete/test_delete_exec 均 **输出校验 PASS** ✅
**编译状态**: 干净重建成功（exit 0）| 编译警告 **0 条**（生产 0 / 测试 0）✅
**工作区状态**: modified: CODE_REVIEW.md（v125 报告未入库）| **untracked: tests/test_gwmain_uaf.c（提交缺失文件）**
**总体评分**: C — 修复了 v125 多数问题（2 MEDIUM 主体 + 3 LOW），但发现 **2 个新 CRITICAL**（UAF 修复不完整 + 提交缺文件）及 2 MEDIUM、2 LOW，需修复后重新 review

---

## 一、概述

本次审查开发者对 v125 全部 6 项的修复提交 `4c9845f`（12 文件，+336/-76）。

### v125 修复确认

| v125 问题 | 修复方式（4c9845f） | 验证 |
|-----------|--------------------|------|
| CRITICAL-1 执行指针 UAF | gwlist_delete_line 返回 next 存活节点；delete_range 返回 range 后 anchor（ASM FNDLIN L791-793）；gwmain 两处循环按 node 身份重定位 | ⚠️ **修复不完整** — 见新 CRITICAL-1 |
| MEDIUM-1 `DELETE 40-` | 开放区间 end=65530（ASM SCNLIN ALLLST L856） | ✅ 正确（test_gwsts_delete_open_range） |
| MEDIUM-2 非数字 | isdigit 校验 → SNERR（ASM LINGET MORLIN L426） | ✅ 主体正确（test_gwsts_delete_non_numeric）；⚠️ 尾随空白回归 — 见新 MEDIUM-1 |
| LOW-1 裸 DELETE | rc=ERROR（SNERR） | ✅（test_gwsts_delete_bare） |
| LOW-2 尾部多余 | 全串校验 → SNERR | ✅（test_gwsts_delete_tail_garbage） |
| LOW-3 测试盲区 | +6 单测 + test_delete_exec.bas（4 场景） | ⚠️ 场景仍避开"删除紧邻下一行" — 见新 LOW-1 |
| 附带修复 | SNERR rc 覆盖 bug / RENUM 回归 node 身份重定位 / TEST_ASSERT NULL | ✅ 逻辑正确 |

### 验证结果（独立执行，当前 checkout 全量重跑）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译 82 个 .c，含工作区 test_gwmain_uaf.c） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3041/3041 PASS**（762 → 771，+9 新测试） |
| .bas 集成套件（70 项） | ✅ **68/70**（0 失败 + 2 预期跳过），test_delete / test_delete_exec 输出校验 ✅ |
| 孤儿检测（唯一函数名） | ✅ **771 定义 = 771 注册，0 孤儿 / 0 多余** |
| 边界实证（probe，已清理） | ❌ 见 CRITICAL-1（两条执行路径均复现） |
| **提交完整性（fresh clone）** | ❌ 见 CRITICAL-2（Makefile 引用未跟踪文件） |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 |
|---|------|------|
| 1 | **UAF 修复不完整：删除"紧邻下一行"时，被删行仍被执行（use-after-free）—— 两条执行路径均实证** | `src/gwmain.c`（RUN 路径 L185+ / .bas 路径 L334+） |
| 2 | **提交缺失文件：tests/Makefile 引用未跟踪的 `test_gwmain_uaf.c` → 干净 clone 构建必然失败；test_runner 注册的 3 个函数定义缺失** | `tests/Makefile`、`tests/test_runner.c`、`tests/test_gwmain_uaf.c`（未提交） |

**详情**:

1. **CRITICAL-1 — 重定位分支 1 使用未校验存活的旧快照**：

   重定位逻辑（gwmain.c）：
   ```c
   if (gwlist_find_line(line_num) == curr) {        /* 分支 1: 当前行存活 */
       curr = saved_next;                            /* ← 使用执行前快照! */
   } else if (saved_next && gwlist_find_line(saved_next->number) == saved_next) {
       curr = saved_next;                            /* 分支 2: 当前行被删, 下行存活 */
   } else {
       curr = gwlist_get_first_line();
       while (curr && curr->number <= line_num) curr = curr->next;  /* 分支 3 */
   }
   ```
   **分支 1 缺陷**：当语句删除的是**当前行的下一行**（如 `20 DELETE 30`），当前行 20 存活 → 进入分支 1 → `curr = saved_next`（执行前快照 = 行 30，已被 free）。下一轮循环解引用已释放节点并执行其 text。

   **实证**（临时 probe，已清理；两条路径均复现）：
   ```
   10 PRINT "start"
   20 DELETE 30
   30 PRINT "LINE 30 SHOULD NOT RUN"
   40 PRINT "after"
   ```
   - `.bas` 路径实测输出：`start` / **`LINE 30 SHOULD NOT RUN`** / `after`
   - `RUN` 交互路径实测输出：`start` / **`L30`** / `after`
   - 被删除的行 30 仍被执行 —— 与修复目标"被删行不再执行"直接矛盾

   **根因**：`curr` 存活时，`gwlist_delete_line` 已把 `curr->next` 更新为区间后的存活行（或 NULL）——**正确写法是 `curr = curr->next`**（活指针），而非执行前快照 `saved_next`（可能是悬垂）。快照仅在"当前行被删"时才需要（分支 2/3）。
   
   **修复建议**：
   ```c
   if (gwlist_find_line(line_num) == curr) {
       curr = curr->next;          /* 存活: delete_line 已更新链, curr->next 必为活节点或 NULL */
   } else if (saved_next && /* 链表身份遍历确认 saved_next 存活, 勿用 find_line(已free->number) */) {
       curr = saved_next;
   } else {
       curr = gwlist_get_first_line();
       while (curr && curr->number <= line_num) curr = curr->next;
   }
   ```
   补充回归用例：`20 DELETE 30`（删紧邻下一行）、`20 DELETE 30-50`、`20 DELETE 30-`（删下一行至末尾）。

2. **CRITICAL-2 — 提交缺文件，HEAD 无法独立构建**：
   - `tests/Makefile` 新增 `../tests/test_gwmain_uaf.c`（TEST_SRCS），但该文件**未 `git add`**（git status: `?? tests/test_gwmain_uaf.c`）
   - `tests/test_runner.c` 注册的 3 个新测试（`gwlist_delete_returns_next` / `gwlist_delete_range_returns_next` / `gwlist_delete_range_to_end`）定义仅存在于该未提交文件
   - **fresh clone 后 `mingw32-make test` → `No rule to make target '../tests/test_gwmain_uaf.c'` → 构建失败**；当前工作区仅因文件幸存而可编译
   - **影响**：4c9845f 是不可独立构建的提交；代码评审/CI/他人 clone 均无法复现
   - **修复建议**：`git add tests/test_gwmain_uaf.c`（提交前确认其 3 个测试的断言与注册一致、0 孤儿；若该文件为废弃草稿，需同步从 Makefile 与 test_runner.c 移除引用并核实 771-3=768 注册）

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **isdigit 全串校验拒绝尾随空白**：`DELETE 40 `、`DELETE 40-60 ` → SNERR；ASM LINGET 以空白作为合法终止符（"DELETE 40 " 应成功）。文件加载路径已剥离行尾空白故实际无影响，但语句级 API 与交互输入路径行为未定义且未测试 | `src/gwsts.c` |
| 2 | **重定位分支 2 在 saved_next 可能已 free 时读取 `saved_next->number`**（UB 读）；当前结果通常正确，但依赖未定义行为，ASAN/内存重用下可能误判 | `src/gwmain.c` |

**详情**:

1. **MEDIUM-1 — 尾随空白**：`DELETE 40-60 ` 的 `endp="60 "` 含空格 → `valid=0 → SNERR`。GW-BASIC 规范中数字后的空白是合法语句终止符。建议：isdigit 校验循环改为"跳过空白后必须到字符串结尾"，或复用 ASM LINGET 的 token 终止规则。
2. **MEDIUM-2 — 分支 2 UB 读**：`saved_next` 指向已 free 节点时，`saved_next->number` 是悬垂读。建议：改为遍历 `g_program_head` 做指针身份比对（不读被删节点字段），或直接用 `gwlist_get_first_line()` 数字扫描（DELETE 不改行号序）。

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | **test_delete_exec.bas 场景盲区**：场景 1 为 `20 DELETE 40`（删除的是 `saved_next=30` **之后**的行），未覆盖"删除紧邻下一行"（`20 DELETE 30`）—— CRITICAL-1 残留因此未被集成测试发现 | `tests/bas/test_delete_exec.bas` |
| 2 | **CODE_REVIEW.md 报告未入库**：4c9845f 消息引用 "CODE_REVIEW v125" 但报告文件未随提交；HEAD 中 CODE_REVIEW.md 仍为 v124，工作区为 v125 草稿（本版 v126 覆盖之） | `CODE_REVIEW.md` |

**详情**:

1. **LOW-1**：建议补充 `20 DELETE 30`（紧邻下一行）/ `20 DELETE 30-` / `20 DELETE 30-50` 三场景到 test_delete_exec.bas（expected 相应更新：被删行内容不得出现）。
2. **LOW-2**：v126 报告（涵盖 v125 修复复核结论与新问题）建议随下次提交一并入库。

---

## 三、架构评估

### 修复质量（4c9845f）

| 决策 | 评价 |
|------|------|
| `gwlist_delete_line` 返回 next 存活节点 / `gwlist_delete_range` 返回 range 后 anchor（ASM FNDLIN L791-793） | ✅ 设计正确 — 与 ASM 对照充分 |
| `DELETE 40-` → end=65530（ASM SCNLIN ALLLST L856），行号上限 65529 哨兵 | ✅ 正确 |
| `DELETE -40` → (0,40)；`DELETE 0` 合法（行 0）；单行/区间/开放区间语义齐备 | ✅ 正确 |
| isdigit 校验 → SNERR；SNERR rc 不被 start>end 检查覆盖（原 bug 修复） | ✅ 主体正确（尾随空白除外） |
| 执行指针重定位按 node 身份（分支 2/3，RENUM 回归暴露） | ✅ “删除当前行/区间含当前行” 场景正确（test_delete_exec 场景 2/3 实测通过） |
| **分支 1（当前行存活时）用 saved_next 快照** | ⚠️ **CRITICAL-1** — 正确写法为 `curr->next`（delete_line 已更新为活指针） |
| test_gwmain_uaf.c 接入 Makefile + test_runner | ⚠️ **CRITICAL-2** — 文件未提交 |
| 新测试 9 个 + test_delete_exec.bas（4 场景，含 expected 校验） | ✅ 注册完整（771=771，0 孤儿） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| delete_line/delete_range 返回值 API（测试 3 用例） | ✅（但仅在未提交文件，见 CRITICAL-2） |
| DELETE 语句语义（单行/区间/开放/开头/非数字/裸/尾部/行 0，6 新用例） | ✅ 全面 |
| **执行指针：删除紧邻下一行（20 DELETE 30）** | ❌ **盲区 → CRITICAL-1 未被发现** |
| 执行指针：删除自己 / 区间含自己 | ✅（场景 2/3） |
| .bas 集成（70 项） | ✅ 68/70，test_delete/test_delete_exec 输出校验 |
| 注册一致性 | ✅ 771=771，0 孤儿 0 多余 |

---

## 四、改进建议

### 短期（阻塞项，必须修复）
1. **CRITICAL-1**：分支 1 改 `curr = curr->next`；分支 2 用链表身份校验；补 `20 DELETE 30` / `20 DELETE 30-` / `20 DELETE 30-50` 回归（.bas 与语句级）
2. **CRITICAL-2**：提交 `tests/test_gwmain_uaf.c`（或撤除 Makefile 与 test_runner.c 的引用）— 恢复 HEAD 可独立构建
3. LOW-2：本 v126 报告与修复一并入库

### 短期（非阻塞）
4. MEDIUM-1：isdigit 校验兼容尾随空白（ASM LINGET 终止符规则）
5. MEDIUM-2：分支 2 避免读取已 free 节点字段

### 中期
6. nodes[256] 上限确认（M1 收尾）
7. 其余 65 个 .bas 补充 expected 基线

### 长期
8. CI 门禁：孤儿检测 + 零警告 + **fresh clone 构建校验（可捕获 CRITICAL-2 类）** + .bas 集成回归
9. AddressSanitizer 构建（可捕获 CRITICAL-1 类 UAF）

---

## 五、总结

**总体评价**: v125 的 DELETE 语义修复（开放区间 65530 / 非数字 SNERR / 裸 DELETE / 尾部多余 / 行 0）与 API 重构（delete_line 返回 next / delete_range 返回 anchor）正确且测试充分（3041 单测、68/70 套件全绿）；**但执行指针 UAF 修复不完整** —— `20 DELETE 30`（删除紧邻下一行）时分支 1 使用已释放的 `saved_next` 快照，被删行仍被执行（两条执行路径实证），且 **提交缺失 `test_gwmain_uaf.c` 导致 HEAD 无法独立构建**。
**关键发现**: **2 CRITICAL**（UAF 残留、提交缺文件）、**2 MEDIUM**（尾随空白 SNERR、分支 2 UB 读）、**2 LOW**（测试盲区、报告未入库）。
**建议**: 修复两个 CRITICAL（分支 1 改 `curr->next` + 补"删紧邻下一行"回归；提交缺失文件）后重新提交 v127 review。

---

## 六、指标追踪

| 指标 | v125 | v126 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3013 | **3041** | ✅ +28（+9 测试函数） |
| 定义数（唯一函数） | 762 | **771** | +9 |
| 注册数（run_test 调用） | 762 | **771** | +9 |
| 孤儿数 | 0 | **0** | 保持 |
| 重复/多余注册 | 0 | **0** | 保持 |
| 生产警告 | 0 | **0** | 保持 |
| 测试警告 | 0 | **0** | 保持 |
| .bas 通过 | 67/69 | **68/70** | ✅ +1（test_delete_exec） |
| test_delete_exec.bas | — | ✅ 输出校验 PASS | 新增 |
| DELETE 语句分支 | ✅（含 CRITICAL-1） | ✅（语义）+ ⚠️ UAF 残留 | 修复中 |
| M1 模块完成 | 4/N | **4/N** | 保持（UAF 阻塞收尾） |
| HEAD 独立构建 | ✅ | ❌ **CRITICAL-2** | 回归 |

**注释**:
- 独立验证全程重跑：干净重建 → 单测 3041 → 套件 68/70 → 孤儿 771=771 → 边界 probe（2 条执行路径 UAF 复现，已清理）
- **当前工作区状态**：仅 CODE_REVIEW.md（本版）+ test_gwmain_uaf.c（未跟踪）；构建产物已忽略
- 修复建议细化见第二章/第四章；**M1 收尾前需 CRITICAL-1/2 修复 + 提交完整性恢复**