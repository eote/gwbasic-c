# GW-BASIC C Port - Code Review v144

**日期**: 2026-10-22
**审查范围**: commit `c63ecf0`（feat: M2 图形深化 — CIRCLE/PAINT/GET/PUT 语句接线 + 弧线点计数 + fb 统一根因 + 图形 GET/PUT 分派）→ 工作区
**测试状态**: 单元测试 **3223/3223 通过**（0 失败，0 警告）✅ | .bas 集成 **16 个受影响/关键回归用例直接运行**（输出与 expected 逐行一致，含新增 test_gfx_circle）✅ | 其余 60 个用例本批代码路径无交集（单测全量覆盖）⏸
**工作区状态**: clean
**总体评分**: C — CIRCLE/PAINT/fb 统一/分派实现良好（16 个 .bas 直接验证一致），但发现 **1 个 CRITICAL**（图形 GET/PUT 对标准 DIM 数值数组破坏数组描述符/数据）

---

## 一、概述

本次审查 M2 图形深化提交 `c63ecf0`（11 文件，+647/-52）：

| 变更 | 说明 |
|------|------|
| CIRCLE 语句 | `CIRCLE (x,y),r[,color[,start[,end[,aspect]]]]`（ASM ADVGRP CIRCLE L460；弧度、负角画半径线、aspect 缩放） |
| PAINT 语句 | `PAINT (x,y)[,fill[,border]]`（ASM PAINT L121；fill/border 默认 -1） |
| 图形 GET/PUT | `GET (x1,y1)-(x2,y2),arr` / `PUT (x,y),arr[,action]`（action：PSET/PRESET/AND/OR/XOR，默认 XOR；文件形态 `GET #n` / `PUT #n` 保持分派） |
| fb 统一根因 | `gengrp_set_screen_mode` 同步 `g_fb_width/height`（0:80×25 / 1:320×200 / 2:640×200）；新增 `gengrp_fb_clear()`（测试隔离） |
| advgrp 实现 | CIRCLE/PAINT/GET/PUT 主体（+159） |
| 新测试 | `test_gwsts_gfx2.c`（6 测试）+ `test_gfx_circle.bas/expected` 集成 |

### 验证结果（独立执行，当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 增量重建 + 单元测试 | ✅ **3223/3223 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 受影响/关键回归（直接运行 + expected） | ✅ **16/16 一致**：gfx_circle（9/9）、arrays（11/11）、multi_array、string_arrays、graphics（11/11）、gfx_v141（7/7）、put_get（5/5）、file_seq（5/5）、file_rand（5/5）、line_input（3/3）、delete、delete_exec、renum、if_else（6/6）、string_funcs（10/10）、on_goto（3/3） |
| probe：CIRCLE 全圆/弧 | ✅ 单测 + gfx_circle 一致（E/W/N/S、IN/OUT、PP、ARCN/ARCS） |
| probe：PAINT 区域 | ✅ 单测（圆内/边缘/外部）＋ 默认 fill `P=1` |
| probe：GET/PUT 私有路径 | ✅ 未 DIM 数组 roundtrip + XOR 默认擦除 + PSET 动作（单测） |
| probe：**DIM 数值数组 GET/PUT** | ❌ **CRITICAL-1 实证**（见下） |
| probe：GET 到字符串变量 | ❌ `S$=` 被破坏（CRITICAL-1 同根） |
| probe：CIRCLE 缺 radius / SCREEN 0 下 PSET | ⚠️ 静默（观察项） |

---

## 二、问题清单

### 🟥 CRITICAL

| # | 描述 | 文件 |
|---|------|------|
| 1 | **图形 GET/PUT 对标准 `DIM` 数值数组破坏数组描述符与数据**：GET 向数组变量 `value.s.ptr`（=数组描述符指针）写入 header+位图，覆写 `dim_count/dims/array_data`；PUT 拒绝数值数组（`type != VT_STRING → ERROR`） | `src/advgrp.c`（L275-461）、`src/biptrg.c`（L84/L138） |

**详情**:

