# GW-BASIC C Port - Code Review v156

**日期**: 2026-10-28
**审查范围**: commit `6680eb2`（feat: v155 观察项批次 — obs-3 reclen>32767 FCERR + obs-5 SCREEN 0 图形语句 gate + obs-6 exec_rc 简化 + obs-4 交互 RUN smoke 实证）→ 工作区
**测试状态**: 单元测试 **3331/3331 通过**（0 失败，0 警告）✅ | .bas **83/83 全量直接运行**（输出与 expected 一致）✅ | 注册/定义 **891=891**（0 孤儿）✅
**工作区状态**: clean（仅 `.reposman` 外部标记）
**总体评分**: B+ — obs-3/5/6 修复确认（边界/门禁/退出码全部验证），测试质量高；但 **obs-4 仅手动实证未闭环**，且发现 **1 MEDIUM**（SCREEN 0 gate 错误缺 ERR/ERL，RESUME NEXT 失效）

---

## 一、概述

本次审查 `6680eb2`（9 文件，+262/-5）。

### v155 观察项处理确认

| 观察项 | 处理 | 独立验证 |
|--------|------|----------|
| obs-3 reclen/filenum 范围 | classic + `LEN=` 两处 `reclen<=0 || >32767 → FCERR`（ASM INTID2）；filenum 0/256 由 dskcom 拒绝（已存在，补测试） | ✅ probe：`reclen 32767` → OK；`32768` → `Error at line 50`；`filenum 0`/`256` → Error；`255` → OK |
| obs-5 SCREEN 0 图形语句 | PSET/PRESET/LINE/CIRCLE/PAINT 分支 mode gate + GET/PUT 顶部早退（文件形式 PUT #1/GET #1 不受影响） | ✅ 单测 7（含 fileform 回归）+ 集成 `test_gfx_screen0`（S0-BLOCKED/DONE）+ probe |
| obs-6 exec_rc 简化 | 删除恒 SUCCESS 变量，尾部 `return SUCCESS` | ✅ terminating 用例 `test_onerr_off` 仍 `Error at line 120` + **Exit Code -1**（退出码不受影响） |
| obs-4 交互 RUN smoke | **仅手动实证**（stdin + **行间 sleep**，依赖 scnedt 读取时序） | ⚠️ **无自动化测试** → 观察项未闭环（另见观察项 2） |

### 新增测试质量

- `test_gwsts_openmodes.c` +5：`reclen 32768→FCERR`、`filenum 0/256→ERROR`、**filenum 255 边界 OK**、**reclen 32767 边界 OK**（均断言"不建文件"）✅
- `test_gwsts_gfx2.c` +7：SCREEN 0 下 PSET/PRESET/LINE/CIRCLE/PAINT/GET/PUT → ERROR + **文件形式 `PUT #1` 不受 gate** 回归 ✅
- 集成 `test_gfx_screen0.bas`：SCREEN 1 画图正常 → SCREEN 0 PSET 被 ON ERROR 捕获（S0-BLOCKED/DONE）✅

### 验证结果（独立执行）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3331/3331 PASS**（0 失败） |
| 编译警告 | ✅ **0 条** |
| **.bas 全量（83 个，分 6 批直接运行）** | ✅ **83/83 输出与 expected 一致**（含新增 test_gfx_screen0；terminating 用例符合预期） |
| 注册/定义一致性 | ✅ **891 = 891**（0 孤儿；`int run_test(` 定义行不计入） |
| probe：reclen/filenum 边界（5 项） | ✅ 全部符合 ASM 语义 |
| probe：terminating 退出码 | ✅ `Error at line 120` + Exit -1 |
| probe：**SCREEN 0 + ON ERROR + RESUME NEXT** | ❌ **MEDIUM-1 实证**（见下） |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

