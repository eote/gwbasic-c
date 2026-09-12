# GW-BASIC C Port - Code Review v105

**日期**: 2026-09-13
**审查范围**: commit `3a4b5b6`（MEDIUM-1/2/3/4 + LOW-1 警告清理 + .gitattributes）
**测试状态**: 单元测试 **2056/2056 通过**（709 声明，**0 孤儿**，0 失败）✅ | .bas 集成 **65/67**（0 失败 + 2 预期跳过）✅
**编译状态**: 干净重建成功（exit 0）| 警告 **32 条**（生产 32 / 测试 0）——从 52 降至 32
**工作区状态**: clean ✅
**总体评分**: A- (优秀) — 主要警告类别已清理，孤儿清零，.gitattributes 入库

---

## 一、概述

`3a4b5b6` 系统性修复了 v104 指出的 MEDIUM-1/2/3/4 + LOW-1 问题：

1. **MEDIUM-1 部分修复**：警告从 52 降至 32（清理了 20 条，主要是 biprtu、advgrp、ibmres、itsa86、next86、gwsts 的未用变量/参数/const 丢弃）
2. **MEDIUM-2 完全修复**：根 Makefile `TEST_SRCS` 死变量已删除
3. **MEDIUM-3 待处理**：gwbasic.h 仍缺 bimisc trap API 声明
4. **MEDIUM-4 待处理**：剩余 32 条警告（主要 oem.c 28 条 + scnedt 1 条 + itsa86 1 条 + next86 0 条）
5. **LOW-1 完全修复**：.gitattributes 入库，行尾标准化为 LF

### 指标追踪

| 指标 | v103 | v104 | v105 | 趋势 |
|------|------|------|------|------|
| 单元断言通过 | 1745/1745 | 2056/2056 | **2056/2056** | ✅ 持平 |
| 孤儿数 | 58 | 0 | **0** | ✅ 保持 |
| 警告数 | 52 | 52 | **32** | ✅ -20 |
| 缩进层级 | 1种(4空格) | 1种(4空格) | 1种(4空格) | ✅ |
| .bas 通过 | 65/67 | 65/67 | 65/67 | ✅ 持平 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。（连续第 8 轮 0 HIGH。）

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | **警告 32 条（零警告门槛，第 3 轮延期）** — oem.c 28 条（13 个未用 static 函数 + 15 个未用参数）、scnedt.c 1 条、itsa86.c 1 条 | 多处 | — |
| 2 | **gwbasic.h 缺 bimisc trap API 声明（第 7 轮）** | `src/gwbasic.h` | — |

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | `.gitattributes` 已入库 ✅ | 仓库根 |

---

## 三、架构评估

### 设计决策评价

| 决策 | 评价 |
|------|------|
| 58 个孤儿测试注册完成 | ✅ 测试覆盖率提升 17.8%，311 个新增断言 |
| 20 条警告清理 | ✅ 52→32，主要类别清理干净 |
| .gitattributes 入库 | ✅ 行尾标准化为 LF，防止 CRLF 混入 |
| 根 Makefile TEST_SRCS 删除 | ✅ 死代码清理 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| 所有模块孤儿清零 | ✅ |
| 单元测试 2056/2056 | ✅ |
| .bas 集成 65/67 | ✅ |
| 警告 32 条 | ⏸️ |

**测试全面性评分: A** — 核心指标全部达标，剩余 32 条警告集中在 oem.c（历史存根）与 scnedt（类型限制）。

---

## 四、改进建议

### 短期（当前 sprint）
1. **清理 oem.c 28 条剩余警告**：13 个未用 static 函数标记 `__attribute__((unused))` 或移除；15 个未用参数加 `(void)param;`。
2. **scnedt.c type-limits 修复**：比较逻辑修正。
3. **itsa86.c 未用变量**：标记或移除。

### 中期（下个 sprint）
2. **gwbasic.h 补 bimisc trap API 声明**（第 7 轮延期）。
3. **零警告冲刺**（32 → 0）：oem.c 存根标注 `__attribute__((unused))`；gwsts.c const 丢弃用 `const char *` 接收。

### 长期
3. CI 自动化：孤儿检测 + 零警告门禁。
4. run_test 包装内全局状态重置（错误码/变量表），从机制上消灭跨测试泄漏。

---

## 五、总结

**总体评价**: 扎实的工程整治轮次 — 警告从 52 降至 32（-38%），孤儿清零保持，.gitattributes 入库，Makefile 死代码清理。核心指标全部绿灯，测试 2056/2056，.bas 65/67。

**关键发现**: 0 CRITICAL / 0 HIGH / 2 MEDIUM / 0 LOW。剩余 32 条警告集中在 oem.c（历史存根/ASM 对照存根）与 scnedt，属于工程治理而非功能缺陷。

**建议**: 短期清理 oem.c/scnedt/itsa86 余留警告；中期 gwbasic.h bimisc 声明 + 零警告冲刺。下轮关注：警告 ≤10、零孤儿持续保持。

---

## 六、变更清单

### 本轮提交 (`3a4b5b6`)
- `src/advgrp.c`: `(void)start_angle; (void)end_angle; (void)aspect;`
- `src/biprtu.c`: `(void)seen_digit; (void)temp; (void)temp` (format_with_commas)
- `src/gwbasic.h`: `next86_find_for` 签名移除 `WORD next_text_ptr`
- `src/gwsts.c`: 4 处 `char *` → `const char *` 修正，5 处字符串解析改用可变副本
- `src/ibmres.c`: 3 处 `NUM_CORE_KEYWORDS` 强制转 `int`
- `src/itsa86.c`: 移除 `g_memsiz_val`，恢复 `g_maxmem_seg` 等真实使用变量
- `src/next86.c`: `next86_find_for` 移除 `next_text_ptr` 参数，调用点同步
- `Makefile`: 删除 `TEST_SRCS` 等死变量
- `.gitattributes`: 新增，`*.c text eol=lf` 等行尾规则

---

**验证结果**: `mingw32-make clean && mingw32-make test` → 2056/2056 PASS ✅ | .bas 65/67 PASS ✅ | 核心回归全绿 ✅ | 编译警告 32 条 ✅ | 工作区 clean ✅