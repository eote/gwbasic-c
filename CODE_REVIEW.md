# GW-BASIC C Port - Code Review v177

**日期**: 2026-11-20
**审查范围**: `9f54670` → `8881817`（v176 修复轮 `a70b2fa` + 进度登记 `8881817`）
**测试状态**: 断言 **3616/3616 通过** | 测试函数 **973 = 973**（0 孤儿）| 编译警告 **0** | .bas 目录全量 **113/113 严格字节全等（FAIL=0，历史上首次）**

**总体评分**: **B+（可合并）** — **0 CRITICAL + 0 HIGH + 0 MEDIUM + 2 LOW**

---

## 一、概述

`a70b2fa`（tests 2 文件）修复 v176 的 1M/1L：

| 文件 | 变更 |
|------|------|
| `tests/test_str_semantics.c` | +22：`test_gosub_tail_host_clear` 自足化——测试内自注册 host 行 `gwlist_add_line(960, "' self-contained host line")`；宿主续点从 `gwlist_find_line(960)->next->number` **动态推导**（不再硬编码 961）；自建行 960 加入生命周期清理 |
| `tests/bas/test_on_error.expected` | 补 `\n` 尾换行（最后一个假失败清零） |

**src 无行为变更**。

### 验证结果

| 项 | 结果 |
|----|------|
| 构建 | ✅ 0 警告 |
| 单测 | ✅ **3616/3616**（v176: 3615，+1 断言 `host && host->next`）；**973=973**，0 孤儿 |
| .bas strict | ✅ **113/113，FAIL=0**（v176: 112/113；**自 v159 起 9 项 expected 假失败经 v175(8)+v176(1) 两轮全部归零**，strict 比对历史上首次全绿） |
| 回归（src 零行为变更 + .bas strict 全量） | ✅ 无回归 |

### v176 问题闭环

| v176 | 状态 |
|------|------|
| M1 测试独立性问题（g_curlin=960 依赖前序测试） | ✅ **闭环**：自注册 960 行 + 动态推导 host 续点 + `gwlist_delete_line(960)` 清理——测试自足，注册顺序重排不再脆断 |
| L1 test_on_error.expected 尾换行 | ✅ **闭环**：补 `\n`，strict 假失败归零 |

---

## 二、问题清单

### 🟥 CRITICAL

无。

### 🟧 HIGH

无。

### 🟨 MEDIUM

无。

### 🟩 LOW

| # | 描述 |
|---|------|
| 1 | **`post-tail RETURN` 的"no host leak"断言仍不可区分泄漏：第二次 `GOSUB 970` 时 `g_curlin` 仍为 960（未更新），泄漏（`g_tail_host_return` 残留 961 → return_line=961）与正常（960 的 next=961）结果相同，断言 `host->next->number` 在泄漏回归下照样通过** |
| 2 | **同处注释误导**：`/* marker cleared — a later GOSUB returns to the line after its caller again (970's next is 980) */`——实现中第二次 GOSUB 的调用行仍是 960（非 970），实际返回 961（960-next）而非注释声称的 980；注释与行为不符 |

**LOW-1 详情:** `next86_gosub_to_line`（next86.c:638）`if (g_tail_host_return > 0) return_line = g_tail_host_return;`。要区分"host 泄漏"有两种方式：① 第二次 GOSUB 前把 `g_curlin` 更新为另一条调用线（如 990），则正常返回 990-next、泄漏返回残留 961；② 断言第二次 GOSUB 压栈的 `text_ptr`（需导出 gosub 栈查询）。当前断言在两种路径下数值恒等，属于"名义上测泄漏、实际测不到"——机制本身的端到端防护仍由 T2 `.bas`（test_gosub_nested_tail）承担，故定为 LOW。

**修复建议（可选，不影响合并）:** 在第二次 GOSUB 前加 `g_curlin = 990;`（并自注册 990 行）使正常/泄漏路径分叉；或删除该断言改为注释声明"泄漏检测由 T2 .bas 覆盖"。同时修正 LOW-2 注释（970 非调用行，应为 960/990）。

