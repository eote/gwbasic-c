# GW-BASIC C Port - Code Review v145

**日期**: 2026-10-23
**审查范围**: commits `9cb0fa4`（v144 CRITICAL-1 修复：图形 GET/PUT 标准数值数组语义）+ `0ee5da5`（FOR 首次迭代检查 + STEP 0 哨兵）→ 工作区
**测试状态**: 单元测试 **3238/3238 通过**（0 失败，0 警告）✅ | .bas 集成 **16 个受影响/回归用例直接运行**（输出与 expected 逐行一致）✅ | 其余 60 个用例本批代码路径无交集（单测全量覆盖）⏸
**工作区状态**: clean
**总体评分**: A- — v144 CRITICAL-1 修复确认（4 项 probe 全绿）+ FOR 常规语义修复正确；发现 **1 MEDIUM**（FOR 0 次 + NEXT 为最后一行边界）及观察项

---

## 一、概述

本次审查两个修复提交：

| 提交 | 内容 |
|------|------|
| `9cb0fa4` | **v144 CRITICAL-1 修复**：图形 GET/PUT 改为标准数值数组语义（ASM GPUTG；头 2 元素 x-bits/y-count、位图 MSB-first 连续、容量不足/字符串 → FCERR、隐式 DIM、GET 覆写语义）；`array_desc_t` 移入 gwbasic.h、`biptrg_ensure_array` 导出 |
| `0ee5da5` | **FOR 首次迭代检查**（ASM FOR→NEXTS：初值超限 → 0 次循环、变量保持初值、扫描匹配 NEXT 跳转）+ **STEP 0 哨兵**（NT0STP：sign=2 → ANSI 无限循环语义） |

### v144 CRITICAL-1 修复确认（全部验证通过）

| 验证项（probe，已清理） | 结果 |
|--------------------------|------|
| DIM 数组 GET/PUT roundtrip + XOR 擦除 | ✅ `DIM G(100)`：`C= 1 D= 1`；二次 PUT `XOR= 0` |
| 数组容量不足 | ✅ `DIM G(2)` + 9×9 GET → `Error at line 40`（FCERR） |
| 隐式 DIM（未声明数组） | ✅ `GET (38,38)-(46,46), H` → PUT → `C= 1` |
| 字符串变量参数 | ✅ `GET ..., S$` → `Error at line 30`（FCERR，S$ 未被破坏） |

### v145 FOR 修复验证

| 验证项（probe） | 结果 |
|------------------|------|
| 0 次循环（初值超限，正常 NEXT） | ✅ `FOR I=1 TO 0` → 体不执行，`AFTER I= 1`（初值保持） |
| 正常循环 / 负步长 | ✅ `I=1..3`；`J=3,2,1`；`DONE` |
| **0 次循环 + NEXT 为最后一行** | ❌ **MEDIUM-1 实证**：`BODY` 被错误执行一次 + `Error at line 30` |
| 紧凑 `FORX=1 TO 2` | ⚠️ 不支持（既有语句分派要求 `"FOR "`，非本批引入） |

### 验证结果（独立执行，当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3238/3238 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 受影响/回归（直接运行 + expected） | ✅ **16/16 一致**：gfx_getput（4/4，新增）、gfx_circle（9/9）、for_edges（13/13，新增）、graphics（11/11）、gfx_v141（7/7）、arrays（11/11）、multi_array、string_arrays、put_get（5/5）、line_input（3/3）、delete、delete_exec、renum、if_else、string_funcs、on_goto |

---

## 二、问题清单

### 🟥 CRITICAL

无（v144 CRITICAL-1 已修复确认）。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **FOR 0 次循环 + NEXT 为程序最后一行时，循环体被错误执行一次（随后 NEXT without FOR）** | `src/next86.c`（`next86_scan_matching_next` / 首次迭代返回路径） |

**详情**:

```basic
10 FOR I=1 TO 0
20 PRINT "BODY"
30 NEXT I
```
实测：`BODY` / `Error at line 30`（GW-BASIC 期望：体不执行、程序正常结束）。

**根因**：首次迭代判定 `first_done` 为真 → 弹栈并调用 `next86_scan_matching_next(g_curlin)`；匹配 NEXT 是最后一行时 `after == NULL` → 返回 **0**；gwmain 将 `rc == 0` 视为"成功继续" → 执行 FOR 的下一行（循环体）→ BODY；随后 `NEXT I` 因 FOR 已弹栈 → NEXT without FOR 错误。

**修复建议**：无后续行时返回一个"跳到程序末尾"的明确信号（例如返回 `-2`/END 语义，或返回一个不存在行号的 sentinel 并让 gwmain 特判），不得用 0 表示跳过；补单测（NEXT 最后一行 0 次循环）与 .bas 用例。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项（不计问题）：

