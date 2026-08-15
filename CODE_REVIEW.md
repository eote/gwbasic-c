# GW-BASIC C Port - Code Review v51

**日期**: 2026-08-16
**审查范围**: HEAD `ae76df4`（工作区已还原为 v48 状态）
**测试状态**: 1084/1084 全部通过 ✅
**编译状态**: 无错误（编译通过）✅
**进度**: 44/44 模块 (100%)
**总体评分**: A (优秀) — 当前提交状态健康

---

## 一、概述

本次审查发现工作区状态发生了重大变化：

1. **v50 审查的大规模功能开发已消失** — 2026-08-16 审查时工作区存在约 4800 行的未提交功能开发（gweval 表达式求值重构、PRINT 语句、FOR/NEXT、DEBUG 宏基础设施等）。当前工作区与 HEAD `ae76df4` **完全一致**，无任何已修改源文件。经查无 stash、无其他分支、无 reset 记录 — 该变更已被 `git restore`/`git checkout` 还原。

2. **CODE_REVIEW.md 也被还原为 v39 内容** — 之前的 v44-v50 审查文档已被替换（该文件被 .gitignore 忽略，不归 git 跟踪，为手动还原）。

3. **补审 v48 提交 (ae76df4) 的 dskcom.c 643 行变更** — v48 审查时仅检查了 fiveo.c 和测试文件，遗漏了 dskcom.c。本次补审发现该变更的真相是**行尾符转换**（LF → CRLF），真实逻辑变更仅 11 行。

4. **工作区遗留未跟踪文件** — TEST_PLAN.md、tests/bas/（26 个 .bas 集成测试）、4 个 .DAT 文件、2 个 .log 文件。

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。（v50 报告的 HIGH-1「PRINT 字符串堆耗尽」等问题的代码已从工作区移除 — 见「重要提醒」）

### 🟩 LOW

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | dskcom.c 行尾符不一致（唯一 CRLF 文件） | `src/dskcom.c` | 全部 |
| 2 | tests/test_fiveo.c 删除未提交（v44 起遗留） | `tests/test_fiveo.c` | — |
| 3 | 根目录测试工件未加入 .gitignore | `.gitignore` | — |
| 4 | v50 大规模功能开发被还原 — 需确认是否是有意为之 | — | — |

**详情:**

#### LOW-1: dskcom.c 是唯一 CRLF 行尾符文件

v48 提交 (ae76df4) 将 `src/dskcom.c` 从 LF 转换为 CRLF（643 行 diff 的来源）。当前仓库 40+ 源文件/头文件中，**仅 dskcom.c 使用 CRLF**，其余全部为 LF。

**修复建议:**
```bash
# 将 dskcom.c 转换回 LF
python -c "d=open('src/dskcom.c','rb').read().replace(b'\r\n',b'\n'); open('src/dskcom.c','wb').write(d)"
# 或添加 .gitattributes 统一管理行尾符
```

#### LOW-2: tests/test_fiveo.c 删除未提交

`tests/test_fiveo.c` 自 v44 审查起就显示为已删除状态，但删除从未被提交。该文件在工作区中实际不存在（被 test_fiveo_ext.c 取代），但 git 索引仍跟踪它。

**修复建议:** 提交时一并 `git rm tests/test_fiveo.c`。

#### LOW-3: 根目录测试工件未加入 .gitignore

以下未跟踪文件不受 `.gitignore` 管理（`git check-ignore` 确认）：

| 文件 | 来源 |
|------|------|
| `LINEIN.DAT`、`RNDTEST.DAT`、`TESTSEQ.DAT`、`test.dat` | .bas 测试运行产生的数据文件 |
| `build.log`、`test_output.log` | 构建/测试输出捕获 |
| `tests/bas/`（26 个 .bas 文件） | **注意:** 这些是测试资产，**建议跟踪**而非忽略 |
| `TEST_PLAN.md` | 测试计划文档，**建议跟踪** |

**修复建议:**
```gitignore
# 追加到 .gitignore
/*.DAT
/build.log
/test_output.log
```
同时 `git add` 添加 `tests/bas/` 和 `TEST_PLAN.md`（它们是有价值的测试资产）。

#### LOW-4: v50 大规模功能开发被还原 — 需确认意图

v50 审查时工作区包含约 4800 行未提交变更（核心是解释器功能：表达式求值 parser、PRINT、FOR/NEXT），且 v50 报告了 1 个 HIGH（PRINT 字符串堆耗尽）。当前这些代码已从工作区完全消失。

