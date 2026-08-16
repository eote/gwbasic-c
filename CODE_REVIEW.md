# GW-BASIC C Port - Code Review v54

**日期**: 2026-08-17
**审查范围**: commit `df7b504` → `d2f8e08` — feat: FOR/NEXT, GOTO/GOSUB/RETURN, PRINT semicolon/comma support
**测试状态**: 1084/1084 单元测试全部通过 ✅ | .bas 集成测试部分通过 ⚠️
**编译状态**: 无错误 ✅
**进度**: 44/44 模块 (100%)
**总体评分**: C+ (及格) — 功能方向正确但跳转机制存在严重缺陷，未识别语句静默终止

---

## 一、概述

本次提交 `d2f8e08` 是功能重做第二次迭代（v53 → FOR/NEXT + GOTO/GOSUB/RETURN + PRINT 分隔符）：

| 文件 | 变更 | 内容 |
|------|------|------|
| `src/next86.c` | +291 | FOR/NEXT 完整实现（FOR 栈、int/float 分支）、GOTO/GOSUB/RETURN 处理器 |
| `src/gwsts.c` | +113 | PRINT 多表达式/逗号分号分隔、END 语句、OPEN 文件、IF 简化 |
| `src/gwlist.c` | +120 | `gwlist_find_line_by_text_ptr()`、load_file 动态缓冲+strtol ✅ |
| `src/gwmain.c` | +55/-27 | 跳转处理循环（rc>0 跳转）、exec_rc、RUN 块缩进修复 |
| `src/gwbasic.h` | +21 | `struct for_entry` 提为公共结构、next86 API 变更 |
| `tests/bas/` | +23 | 新增 11 个 test_let*.bas + test_simple.bas |
| `.qwen/*` `err.txt` | 删除 | ✅ v53 MEDIUM-4 清理完成 |

**测试声明 vs 实测:**
- ✅ 1084/1084 单元测试通过
- ✅ commit message 声称 "9 .bas integration tests pass" — **与实测不符**（见下方）

**实测结果（本审查独立验证）:**

| .bas 测试 | commit 声称 | 实测 | 说明 |
|-----------|------------|------|------|
| test_hello.bas | 通过 | ✅ 通过 | PRINT |
| test_for.bas | 通过 | ✅ 通过 | 5 次 "Hello, World" 循环 |
| test_if.bas | 通过 | ✅ 通过 | A>3 / A<10 / A=5 |
| **test_goto_gosub.bas** | **通过** | **❌ 失败** | "Error at line 20"（GOTO 50 跳转失败）|
| 未识别语句测试 (自建) | — | ❌ 静默终止 | 无输出，exit 0，后续行不执行 |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | 指针截断跳转机制 — GOTO/GOSUB/FOR 跳转不可靠 | `src/next86.c` `src/gwlist.c` `src/gwmain.c` | 355/360, 99-104, 250 |
| 2 | 未识别语句被当作 END 静默终止程序 | `src/gwsts.c` `src/gwmain.c` | 35, 258-263 |

**详情:**

#### CRITICAL-1: 指针截断跳转机制（实测 test_goto_gosub.bas 失败）

```c
/* next86.c GOTO 处理 */
return (int)(uintptr_t)target->text;        /* ① 64位指针截断为32位int */

/* gwmain.c 跳转处理 */
if (rc > 0) {
    curr = gwlist_find_line_by_text_ptr((WORD)rc);   /* ② 再截断为16位 */

/* gwlist.c 匹配 */
if (curr->text && (WORD)(uintptr_t)curr->text == text_ptr)  /* ③ 16位截断比较 */
```

**问题链:**
1. **① (int) 截断**: 64 位进程堆地址 > 4GB 的指针低 32 位最高位为 1 时 → 负数 → main `rc > 0` 判定失败 → **明明成功的 GOTO 被当作错误**
2. **②③ WORD 16 位截断**: 两个不同行 text 指针的低 16 位碰撞 → 跳到错误行
3. **实测证据**: `test_goto_gosub.bas` line 20 (`GOTO 50`) 输出 "Error at line 20"，exit code 4294967295(-1) — **commit message 声称该测试通过，与实际不符**