1. **PUT 缺容量校验**：按数组 header 的 width/height 读取时不核对 `desc->size`；若 header 被用户改写 → 越界读（ASAN 可捕获）。建议 PUT 也按 header 计算 need_bytes 并与容量比对 → FCERR。
2. 紧凑 `FORI=...` 不支持（语句分派要求 `"FOR "`，既有兼容性缺口；`next86_scan_matching_next` 的 `"FOR "` 判定与语言支持一致）。
3. GET/PUT 元素大小归一 4 字节（ASM 依数组元素大小；INTEGER 数组在 ASM 为 2 字节 → 跨格式兼容需确认）。
4. STEP 0 按 ANSI 进入无限循环（NT0STP 语义，符合规范；用户程序会挂起）。
5. GET/PUT header 为 x-bits/y-count（LE）内部自洽；与 `BSAVE/LOAD` 数组文件格式的跨工具兼容未验证。
6. v144 观察项（CIRCLE 缺 radius、SCREEN 0 下 PSET、PAINT 默认 border、行内冒号）仍开放。

---

## 三、架构评估

### 修复质量

| 提交 / 决策 | 评价 |
|-------------|------|
| GET/PUT 改用数值数组元素（header + MSB-first 位图，通过 desc->data） | ✅ 正确 — DIM roundtrip/XOR/容量/隐式/字符串 4 项 probe 全绿 |
| 容量检查（need_bytes > size*elem_size → FCERR） | ✅ 正确（DIM G(2) 实证） |
| GET 覆写语义（memset 位图区） | ✅ 符合 ASM「GET overwrites」 |
| `biptrg_ensure_array` 导出 + `array_desc_t` 共享 | ✅ 消除跨模块重复定义，布局一致 |
| PRESET 动作按位取反 `(1<<bpp)-1-pixel` | ✅ 合理（ASM complement） |
| FOR 首次迭代（0 次体、初值保持、扫描匹配 NEXT） | ✅ 常规场景正确（probe + for_edges 13/13）|
| STEP 0 哨兵（sign=2，ANSI 无限） | ✅ 符合 NT0STP |
| **NEXT 最后一行返回 0** | ⚠️ **MEDIUM-1** — 0 与"继续"歧义 |
| 测试（test_gwsts_gfx2 +90、test_next86_firstiter 114、gfx_getput/for_edges 集成） | ✅ 质量高；缺 NEXT 最后一行边界用例 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| GET/PUT DIM/隐式/容量/字符串 | ✅ 单测 + probe + gfx_getput |
| FOR 0 次（正常 NEXT）/ 正常 / 负步长 | ✅ 单测 + probe + for_edges |
| **FOR 0 次 + NEXT 最后一行** | ❌ 盲区 → MEDIUM-1 |
| FOR 嵌套（深度扫描） | ✅ for_edges 含嵌套场景 |
| 注册一致性 | ✅（单测 3238 全过） |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：`next86_scan_matching_next` 无后续行时返回明确的程序末尾信号（非 0）；补 NEXT 最后一行 0 次循环单测 + .bas
2. 观察项 1：PUT 增加按 header 的容量校验（FCERR）

### 中期（M2 规划）
3. GET/PUT 与 BSAVE/LOAD 数组格式的兼容性验证（如后续实现）
4. v144 图形观察项（CIRCLE radius、SCREEN 0 图形语句）随批次处理
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
6. ASAN 构建（PUT 越界读/描述符类问题）
7. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: 两个修复提交质量高 — v144 CRITICAL-1（GET/PUT 标准数值数组）4 项 probe 全部修复确认（DIM roundtrip/XOR、容量 FCERR、隐式 DIM、字符串 FCERR），FOR 首次迭代常规语义正确（0 次体/初值保持/负步长）、STEP 0 哨兵符合 ANSI；单测 3238/3238、16 个 .bas 直接验证一致、0 警告。**但发现 1 MEDIUM**：FOR 0 次 + NEXT 为程序最后一行时，扫描返回 0 被当作"继续"→ 体错误执行一次 + NEXT without FOR。
**关键发现**: 0 CRITICAL / 0 HIGH / **1 MEDIUM**（FOR 边界）、6 观察项。
**建议**: 修复 NEXT 最后一行边界（返回明确末尾信号）+ PUT 容量校验；修复后进行 v146 review。

---

## 六、指标追踪

| 指标 | v144 | v145 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3223 | **3238** | ✅ +15（+测试函数） |
| 定义数（唯一函数） | 846 | **851** | +5 |
| 注册数（run_test 调用） | 846 | **851** | +5 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 16/16 受影响 | **16/16 受影响/回归一致**（新增 gfx_getput/for_edges） | 保持 |
| 图形 GET/PUT（DIM 数组） | ❌ CRITICAL-1 | ✅ **修复**（4 probe 全绿） | 关闭 |
| FOR 首次迭代（常规） | ❌ 缺失 | ✅ **修复**（0 次/初值/负步长） | 完成 |
| **FOR 0 次 + NEXT 最后一行** | — | ❌ **MEDIUM-1** | 新发现 |
| M2 进度 | 2/3 + 图形深化批 | **2/3 + 修复轮** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3238 → 16 个 .bas 直接运行 + expected 对比 → 8 项 probe（已清理）
- 全程未使用脚本（逐条直接运行命令 + 内置工具）
- 其余 60 个 .bas 与本批代码路径（advgrp/biptrg/next86）无交集，底层由 3238 单测全量覆盖
- **M2 状态**: 图形 GET/PUT 与 FOR 语义修复；MEDIUM-1 修复后进行 v146