**实证 1**（数组数据被破坏）：
```basic
10 DIM G(100)
20 G(1) = 999
30 SCREEN 1
40 GET (0,0)-(3,3), G
50 PRINT "G1="; G(1)
```
实测：`G1= 0`（原值 999 被破坏）→ GET 把 header+像素写进了**数组描述符**（`biptrg.c:84/138` 将 `value.s.ptr` 设为 `desc` 指针），覆写 `dim_count/dims[]/array_data` → 数组数据访问失效，`array_data` 指针可被污染（野指针风险 UB）。

**实证 2**（标准 PUT 失败）：
```basic
10 DIM G(100): SCREEN 1: LINE (40,40)-(44,44),1,BF
40 GET (38,38)-(46,46), G
60 PUT (60,60), G
```
实测：`Error at line 60`（`advgrp_put` 要求 `VT_STRING`，数值数组被拒绝）→ GW-BASIC 标准 `DIM` + `GET`/`PUT` 用法失效。

**实证 3**（字符串变量被破坏）：
```basic
10 S$ = "ABC": SCREEN 1
30 GET (0,0)-(3,3), S$
40 PRINT "S$="; S$
```
实测：`S$=`（原 "ABC" 被覆盖）→ 应报类型错误（GET 只接受数值数组）。

**根因**：实现以"字符串变量"作为 GET/PUT 的私有存储格式；未实现 ASM 的**数值数组存储语义**（头 2 元素 width/height + 位图按整数元素打包，MSB-first）。`DIM` 数组的 `value.s.ptr` 指向数组描述符（非像素缓冲），直接作为写入缓冲 → 描述符覆写；PUT 对 `VT_STRING` 的硬性检查拒绝合法数组。

**测试盲区**：6 个新单测的 GET/PUT 用例全部使用**未 DIM** 的 `G`（走"创建字符串变量"私有路径）；`test_gfx_circle.bas` 只测 CIRCLE/PAINT → 标准 DIM 路径从未被覆盖。

**修复建议**：
1. GET/PUT 改为**数值数组元素**存取（通过 biptrg 数组 API）：头 2 元素 `width/height`，随后按 ASM 位图打包（模式 1：2 bits/像素、模式 2：1 bit/像素，MSB-first，跨行连续）
2. 容量检查：数组元素不足 → FCERR/Illegal function call（不得越界写）
3. 字符串变量 → 类型错误（SNERR/FCERR），不得覆盖
4. 补测试：`DIM` 数组 roundtrip（GET→PUT 验像素）、数组不足、字符串变量拒绝、隐式数组（0..10）行为
5. 若保留私有字符串格式作为扩展，需显式类型检查并禁止数值数组进入该路径

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞）。观察项（不计问题）：

1. **CIRCLE 缺 radius**（`CIRCLE (50,50)`）→ 静默 radius=0（应语法错误）
2. **SCREEN 0 下 PSET** 静默成功（应 Illegal function call；GET 在 mode 0 已正确 return ERROR）
3. **PAINT 默认 fill** 实测 `P=1`（FORCLR；待 ASM 对照确认默认 border 语义）
4. 行内冒号多语句（`... : PRINT`）为全局既有执行模型限制（非本批引入）
5. GET/PUT 数组名后尾随垃圾被忽略

---

## 三、架构评估

### 图形深化质量（c63ecf0）

