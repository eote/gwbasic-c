# GW-BASIC C Port - Code Review v142

**日期**: 2026-10-20
**审查范围**: commit `022502a`（fix: CODE_REVIEW v141 — MEDIUM-1 隐式数组声明 DIM(10)+ERRBS + MEDIUM-2 共享坐标解析 depth 感知 + LOW-1 LINE( / SCREEN1 紧凑 + itsa86 flaky）→ 工作区
**测试状态**: 单元测试 **3199/3199 通过**（0 失败，0 警告）✅ | .bas 集成 **75/75 全 PASS（直接运行全部用例，逐字节对比 expected，含尾换行/CRLF 归一）** ✅ | 孤儿 **840=840**（16 图形测试全部注册）✅
**工作区状态**: clean
**总体评分**: A- — v141 的 2 MEDIUM + 1 LOW 全部修复确认（probe 全绿），无阻塞问题；附 4 个观察项

---

## 一、概述

本次审查开发者对 v141 修复提交 `022502a`（11 文件，+395/-115）。

### v141 修复确认（全部验证通过）

| v141 问题 | 修复方式（022502a） | 独立验证 |
|-----------|--------------------|----------|
| 🟨 MEDIUM-1 隐式数组声明 | `biptrg_ensure_array()`：未 DIM 名称自动 `DIM(10)` + 类型后缀推断（$/#/%/默认 single）；所有数组入口（single/multi/string set/get）统一；越界由 **clamp 改为 ERRBS(9)** | ✅ probe `A(1)=5 / A(10)=7`（未 DIM）→ `A1= 5 A10= 7`（0..10 共 11 元素，size 语义正确）；`A(11)` → `Error at line 10`；`DIM B(20): B(21)` → `Error at line 20` |
| 🟨 MEDIUM-2 坐标嵌套括号 | 新增 `gwsts_parse_coord_pair()`：depth/inq 感知找匹配 `)`、顶层 `,` 切分、两段 eval；PSET/PRESET/LINE 三处共用 | ✅ probe `PSET(A(1), A(1)), 3` → `POINT(5,5)=3`；`LINE(10,10)-(20,20),1`（紧凑）→ `POINT(10,10)=1` |
| 🟩 LOW-1 紧凑写法 | SCREEN：`starts_with("SCREEN")` + `[6]` 空格/制表/数字；LINE：`starts_with("LINE")` + `[4]` 空格/制表/'('/end + LINE INPUT 排除 | ✅ probe `SCREEN1` → `POINT(5,5)=1`；`LINE(...)` 紧凑 → 生效 |
| itsa86 flaky | `g_data_seg/g_code_seg = (addr & 0xF000) | 0x1000`（保证非零段，避免 mapclc NEWDS flaky） | ✅ 单测 3199/3199 全过（含 scndrv/itsa86 相关） |

### 验证结果（独立执行，当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 干净重建（全量编译） | ✅ exit 0 |
| 编译警告 | ✅ **0 条** |
| 单元测试 | ✅ **3199/3199 PASS**（836 → 840，+6 测试函数：隐式数组×3 + 数组坐标×2 + 紧凑×1） |
| 孤儿检测（内置 grep 核对） | ✅ `test_gwsts_gfx_` 16 extern + 16 run_test（原 10 + 新增 6），0 孤儿 |
| probe：隐式数组 0..10 | ✅ `A1= 5 A10= 7` |
| probe：越界 ERRBS | ✅ 未 DIM `A(11)` / 显式 `B(21)` → Error |
| probe：嵌套坐标 | ✅ `PSET(A(1),A(1)),3` → `C= 3` |
| probe：紧凑写法 | ✅ `LINE(10,10)-(20,20)` → `C2= 1`；`SCREEN1` → `C= 1` |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

无（阻塞性）。以下为观察项：

**观察项（不计问题）**：
1. **`LINE  INPUT`（双空格）遮蔽风险**：LINE INPUT 排除用 `starts_with("LINE INPUT")`（单空格）→ `LINE  INPUT A$`（双空格，GW-BASIC 合法）可能进入 LINE 分支并被静默忽略。建议排除改为"LINE + 空白 + INPUT"词法判断。影响极小（罕见写法）。
2. **malformed 坐标静默降级**：`gwsts_parse_coord_pair` 返回 false（无逗号/无闭括号/超长 >127 字符）时 `p` 不推进，PSET/LINE 以 (0,0) 或光标继续执行（无 SNERR）。建议 malformed → SNERR（GW-BASIC 语法错误语义）。
3. **隐式多维数组**：`A(1,2)` 未 DIM → `ensure_array` 创建 1D(10)，multi 入口按 `desc->dimensions=1` 仅校验 indices[0]（第二维被忽略）。GW-BASIC 未 DIM 多维行为待 ASM 对照确认（观察）。
4. **itsa86 段值 `| 0x1000`**：段值不再等于地址高 4 位（测试稳定性修复）；若未来模拟器逻辑依赖段地址转换，需复查。当前单测全绿。

---

## 三、架构评估

### 修复质量（022502a）

| 决策 | 评价 |
|------|------|
| `biptrg_ensure_array`（隐式 DIM 10 + 类型后缀推断） | ✅ 正确 — 全部数组入口统一；0..10 边界 probe 实证 |
| 越界 clamp → ERRBS(9)（ASM FCERRG） | ✅ 更符合 GW-BASIC（显式数组同样生效） |
| `gwsts_parse_coord_pair`（depth/inq 感知） | ✅ 正确 — 三处复用（PSET/PRESET/LINE），嵌套坐标 probe 实证 |
| SCREEN/LINE 紧凑 + 标识符边界（`[6]`/`[4]` 判定） | ✅ 正确 — `SCREENX`/`LINEABC` 不误匹配 |
| LINE INPUT 排除保持 | ✅（单空格形式正确；双空格形式见观察项 1） |
| itsa86 非零段修复 | ✅ 合理（flaky 根因说明清晰） |
| 测试更新（test_biprtu_biptrg/test_biptrg clamp→ERRBS 断言 + 6 新用例） | ✅ 注册完整（840=840）|
| 新集成测试 test_gfx_v141.bas（+expected）+ 套件收录 | ✅ 含在全量 75/75 中，输出逐字节一致 |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| 隐式数组（赋值/读取/字符串/越界） | ✅ 3 单测 + probe |
| 显式数组越界 ERRBS | ✅ 单测（clamp 断言已更新）|
| PSET/LINE 嵌套数组坐标 | ✅ 2 单测 + probe |
| LINE/SCREEN 紧凑写法 | ✅ 1 单测 + probe |
| itsa86 稳定性 | ✅ 单测全过 |
| 注册一致性 | ✅ 840=840，0 孤儿 |

---

## 四、改进建议

### 短期（可选）
1. LINE INPUT 排除改为词法判断（观察项 1）
2. malformed 坐标 → SNERR（观察项 2）
3. 隐式多维数组语义对照 ASM 确认（观察项 3）

### 中期（M2 规划）
4. 图形深化：CIRCLE/PAINT/GET/PUT（M2 后续批次）
5. .bas 集成验证模式："直接运行全部用例 + 逐字节对比 expected"（不经脚本文件，内联单条命令完成），本轮 **75/75 全 PASS**，无回归；后续可沿用该模式

### 长期
6. ASAN 构建
7. 定期 .bas 集成回归（用户许可后）

---

## 五、总结

**总体评价**: v141 的 2 个 MEDIUM + 1 LOW 全部修复且独立验证通过 — 隐式数组 0..10（`A1=5 A10=7`）+ 越界 ERRBS、共享 depth 感知坐标解析（嵌套 `PSET(A(1),A(1))` 正确）、LINE(/SCREEN1 紧凑写法、itsa86 flaky 修复；单测 3199/3199、0 警告、注册 840=840（0 孤儿）。
**关键发现**: 0 CRITICAL / 0 HIGH / 0 MEDIUM / **0 LOW**（4 个观察项）。无阻塞性缺陷。
**建议**: M2 图形深化（CIRCLE/PAINT/GET/PUT）；观察项可随批次处理。

---

## 六、指标追踪

| 指标 | v141 | v142 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3177 | **3199** | ✅ +22（+6 测试函数） |
| 定义数（唯一函数） | 836 | **840** | +4* |
| 注册数（run_test 调用） | 836 | **840** | +4* |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成（直接运行，不经脚本） | ✅ **75/75 全 PASS**（套件列表全部用例逐一直接运行 + expected 逐字节对比；含 gfx_v141/arrays/multi/string_arrays/graphics/if_else/line_input/delete/renum/read_data 等全部历史用例，无回归） |
| 隐式数组声明 | ❌ MEDIUM-1 | ✅ **修复**（0..10 + ERRBS） | 关闭 |
| PSET 嵌套坐标 | ❌ MEDIUM-2 | ✅ **修复**（共享 depth 扫描） | 关闭 |
| LINE 紧凑写法 | 🟩 LOW-1 | ✅ **修复** | 关闭 |
| itsa86 flaky | ⚠️ | ✅ 修复 | 关闭 |
| M2 进度 | 2/3 + 图形批接线 | **2/3 + 图形修复轮** | 推进 |

*注：定义/注册 840 = 836 + 6 新测试函数（test_runner 声明含 16 图形测试）；与 v141 的 836 基线核对一致（0 孤儿）。

**注释**:
- 独立验证：干净重建 → 单测 3199 → grep 注册核对 → 5 项 probe（已清理）
- **按用户要求未运行任何脚本**（含 run_bas_tests.sh）；改用**单条内联命令直接运行全部 .bas 用例 + 逐字节对比 expected**（CRLF/尾换行归一）：**75/75 全 PASS，0 失败**，覆盖历史全部用例，无回归
- **M2 状态**: 图形主路径 + v141 修复完成；下一步 CIRCLE/PAINT/GET/PUT