---

## 三、架构评估

| 决策 | 评价 |
|------|------|
| 测试自足化（自注册 960 + 动态推导 host + 行生命周期清理） | ✅ 干净闭环 v176 MEDIUM-1；与 970/980 同样的自建/自清模式，注册顺序不再敏感 |
| host 续点动态推导（`host->next->number`） | ✅ 消除了硬编码 961 的脆断源，且同时锁定"GOSUB 返回调用行下一行"语义 |
| test_on_error.expected 补尾换行 | ✅ strict 全绿，9 项假失败历史清零（v175 修 8 + v176 修 1） |
| post-tail 泄漏检测断言 | ⚠️ 名不副实（LOW-1），但不影响机制验证（T2 .bas 兜底） |

**根因**: v176 的 MEDIUM-1 与 LOW-1 均按既定建议精确修复；唯一残留是测试内"泄漏检测"场景构造不完整（调用行未变化），属测试设计余量而非功能缺陷。

---

## 四、改进建议

### 短期
1. （可选）LOW-1/LOW-2：第二次 GOSUB 前 `g_curlin = 990` 使泄漏路径分叉，或删除断言并注释引 T2 `.bas`；修正"980"注释为实际语义。

### 中期
2. .expected 尾换行已全绿——建议把 expected 生成/校验纳入工具链（统一 `printf` 带 `\n`），杜绝此类假失败复发。
3. 保持"探针→红灯→修复→转正"循环；本轮起 .bas strict 可作为硬门禁（FAIL=0 才计通过）。

### 长期
4. （同 v176）考虑导出 `next86_gosub_peek_return_line()` 类查询，使单测可断言压栈 text_ptr，根治"泄漏检测"类断言的名不副实。

---

## 五、总结

**总体评价**: v176 的 MEDIUM-1（测试独立性）与 LOW-1（test_on_error 尾换行）均精确闭环：gosub 测试自足化（自注册/动态推导/自清）消除了注册顺序脆断，`.bas` strict 比对 **113/113 FAIL=0**——自 v159 起 9 项 expected 假失败经两轮全部清零，严格门禁历史上首次全绿。0C/0H 连续四轮，src 三轮零行为变更。残留仅 2 项 LOW：post-tail 泄漏检测断言在恒定 g_curlin 下无法区分泄漏（名不副实，机制由 T2 .bas 兜底）及注释 980 与实现不符。**建议：可合并**。

**关键发现**: **0 CRITICAL / 0 HIGH / 0 MEDIUM / 2 LOW**；3616 单测、113/113 strict .bas（首次全绿）、0 警告、0 孤儿。
**建议**: 可合并；两项 LOW 为测试设计余量，随缘修复。

---

## 六、指标追踪

| 指标 | v176 | v177 | 变化 |
|------|------|------|------|
| 断言 / 通过 | 3615 / 3615 | **3616 / 3616** | +1 |
| 测试函数 | 973 | **973** | 保持 |
| 孤儿数 | 0 | **0** | 保持 |
| 警告 | 0 | **0** | 保持 |
| .bas strict 全等 | 112/113 | **113/113 FAIL=0** | ✅ 首绿 |
| 测试独立性（gosub host） | 依赖前序 960 | **自足 + 动态推导 + 自清** | ✅ M1 闭环 |
| test_on_error 尾换行 | 缺 \n | **已补** | ✅ L1 闭环 |
| CRITICAL / HIGH / MEDIUM | 0/0/1 | **0/0/0** | ✅ 首次 MEDIUM 清零 |

**注释**: 构建/单测 `CC=D:/myplace/program/mingw64/bin/gcc.exe`；.bas 全量字节比对（\r 归一、stdin=/dev/null）113/113 严格相等。`g_tail_host_return` 相关取证自 next86.c:28/507/565/638-639。仅更新本报告，未改源码/测试、未提交。