**修复建议（正确设计）:** 跳转目标用**行号**传递，main 用 `gwlist_find_line(int)` 查找：
```c
/* next86_goto 返回目标行号 */
return target->number;        /* 或定义跳转约定：正数=行号 */

/* main 中 */
if (rc > 0) {
    curr = gwlist_find_line(rc);   /* 按行号查找，无指针截断 */
    continue;
}
```
或定义显式跳转结构（{type: GOTO/FOR/GOSUB, line: N}）。

---

#### CRITICAL-2: 未识别语句被当作 END 静默终止

```c
/* gwsts.c — 无匹配分支的语句返回 ERROR(-1) */
int result = ERROR;   /* line 35: 默认值，未识别语句保持 -1 */

/* gwmain.c — ERROR(-1) 被当作 END */
if (rc == -1) {
    /* END - stop execution gracefully */
    break;            /* ← 未识别语句在这里被静默吞掉！ */
}
```

**实测证据:** 自建 `t_unknown.bas`：
```
10 XXYYZZ
20 PRINT "SHOULD NOT REACH"
```
运行结果：**无任何输出、exit 0**。line 10 未识别 → ERROR(-1) → 被当 END 静默终止，line 20 从未执行。

**影响:** 任何拼写错误、未实现语句、语法错误都会导致程序**静默终止且退出码为 0 (成功)** — 用户和 CI 都无法察觉执行失败。同时与 CRITICAL-1 的 GOTO 负值错误互斥混淆。

**根因:** END 用 `ERROR`(-1) 作为"停止信号"与真实错误共享同一返回值，无区分。

**修复建议:**
```c
/* gwsts.c — END 返回专属值 */
else if (starts_with(trimmed, "END")) {
    result = -2;   /* 专属 END 信号，与 ERROR(-1) 区分 */
}
/* 未识别语句：打印错误并返回 ERROR */
/* == 在所有 if/else if 之后 == */
else {
    printf("Syntax error: %s\n", trimmed);   /* 至少输出错误 */
    result = ERROR;
}

/* gwmain.c — 区分处理 */
if (rc == -2) { break; }        /* END */
if (rc < 0) { printf(...); exec_rc = ERROR; break; }  /* 真实错误 */
```

---

### 🟧 HIGH

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | NEXT 省略变量名（纯 "NEXT"）不支持 — 静默终止 | `src/gwsts.c` `src/next86.c` | 134, 121 |

**详情:**

#### HIGH-1: "NEXT"（无变量名）无法工作

GW-BASIC 允许 `NEXT` 省略变量名（匹配最近的 FOR，是常用惯用法）。当前实现：

```c
/* gwsts.c — 只匹配 "NEXT "（带空格），纯 "NEXT" 走默认分支 */
else if (starts_with(trimmed, "NEXT ")) { ... }

/* next86.c — "NEXT " + 空变量名 → find_for("") 无匹配 → -1 */
struct for_entry *entry = next86_find_for(var_name, g_nxttxt);
if (!entry) {
    return -1;  /* NEXT without FOR error */
}
```

两条路径都返回 -1 → 被 CRITICAL-2 的 END 逻辑吞掉 → **程序静默终止**。

**修复建议:** `starts_with(trimmed, "NEXT")`（含无空格），`find_for` 对空变量名匹配栈顶 entry。

---

### 🟨 MEDIUM

| # | 描述 | 文件 | 行号 |
|---|------|------|------|
| 1 | FOR 初值/步长 WORD 截断 (`g_fvalsv = (WORD)start`, `(WORD)entry->step`) | `src/next86.c` | 327, 151, 168 |
| 2 | PRINT 字符串泄漏 — v50 HIGH-1 重现（未释放） | `src/gwsts.c` | 195, 213 |
| 3 | PRINT 用 printf 绕过 giocon — v50 MEDIUM 重现 | `src/gwsts.c` | 184, 213, 240 |
| 4 | `strcasecmp` 平台依赖（MSVC 无此函数） | `src/next86.c` | 118 |
| 5 | main() 函数内内联 `extern WORD g_curlin` | `src/gwmain.c` | 242 |
| 6 | `gwlist_find_line_by_text_ptr` 作为跳转 API — 设计随 CRITICAL-1 废弃 | `src/gwlist.c` | 99 |