**可能的情形:**
- **有意还原**: 开发者放弃该方案或需要重新设计（例如接受 v50 的 HIGH-1 反馈后决定重做）
- **误操作**: 意外的 `git restore .` 导致工作丢失

⚠️ **若是误操作，代码无法恢复**（无 stash、无分支、无 reflog 记录）— 但 v50 审查文档中记录了关键代码结构，可用于重建。

---

## 三、架构评估

### v48 提交 (ae76df4) 补审 — dskcom.c

| 指标 | 数值 |
|------|------|
| diff 总行数 | +327/-316 (643) |
| 忽略空白后的逻辑变更 | **+11/-0** |
| 真实逻辑内容 | `dskcom_find_free_filenum()` 函数 |

```c
/* src/dskcom.c — v48 新增 (唯一逻辑变更) */
int dskcom_find_free_filenum(void) {
    /* Scan from 1 to 255 for a free file number */
    for (int filenum = 1; filenum <= 255; filenum++) {
        if (!dskcom_find_file(filenum)) {
            return filenum;
        }
    }
    return 0;  /* All file numbers in use */
}
```

**评价:** 实现正确 — 扫描 1-255 查找空闲文件号，全部占用时返回 0，被 fiveo_chain 正确使用（v48 已审）。行尾符转换无功能影响但产生代码卫生问题（LOW-1）。

### 文件组织（当前 HEAD）

- 工作区 = HEAD `ae76df4`（无已修改文件）
- 未跟踪文件 4 类（见 LOW-3）

### 测试覆盖分析

| 指标 | v50 | v51 (当前) |
|------|-----|-----------|
| 总测试计数 | 1085 | **1084** |
| 说明 | 含未提交 test_giodsk 增强 (PUT→GET 回读) | 已还原，恢复 v48 基准 |

**1084/1084 全部通过** — 当前提交状态完全健康。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟩 修复 LOW-1**: dskcom.c 行尾符转换回 LF（一行 python 命令）
2. **🟩 修复 LOW-2**: 提交 `git rm tests/test_fiveo.c`
3. **🟩 修复 LOW-3**: .gitignore 添加根目录工件规则；`git add` 跟踪 tests/bas/ 和 TEST_PLAN.md
4. **🟩 确认 LOW-4**: 确认 v50 大规模功能开发的还原是否是有意为之

### 中期 (下个 sprint)

5. 若重新实施 v50 的功能开发（PRINT/FOR-NEXT/表达式求值），**修复 v50 报告的 HIGH-1（PRINT 字符串堆耗尽）后再提交**：
   - `gwsts_print_console` 中 `gweval_eval_string()` 返回值未释放
   - 字符串堆 (16KB bump allocator) 不回收 — 长运行后 PRINT 字符串静默输出 "0"
6. 清理 next86.c 缩进混乱（4-36 空格混用）和 DEBUG_PRINT 残留
7. PRINT 改用 giocon_print_str/print_num（模块边界）

### 长期

8. 添加 .gitattributes 统一管理行尾符，避免 CRLF/LF 混用
9. 将 tests/bas/ 的 .bas 集成测试接入测试框架（目前仅手动运行）

---

## 五、总结

**总体评价**: 当前 HEAD (ae76df4) 状态健康 — 1084/1084 测试通过，无 CRITICAL/HIGH 问题。v48 提交的 dskcom.c 643 行"大变更"实为行尾符转换（逻辑变更仅 11 行），无功能风险。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 0
- 🟨 MEDIUM: 0
- 🟩 LOW: 4（均为代码卫生/流程问题）

**重要提醒**:
1. **v50 大规模功能开发（~4800 行）已从工作区消失** — 请确认是有意还原还是误操作。若需重建，v50 审查文档记录了代码结构和关键问题。
2. **若重新提交该功能开发，必须先修复 HIGH-1（PRINT 字符串堆耗尽）** — 该问题已在 v50 审查中详细记录（gwsts.c:648-652）。

**编译与测试**:
- ✅ `mingw32-make` 编译成功
- ✅ 1084/1084 测试全部通过
- ✅ 工作区与 HEAD 一致，无未提交源码变更

**建议**: 修复 4 个 LOW 项（行尾符、git 清理、.gitignore），并确认 v50 变更去向后再继续功能开发。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-16
**审查版本**: v51 — 工作区状态确认 + v48 补审