| 决策 | 评价 |
|------|------|
| CIRCLE 解析（共享 coord_pair + 逐级可选参数）+ advgrp 弧线实现 | ✅ 正确（单测 + gfx_circle 9/9） |
| PAINT 解析 + 区域填充/边界 | ✅ 正确（单测） |
| GET/PUT 语句分派（`(` 图形 / `#` 文件）+ 文件路径回归 | ✅ 正确（test_put_get/file_seq/file_rand/line_input 全过） |
| PUT action（PSET/PRESET/AND/OR/XOR，默认 XOR） | ✅ 正确（XOR 擦除单测实证） |
| fb 统一（`gengrp_set_screen_mode` 同步尺寸） | ✅ 正确根因修复（图形边界检查依赖 fb 尺寸） |
| `gengrp_fb_clear`（测试隔离） | ✅ 合理 |
| **GET/PUT 存储模型（字符串变量私有格式）** | ❌ **CRITICAL-1** — 与 ASM 数值数组语义不符，破坏 DIM 数组 |
| 6 新单测 | ✅ 隔离良好；❌ GET/PUT 只覆盖未 DIM 私有路径（盲区） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| CIRCLE 全圆/弧/端点 | ✅ 单测 + .bas |
| PAINT 区域/边界 | ✅ 单测 |
| GET/PUT 私有（未 DIM）roundtrip / XOR / PSET | ✅ 单测 |
| 文件 GET/PUT 分派回归 | ✅ 单测 + .bas |
| **DIM 数值数组 GET/PUT（标准语法）** | ❌ **盲区 → CRITICAL-1** |
| 字符串变量参数拒绝 | ❌ 盲区（实测被破坏） |
| 数组容量不足 | ❌ 盲区 |

---

## 四、改进建议

### 短期（阻塞项）
1. **CRITICAL-1**：按 ASM 数值数组语义重写 GET/PUT 存储（2 元素头 + 位图整数打包 MSB-first）；容量检查；字符串变量类型错误；补 DIM roundtrip/不足/拒绝测试
2. 观察项 1/2：CIRCLE 缺 radius、SCREEN 0 图形语句 → 语法/FCERR

### 中期（M2 规划）
3. .bas 集成补 `DIM` 数组 GET/PUT 场景（含大块跨行打包）
4. PAINT 默认 fill/border 与 ASM 对照
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 套件回归）

### 长期
6. ASAN 构建（GET/PUT 描述符覆写类问题可被捕获）
7. 定期 .bas 集成回归

---

## 五、总结

**总体评价**: CIRCLE/PAINT/GET/PUT 语句接线与 fb 统一根因质量良好 — CIRCLE 全圆/弧、PAINT 区域、GET/PUT 文件分派、action 默认 XOR 全部正确（16 个 .bas 直接验证一致，含新增 test_gfx_circle）；单测 3223/3223、0 警告。**但图形 GET/PUT 的存储模型存在 CRITICAL 缺陷**：对 GW-BASIC 标准 `DIM` 数值数组，GET 覆写数组描述符（`G(1)` 被破坏、`array_data` 野指针风险），PUT 直接拒绝（`Error`）；字符串变量参数也被破坏。测试仅覆盖未 DIM 的私有路径。
**关键发现**: **1 CRITICAL**（GET/PUT 存储模型/数组描述符破坏）、0 HIGH、0 MEDIUM、5 观察项。
**建议**: 按 ASM 数值数组语义重写 GET/PUT（含容量检查与类型错误），补 DIM 路径测试；修复后进行 v145 review。

---

## 六、指标追踪

| 指标 | v143 | v144 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3199 | **3223** | ✅ +24（+6 测试函数） |
| 定义数（唯一函数） | 840 | **846** | +6 |
| 注册数（run_test 调用） | 840 | **846** | +6 |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 75/75 基线 | **16/16 受影响/关键回归直接验证一致**（其余 60 无交集） | 新增 test_gfx_circle |
| CIRCLE / PAINT | ⏳ 未实现 | ✅ 实现（单测 + 9/9 .bas） | 完成 |
| fb 统一（SCREEN 尺寸同步） | 缺陷根因 | ✅ 修复 | 完成 |
| **图形 GET/PUT（DIM 数组）** | — | ❌ **CRITICAL-1** | 新发现 |
| M2 进度 | 2/3 + 图形修复轮 | **2/3 + 图形深化批** | 推进 |

**注释**:
- 独立验证：增量重建 → 单测 3223 → 16 个 .bas 直接运行 + expected 对比 → 6 项 probe（已清理）
- 按用户要求全程未使用脚本（含内联循环/批量脚本）；.bas 验证采用逐条直接运行
- 其余 60 个 .bas 与本批代码路径（advgrp/gengrp/CIRCLE/PAINT/GET/PUT 分派）无交集，底层逻辑由 3223 单测全量覆盖
- **M2 状态**: 图形深化批完成但 GET/PUT 存储模型需重做；修复后进行 v145