**详情:**

#### MEDIUM-1: FOR 初值/步长 WORD 截断

```c
g_fvalsv = (WORD)start;          /* FOR X=1.5 → g_fvalsv=1（截断）*/
v->value.i += (WORD)entry->step; /* STEP 1.5 → +1（截断）*/
```

浮点 FOR 循环（`FOR X=1 TO 2 STEP 0.5`）的初值和整数步长处理会截断丢失精度。float 分支 `next86_float_next` 用 `entry->step`（float）✅ 正确，但 `g_nxtflg==0` 分支 `v->value.f = (float)g_fvalsv` 仍用截断初值。

#### MEDIUM-2: PRINT 字符串泄漏（v50 HIGH-1 重现）

```c
/* gwsts.c — console 路径 (line 195) */
char *str_val = gweval_eval_string(trim_start);
if (str_val) {
    printf("%s", str_val);   /* ← 未释放 */
}
/* 文件路径 (line 213) — 同样未释放 */
```

`gweval_eval_string` 返回 `gwdata_alloc_string` 的 bump 分配（16KB str_heap，free 为 no-op）。连续字符串 PRINT 会耗尽字符串堆 → 之后字符串输出静默变 "0"。**v50 已报告此问题（HIGH-1），本次提交重现。**

#### MEDIUM-4: strcasecmp 平台依赖

FOR 变量匹配用 `strcasecmp`（line 118）— MSVC（cl.exe）不提供此函数（只有 `_stricmp`），仅 MinGW 可用。项目应保持跨编译器兼容（v50 曾实现 `strcasecmp_ascii` 正是原因）。当前用 MinGW 编译不报错，但锁定了编译器选择。

---

### 🟩 LOW

| # | 描述 | 文件 |
|---|------|------|
| 1 | gwlist.c / next86.c / gwsts.c 三个文件末尾均无换行符 | 三个文件 EOF |
| 2 | `struct for_entry` 注释 "FOR token in high byte" 与实际不符（token 是 BYTE 值 0x81） | `src/gwbasic.h` 482 |
| 3 | `for_text_ptr` 实际存 FOR 行**下一行**的 text ptr，注释却写 "text pointer into FOR statement" | `src/next86.c` 316-322 |
| 4 | 空 .bas 文件返回 SUCCESS 无提示 | `src/gwlist.c` 158 |

---

## 三、架构评估

### 文件组织

功能迭代新增/修改 5 个核心源文件 + 12 个 .bas 测试，628 行净增。

### 设计决策评价

| 决策 | 评价 |
|------|------|
| FOR 栈 (MAX_FOR_DEPTH=16) | ✅ 合理 — 嵌套循环用栈天然匹配 |
| NEXT 跳回 FOR 行下一行（不重执行 FOR）| ✅ 正确 — 避免重复 push（**但依赖指针截断，见 CRITICAL-1**）|
| 返回值编码跳转 (rc>0=指针) | ❌ 错误 — 指针截断不可靠，应传行号 |
| END 用 ERROR 值标记 | ❌ 错误 — 与真实错误冲突（CRITICAL-2）|
| exec_rc 错误传播 | ✅ 正确（实测 exit -1 传播成功）|
| gwlist_load_file 动态缓冲 + strtol | ✅ 正确（v53 LOW-1/2 修复）|
| .qwen 误提交清理 | ✅ 正确 |
| RUN 块缩进修复 | ✅ 正确 |
| PRINT 逗号→tab / 分号→无分隔 | ✅ 语义正确 |
| getline_windows 动态扩容 | ✅ 正确 |

### 测试覆盖分析

| 指标 | 值 |
|------|-----|
| 单元测试 | ✅ 1084/1084 |
| .bas: hello / for / if | ✅ 实测通过 |
| .bas: goto_gosub | ❌ **实测失败**（commit 声称通过）|
| .bas: 错误路径（未识别语句）| ❌ 静默终止（无错误输出）|
| .bas: NEXT 无变量 | ❌ 静默终止 |
| .bas: 浮点 FOR (for3.bas STEP 0.5) | ⚠️ 未实测（MEDIUM-1 截断风险）|

