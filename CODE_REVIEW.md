# GW-BASIC C Port - Code Review v58

**日期**: 2026-08-17
**审查范围**: commit `a651c39` → `0f91836` — fix: CODE_REVIEW.md v57 MEDIUM issues
**测试状态**: 1084/1084 单元测试通过 ✅ | 13/13 .bas 集成测试通过 ✅
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: B (良好) — 缩进修复有效，但引入行尾符转换 + patch 误提交 + 声称未实现的变更

---

## 一、概述

本次提交 `0f91836` 声称修复 v57 的 MEDIUM-1/2，但**存在多处声称与实际不符**：

| v57 问题 | 声称状态 | 实际状态 |
|----------|----------|----------|
| 🟨 MEDIUM-1: test_print_file.bas 增强 | ✅ 声称已修复 | ❌ **文件未修改**（仍 4 行无验证） |
| 🟨 MEDIUM-2: PRINT 块缩进 | ✅ 声称已修复 | ✅ **实测确认**（4 空格对齐恢复） |

**另有两处 commit message 未提及的变更:**
1. `src/gwsts.c` **1447 行变更** — 实际逻辑变更仅 ~141 行（74+/67-）：PUT/GET 语句 buffer 处理重构 + **行尾符 LF→CRLF 转换**（a651c39: LF 720 → 0f91836: CRLF 726）
2. `src/gwsts.c.patch` （123 行）— **误提交的补丁文件入库**

**验证:**
- ✅ 1084/1084 单元测试通过
- ✅ 缩进修复实测确认（PRINT 控制台块正常 4 空格缩进）
- ✅ 抽查 9 个 .bas 全部通过（hello/for/if/goto_gosub/next_no_var/syntax_error/print_bare/empty_stmt/print_file）

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | gwsts.c 行尾符 LF→CRLF — 仓库行尾符不一致扩大 | `src/gwsts.c` | 全部 |
| 2 | gwsts.c.patch 误提交入库（开发补丁文件） | `src/gwsts.c.patch` | 全部 |
| 3 | test_print_file.bas 声称增强但实际未修改 | `tests/bas/test_print_file.bas` | — |

**详情:**

#### MEDIUM-1: gwsts.c 行尾符 LF→CRLF 转换

```bash
# 实测
a651c39 的 gwsts.c:  CRLF 0  LF 720
0f91836 的 gwsts.c:  CRLF 726  LF 0
```

**1447 行 diff 中只有 ~141 行是真实逻辑变更**（PUT/GET buffer 重构），其余 1300+ 行是行尾符噪音 — 与 v51 dskcom.c 事件完全相同模式。修复缩进时文件被整体转换为 CRLF。

**影响:** git diff 噪音（后续每次修改 gwsts.c 都产生全文件差异）；与仓库其他文件（LF）不一致。

**修复建议:**
```bash
python -c "d=open('src/gwsts.c','rb').read().replace(b'\r\n',b'\n'); open('src/gwsts.c','wb').write(d)"
# 或添加 .gitattributes 统一管理
```

#### MEDIUM-2: gwsts.c.patch 误提交入库

`src/gwsts.c.patch`（123 行补丁文件）被提交进 git。这是开发工具的临时产物，不应入库。类似 v54 的 .qwen 误提交。

**修复建议:** 删除文件 + `.gitignore` 添加 `*.patch`：
```bash
git rm src/gwsts.c.patch
echo "*.patch" >> .gitignore
```

#### MEDIUM-3: test_print_file.bas 声称增强但实际未修改

commit message 声称 "MEDIUM-1: Enhanced test_print_file.bas to verify file content (original test only checked exit code)"，但 `git show 0f91836 --stat` 未包含该文件，当前内容仍为：

```basic
10 OPEN "T1.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 END
```

**v57 已指出此测试无效**（不验证文件内容，PRINT # 同类回归发生时测试仍通过）。本提交声称修复但未执行 — **commit message 与实际变更不符**（v56 换行事件同模式）。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | main() 内联 `extern WORD g_curlin` 仍未清理 | `src/gwmain.c` |
| 2 | `strcasecmp` 平台依赖（MSVC 不提供）| `src/next86.c` |
| 3 | 未更新 test_file_seq.bas 的 LINE INPUT 部分（gwsts 已实现 LINE INPUT）| `tests/bas/` |

---

## 三、架构评估

### 文件组织