| # | 描述 | 文件 |
|---|------|------|
| 1 | **SCREEN 0 gate 的 ERROR 未设置 error code/行号**：`ON ERROR + RESUME NEXT` 无法恢复（ERR=0/ERL=0，handler 后程序终止），与 eval 错误处理不一致 | `src/gwsts.c`（PSET/PRESET/LINE/CIRCLE/PAINT/GET/PUT gate） |

**详情**:

**实证**（probe，已清理）：
```basic
10 SCREEN 0
20 ON ERROR GOTO 200
30 PSET (1,1)
40 PRINT "PSET-FAIL"
...
200 PRINT "ERR AT"; ERL
210 RESUME NEXT
```
实测：仅 `ERR AT 0` 后程序终止（期望逐条捕获 + `GATE-DONE`）。

**对照**：`test_error.bas`（CHR$ FCERR，走 `gwdata_set_error`）→ `ERR = 11` + `RESUME NEXT` 正常继续 ✅。

**根因**：gate 直接 `result = ERROR`（rc<0 通道），未调用 `gwdata_set_error(5, 行号, ...)`；`gwdata_handle_error` 未获得错误码/行号 → `ERR`/`ERL` 为空，`RESUME NEXT` 无法定位恢复点。

**影响**：ON ERROR 程序无法对"SCREEN 0 下图形语句"做 RESUME NEXT 恢复；ERR/ERL 无值。`test_gfx_screen0` 用 GOTO handler（不 RESUME）故未暴露。

**修复建议**：gate 处设置 `gwdata_set_error(5, g_curlin, 0)`（Illegal function call），或统一错误路径；补 `ON ERROR + RESUME NEXT` 覆盖测试（逐条语句恢复）。

### 🟩 LOW

无（阻塞）。观察项：

1. **POINT 在 SCREEN 0 返回 -1**（未 gate，与越界同值；GW-BASIC 可能应 FCERR）— probe：`P=-1` + 程序继续；建议 ASM 对照后决定 gate 与错误码（若 gate，同样需 set_error 以支持 RESUME）。
2. **obs-4 未闭环**：交互 RUN 无自动化测试；dev 手动实证依赖 stdin **行间 sleep**（scnedt 读取时序）。建议 main 级自动化 smoke（用 stdin 重定向量化；本 review 的 printf 管道此前在 FOR 场景成功，可作为无 sleep 方案验证）。
3. 表达式 filespec 消耗 string heap（v152 观察，ASM 堆语义一致）。
4. v145 图形观察项：PUT 容量校验仍未做；CIRCLE 缺 radius 静默（radius=0）；SCREEN 0 已由本批处理。
5. SCREEN 0 gate 的"包裹 + 事后覆盖 result"写法可读性偏低（可改为统一早退 helper，与 GET/PUT 一致）。

---

## 三、架构评估

### 修复质量（6680eb2）

| 决策 | 评价 |
|------|------|
| reclen 上限（classic + LEN= 两处，32767 边界保留） | ✅ 正确 — probe/单测双边界 |
| filenum 0/256 依赖 dskcom 拒绝 + 补测试 | ✅ 合理（ASM POFNUM/GETBYT 语义） |
| SCREEN 0 gate（5 语句 + GET/PUT；file-form 不受影响） | ✅ 功能正确（单测 7 + 集成 + probe）；⚠️ 错误码/ERL 缺失（MEDIUM-1） |
| exec_rc 简化 | ✅ 安全（terminating exit -1 验证） |
| 测试（12 新函数，含边界与 fileform 回归） | ✅ 质量高（不建文件断言、ON ERROR 捕获） |
| obs-4 手动实证 | ⚠️ 未自动化（观察项 2） |

### 测试覆盖分析