**测试全面性评分: C+** — 成功路径部分可用，但关键错误路径（未识别语句、GOTO 跳转、NEXT 省略）无保护，且 .bas 验收声明与实际不符。

---

## 四、改进建议

### 短期 (当前 sprint) — 提交前必须修复

1. **🟥 CRITICAL-2 优先**: 修复 END 与 ERROR 返回值冲突 — 未识别语句必须报错而非静默停止（END 用 -2 专属值；未识别语句打印错误）
2. **🟥 CRITICAL-1**: 跳转改为行号传递（`return target->number` + `gwlist_find_line`），废弃指针截断机制
3. **🟧 HIGH-1**: 支持 `NEXT` 无变量名（匹配栈顶）

### 中期 (下个 sprint)

4. **🟨 MEDIUM-2**: PRINT 字符串释放（修复 v50 遗留的字符串堆耗尽）
5. **🟨 MEDIUM-1**: FOR 初值/步长用 float 全程（消除 WORD 截断）
6. **🟨 MEDIUM-3**: PRINT 改用 giocon_print_str/print_num
7. **🟨 MEDIUM-4**: 用 strcasecmp_ascii 替换 strcasecmp
8. 修正 commit message 的 .bas 测试声明与实测一致

### 长期

9. 将 .bas 集成测试接入自动化（含错误路径断言）
10. 统一行尾符与文件末尾换行（.gitattributes + 自动化检查）

---

## 五、总结

**总体评价**: 本次提交实现了解释器核心功能（FOR/NEXT、GOTO/GOSUB、PRINT 分隔符），方向正确，单元测试全部通过，test_for.bas / test_if.bas 实测通过。**但跳转机制（指针截断）和错误处理（END 与 ERROR 冲突）存在 CRITICAL 缺陷**，且 commit message 声称的 "9 .bas tests pass" 与实测不符（test_goto_gosub.bas 实际失败）。

**关键发现**:
- 🟥 CRITICAL: 2 (指针截断跳转、未识别语句静默终止)
- 🟧 HIGH: 1 (NEXT 无变量不支持)
- 🟨 MEDIUM: 6 (截断、字符串泄漏重现、giocon 绕过重现、strcasecmp、extern、API 设计)
- 🟩 LOW: 4

**编译与测试**:
- ✅ 1084/1084 单元测试通过
- ✅ test_for.bas: 5 次循环正确，exit 0
- ✅ test_if.bas: 条件分支正确
- ❌ test_goto_gosub.bas: "Error at line 20"（GOTO 跳转失败）
- ❌ 未识别语句: 静默终止（无错误、exit 0）
- ✅ exec_rc 错误传播实际工作（exit -1）

**v50→v54 问题追踪**:

| 问题 | v50 | v53 | v54 |
|------|-----|-----|-----|
| PRINT 字符串泄漏 (HIGH-1) | 报告 | 代码还原 | **重现** (MEDIUM-2) |
| printf 绕过 giocon | 报告 | 代码还原 | **重现** (MEDIUM-3) |
| 跳转机制 | — | — | **CRITICAL-1 (新)** |
| 未识别语句静默 | — | — | **CRITICAL-2 (新)** |
| 退出码错误 | — | ✅ 修复 | — |
| 调试输出残留 | — | ✅ 修复 | — |
| RUN 块缩进 | — | ✅ 修复 | — |
| 误提交文件 | — | ✅ 修复 | — |

**建议**: 先修复 CRITICAL-2（错误静默吞掉 — 最危险的隐蔽性缺陷）和 CRITICAL-1（跳转不可靠），再提交下一批功能迭代。当前状态不建议合并到 master 作为稳定基线（但若作为开发分支继续迭代可接受）。

---

**审查员**: AI Code Reviewer (gwbasic-code-review skill)
**审查日期**: 2026-08-17
**审查版本**: v54 — feat: FOR/NEXT, GOTO/GOSUB/RETURN, PRINT semicolon/comma support