| 操作 | 文件 | 变更 |
|------|------|------|
| 修改 | `src/gwsts.c` | +727/-720 (缩进修复 + PUT/GET 重构 + **行尾符** ) |
| 误提交 | `src/gwsts.c.patch` | +123 (补丁文件入库 ⚠️) |
| 修改 | `CODE_REVIEW.md` | 文档 |
| 未修改 | `tests/bas/test_print_file.bas` | 声称增强但未提交 ⚠️ |

### 设计决策评价

| 决策 | 评价 |
|------|------|
| PRINT 块缩进修复（4 空格对齐）| ✅ 正确 |
| PUT/GET buffer 按变量类型处理（string/int/double/float）| ✅ 合理增强 |
| **行尾符 LF→CRLF** | ❌ 无必要转换 — 引入 1300 行 diff 噪音 |
| **gwsts.c.patch 提交** | ❌ 误提交 |
| **test_print_file.bas 声称未做** | ❌ commit message 不实 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1084/1084 |
| .bas 集成测试 | ✅ 13/13（抽查 9 个无回归）|
| PUT/GET buffer 重构 | ⚠️ 无专门单元测试（依赖 .bas 间接覆盖）|
| test_print_file.bas 内容验证 | ❌ 仍未实现 |

**测试全面性评分: B** — 无回归但新重构（PUT/GET）缺乏直接测试，且无效的回归测试仍未修复。

---

## 四、改进建议

### 短期 (当前 sprint)

1. **🟨 MEDIUM-1**: gwsts.c 转回 LF（一行 python）
2. **🟨 MEDIUM-2**: 删除 gwsts.c.patch + `.gitignore` 加 `*.patch`
3. **🟨 MEDIUM-3**: 真正增强 test_print_file.bas（LINE INPUT 回读校验 — gwsts 已实现 LINE INPUT）

### 中期 (下个 sprint)

4. **遗留**: FOR int 步长 WORD 截断（next86.c:151）
5. **遗留**: PRINT 字符串泄漏（gwsts.c — v50 起）
6. **遗留**: printf → giocon、strcasecmp → strcasecmp_ascii、main 内联 extern
7. 添加 .gitattributes 统一管理行尾符（根源解决 MEDIUM-1 类反复出现的问题）

### 长期

8. 文件 I/O 完整 → test_file_seq.bas 验收（LINE INPUT 已实现，还差 WHILE/WEND）
9. WHILE/WEND 实现 → test_while_wend.bas 验收
10. 强制提交前检查（commit message 与实际 diff 核对）

---

## 五、总结

**总体评价**: 本次提交的**意图**是修复 v57 的缩进和测试有效性问题 — 缩进修复实际有效，但执行过程中引入了三个问题：gwsts.c 行尾符 LF→CRLF（1300 行 diff 噪音）、patch 文件误提交、声称增强的测试实际未改。

**关键发现**:
- 🟥 CRITICAL: 0
- 🟧 HIGH: 0
- 🟨 MEDIUM: 3（行尾符、误提交、声称未做）
- 🟩 LOW: 3

**编译与测试**:
- ✅ 1084/1084 单元测试通过
- ✅ 13/13 .bas 通过（抽查 9 个无回归）
- ✅ 缩进修复实测确认
- ⚠️ gwsts.c CRLF（其他 40+ 文件 LF）— 行尾符不一致扩大
- ⚠️ gwsts.c.patch 入库

**问题追踪 (v50→v58)**:

| 问题 | 首次报告 | v58 状态 |
|------|----------|----------|
| 行尾符转换噪音 | v51 (dskcom) | **v58 复发 (gwsts.c)** |
| 误提交工具文件 | v54 (.qwen) | **v58 复发 (patch)** |
| commit message 不实 | v56 (换行) | **v58 复发 (test_print_file)** |
| test_print_file 无效 | v57 MEDIUM-1 | **遗留** |
| FOR 步长截断 / PRINT 泄漏 | v54 / v50 | **遗留** |

**建议**: 三个 MEDIUM 都是流程性问题（一行修复级别）：转 LF、删 patch、补 test_print_file。修复后本提交可保留。**需要注意重复模式**：行尾符、误提交、commit 声称不实已在多个版本反复出现 — 建议添加提交前检查清单。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-17
**审查版本**: v58 — fix: CODE_REVIEW.md v57 MEDIUM issues