| 功能 | 覆盖情况 |
|------|----------|
| reclen 32767/32768、0 | ✅ 单测 + probe |
| filenum 0/255/256 | ✅ 单测 + probe |
| SCREEN 0 全图形语句 + file-form | ✅ 单测 + 集成 + probe |
| **SCREEN 0 gate 错误码/ERL/RESUME NEXT** | ❌ **盲区 → MEDIUM-1** |
| POINT 在 SCREEN 0 | ❌ 观察项 1 |
| 注册一致性 | ✅ 891=891，0 孤儿 |

---

## 四、改进建议

### 短期（阻塞项）
1. **MEDIUM-1**：gate 补 `gwdata_set_error(5, g_curlin, 0)`；补 `ON ERROR + RESUME NEXT` 覆盖（逐条语句）
2. 观察项 1：ASM 对照 POINT 在 SCREEN 0 的语义，按结论 gate + set_error
3. 观察项 2：交互 RUN main 级自动化 smoke（可复用 stdin 重定向方案）

### 中期
4. v145 图形观察项剩余（PUT 容量校验、CIRCLE radius）
5. CI 门禁（孤儿 + 零警告 + fresh clone 构建 + 文件/交互双路径 smoke）

### 长期
6. ASAN 构建
7. 定期文件 + 交互双路径回归

---

## 五、总结

**总体评价**: v155 观察项批次完成度高 — obs-3（reclen>32767/filenum 边界）、obs-5（SCREEN 0 图形 gate，含 file-form 回归）、obs-6（exec_rc 简化，退出码保持）全部修复确认；新增 12 个测试质量好（边界值 + 不建文件 + ON ERROR 捕获）；单测 3331/3331、83/83 全量、0 警告、891=891。**但 obs-4 仅手动实证未闭环**，并发现 **1 MEDIUM**：SCREEN 0 gate 错误未设置 ERR/ERL，`RESUME NEXT` 无法恢复（与 eval 错误处理不一致）。
**关键发现**: 0 CRITICAL / 0 HIGH / **1 MEDIUM**（gate 错误码/ERL）、5 观察项。
**建议**: gate 补 `set_error(5, 行号)` + RESUME NEXT 测试；POINT 模式语义 ASM 对照；交互 RUN 自动化；修复后进行 v157 review。

---

## 六、指标追踪

| 指标 | v155 | v156 | 变化 |
|------|------|------|------|
| 单元测试通过（test_runner） | 3308 | **3331** | ✅ +23（+12 测试函数） |
| 定义数 / 注册数 | 872 / 872 | **891 / 891** | +19（含前序未计入项；0 孤儿） |
| 孤儿数 | 0 | **0** | 保持 |
| 生产/测试警告 | 0 | **0** | 保持 |
| .bas 集成 | 82/82 | **83/83 全量直接运行一致** | ✅ +1（gfx_screen0） |
| obs-3 reclen/filenum | ⏳ | ✅ **修复**（边界全验） | 关闭 |
| obs-5 SCREEN 0 图形 | ⏳ | ✅ **修复**（含 file-form 回归） | 关闭 |
| obs-6 exec_rc | ⏳ | ✅ **修复**（exit -1 保持） | 关闭 |
| obs-4 交互 RUN smoke | ⏳ | ⚠️ **仅手动实证（无自动化）** | 未闭环 |
| **SCREEN 0 gate 错误码/ERL/RESUME** | — | ❌ **MEDIUM-1** | 新发现 |
| M2 进度 | 2/3 + OPEN 语义 | **2/3 + 观察项批次** | 推进 |

**注释**:
- 独立验证：干净重建 → 单测 3331 → 83 个 .bas 分 6 批直接运行（expected 对比）→ 6 组 probe（SCREEN0 gate/reclen/filenum/POINT/terminating/注册统计）
- 全程未使用脚本（分批命令链 + 内置工具）；Reviewer 角色约束保持（仅更新 CODE_REVIEW.md）
- 注册计数说明：`int run_test(` 定义行被 grep 计入 → 实测 892-1=891，与定义数一致
- **M2 状态**: 观察项批次落地；gate 错误处理一致性为剩余缺口