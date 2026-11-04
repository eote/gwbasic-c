# GW-BASIC C Port - 开发进度记录 (基于 PLAN_FRESH.md)

## 对应 PLAN_FRESH.md 计划位置

| 维度 | 定义 |
|------|------|
| **里程碑** | M0 ✅ → M1 正式关闭 ✅ → **M2 进行中 2/3+审计** (GWLIST ✅ / .bas 基线+ON..GOTO ✅ / INSTR 正式闭环 ✅ / 字符串函数一致性审计 ✅) |
| **当前阶段** | v164 修复轮 — 空串默认语义 + 秩校验全路径 (数值读/串读/串赋值) (3H 全处理); 下一步 v165 review |
| **依赖顺序** | M0 ✅ → M1 ✅ → M2 → M3+ |
| **TDD 工作流** | 7 步标准化流程 |

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

---

## 当前轮: CODE_REVIEW v159 修复轮 — 2C/8H/8M/5L 全处理 (阻断解除)

### CRITICAL (reviewer 实证复现)

| # | 修复 |
|---|------|
| C1 outbuf[1024] 溢出 (6×200 字符崩) | 输出改 4KB 分块 chunk 写 (malloc + fwrite flush), 无总长限制 |
| C2 buf[160] 栈溢出 + cast UB | 字段位数 ASM 上限 24 (扫描器强制, 超限 FCERR 5); double→int 前范围检查 (>=1e18/NaN → % 路径 %.0f); 无界 sprintf 全部 snprintf/有界 |

### HIGH (8 项)

| # | 修复 |
|---|------|
| H1 科学计数 E+10 写死 | ASM $PUFE 规范化: 尾数 [1,10) + 2 位指数 (进位处理) |
| H2 $$/**/**$ | 字段起始解析: $$ 空格+$ 浮动; ** 星填; **$ 星+$ (宽度含星列与 $ 列) |
| H3 负溢出 %--1234.56 | 单 % + 单负号 |
| H4 HEX/OCT 截断 | FRCINT 语义: 四舍五入 + -32768..65535 范围 + Overflow(6) |
| H5 SWAP 数组/自动建 | parse_var_ref + biptrg get/set 数组元素; 未定义标量自动建 (PTRGET 语义); 修 result 初值 ERROR bug |
| H6 USING 大小写 | 显式 U/s/i/n/g 比较 |
| H7 vals[64] 截断 | gwram 动态扩容 (倍增) |
| H8 类型不匹配静默 | TMERR(13) — 串入数字字段/数入串字段; gwsts_print_using 错误传播修复 (driver set_error 后 return SUCCESS bug) |

### MEDIUM/LOW

M1 无字段 FCERR(5) ✓ / M2 缺分号 SNERR(2) ✓ / M3 _ 转义 ✓ /
M4 逗号计宽 ✓ / M5 尾逗号打印区 ✓ / M6 test_using.bas 登记 ✓ /
M7 补 9 条单测 (test_biprtu.c: 溢出/科学/$$/**/**$/TMERR×2/24位
上限/长串不崩/无字段/缺分号) ✓ / M8 24 位上限 ✓ /
LOW-1 注释错挂修复 ✓ / LOW-2 死代码删除 ✓ / LOW-3 空格式 ✓ /
LOW-4 ! 空串空格 ✓ / LOW-5 union 移除 ✓

### 额外发现 (本轮修复)

- gweval 数字词法不支持科学计数字面量 (1E+10 → 1) → 补 E/D 指数解析
- SWAP 字符串指针交换跨堆 free (gwdata vs gwram) → 纯指针交换
  (v138 所有权语义一致)
- gwsts_print_using 忽略 driver 内 error → 传播修复

### 验证（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3366/3366 PASS** (3344→3366, +22), 0 警告 |
| .bas 逐个验证 | ✅ **86/86** (+test_using 登记) |
| 孤儿检测 | ✅ **906 = 906** |
| reviewer 实证回归 | ✅ C1 1200 字符存活 / C2 长字段+1E10 / H1 1.23E+02 / H2 $12.34 ***12 / H3 %-1234.56 / H4 HEX$(1.9)=2 65536→Overflow / H5 数组交换+自动建 / H8 TM=13 / M1 FC=5 / M2 SN=2 |
| 遗留 | ERL (v158 MEDIUM-1) 仍开放; **$ 填充列数按右对齐 (与 reviewer 期望差 1 星, 按手册 **$ 数字位语义) |

---
## 当前轮: M4 收尾批 — PRINT USING 重写 + SWAP + HEX$/OCT$ (ASM 对照)

### 背景

probe 盘点发现 M4 三缺口: PRINT USING 从未生效 (分派设计错误:
"USING 前=格式" 而真实语法 fmt 在 USING 之后, 且值列表跨表达式
片段); SWAP 语句缺失; HEX$ 未实现 (OCT$ stub)。

### ASM 对照 (BIPRTU.ASM L17-140 PRINUS)

- FRMCHK 求 USING 串 (任意表达式) + SYNCHR 73O (分号必须)
- 流式扫描: 非字段字符原样输出; "!" 单字符; "&" 整串;
  "\"(2+空格) 定宽; "#" 数字字段 (+ . , $ ** ^ 前后缀);
  USFLG/REUSIN: 格式扫完值未尽 → 重扫格式串 (打印过值才合法)
- 溢出: 值超字段宽 → '%' 前缀 + 完整值

### 实现

| # | 内容 |
|---|------|
| 1 | biprtu_print_using_values(fmt, vals[], n) 值列表驱动 (含 scientific /$,逗号分组/前后缀; 溢出 % + PUFOUT 风格十进制; frac=0 整数无小数) |
| 2 | gwsts_print_using() 语句级 handler: fmt 分割 (顶层 ';' 引号/括号感知) + 值列表收集 (三段式类型探测: 引号→串; 串解析探针成功→串; 否则数值) + 尾分隔符 newline 语义 |
| 3 | gwsts PRINT 入口 USING 前缀分派 (整条语句, 不进逐表达式循环) |
| 4 | SWAP 语句: 同型标量交换 (字符串换 ptr/len; 数值 member-wise); 类型不匹配/缺失 → error 13 |
| 5 | HEX$/OCT$: 16 位补码 (HEX$(-1)="FFFF"); gweval 函数分派 |
| 6 | 旧 USING 测试 expected 重写 (旧为缺陷产物: 无格式化输出) |

### 验证（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3344/3344 PASS**, 0 警告 |
| .bas 逐个验证 | ✅ **85/85**（+test_using +test_m4_funcs; 3 个旧 USING expected 按真值重写） |
| 孤儿检测 | ✅ **897 = 897** |
| ASM 语义 probe | ✅ ##.##→3.14 / 溢出 %1234 / , 字面量 / -45.68 符号紧贴 / REUSIN 重用 / ! & \\ 字段 / SWAP 数值串浮点 / HEX$(-1)=FFFF |

---
## 当前轮: CODE_REVIEW v157 复核确认 — 0 问题 + 观察项 1/2 补齐

- v157 报告: 0 CRITICAL/HIGH/MEDIUM/LOW; MEDIUM-1 修复确认
  (6/6 gate probe 闭环); 观察项 1 (gate 覆盖 2/6) + 2 (teardown
  不一致) 本轮补齐
- 观察项 1: 补 LINE/PAINT/GET/PUT 4 处 gate 错误码单测
  (test_gwsts_gfx2_screen0_{line,paint,get,put}_err — 各断言
  语句失败 + error==5); 端到端 probe: LINE/PAINT RESUME NEXT
  恢复 (ERR=5 → *-FAIL → GATE-DONE) ✓
- 观察项 2: err_line 测试补 gwdata_clear_error() teardown;
  新测试统一走 gwsts_o1_setup/teardown helper
- 验证: 单测 3344/3344 (893→897 定义, +4), 0 警告, 84/84,
  孤儿 897=897; 提交含共同作者 trailer

---
## 当前轮: CODE_REVIEW v156 — MEDIUM-1 修复 (SCREEN 0 gate 错误码/RESUME NEXT)

### 问题 (reviewer 实证)

SCREEN 0 gate 直接 `result = ERROR` (rc<0 通道), 未 set_error →
ON ERROR handler 进入但 ERR=0、RESUME NEXT 无法恢复 (rc<0 分支的
gwdata_handle_error 不分配 ERR)。

### 修复

6 处 gate (PSET/PRESET/LINE/CIRCLE/PAINT wrapped-gate 尾 + 
gwsts_handle_gfx_get/put 顶部) 统一:
`gwdata_set_error(5, (uint16_t)g_state->current_line, 0)` 后 ERROR。
executor 的 `gwdata_get_error()!=0` 分支分配 ERR=5 → dispatch
handler → RESUME NEXT 读 get_error_line() 恢复。

### 验证（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3334/3334 PASS** (+3: err code/line 单测) |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证 | ✅ **84/84**（+test_gfx_screen0_resume: ERR=5/PSET-FAIL/ERR=5/CIRCLE-FAIL/GATE-DONE — RESUME NEXT 逐语句恢复） |
| 孤儿检测 | ✅ **893 = 893** |
| mini 链路 | ✅ set_error(5,30) → dispatch ERRvar=5 → handler |
| 端到端 | ✅ reviewer 原实证程序: ERR= 5 / PSET-FAIL / GATE-DONE 全恢复 |
| 遗留登记 | ERL 变量未实现 (打印 0) → 新观察项; obs-4 自动化/POINT gate → 开放 |

---
## 当前轮: v155 观察项批次 — obs-3/4/5/6 处理 (ASM 对照 + 修复)

### ASM 对照结论 (观察项 3)

- **POFNUM/GETNZB (GIO86 L665-694)**: filenum 经 GETBYT (byte 0..255);
  **0 → DERBFN (Bad file number)**; >255 → FCERR。dskcom_open 已有
  filenum<1||>255 → ERROR ✓ (无需改)
- **INTID2 (GWMAIN L1930)**: 结果 16 位 int, 负 (高位 1, 即
  reclen>=32768) → **FCERR**; VARECS: 0 → FCERR
  → 修复: reclen<=0 || reclen>32767 → ERROR (classic + LEN= 两处)

### 修复 (观察项 5/6)

| # | 内容 |
|---|------|
| 1 | obs-5: SCREEN 0 (文本模式) 下图形语句 → ERROR (ASM FCERR): PSET/PRESET/LINE/CIRCLE/PAINT 分支 mode gate + gwsts_handle_gfx_get/put 顶部 gate (文件形式 PUT #1/GET #1 不受影响) |
| 2 | obs-6: run_program_lines 的 exec_rc 恒为 SUCCESS (循环内全部 return) → 删除变量, 尾部 return SUCCESS |
| 3 | obs-4: 交互 RUN smoke 实证 (stdin: 10 FOR/20 PRINT/30 NEXT + RUN → IT=1/2/3 + Ok; 需行间 sleep — scnedt 读取时序, 与 RUN 语义无关) |
| 4 | 测试: openmodes +5 (reclen 32768→FCERR、filenum 0/256→ERROR、filenum 255 边界 OK、reclen 32767 边界 OK) + gfx2 +7 (SCREEN 0 下 PSET/PRESET/LINE/CIRCLE/PAINT/GET/PUT → ERROR, 文件形式不受影响) + 集成 test_gfx_screen0.bas/.expected (SCREEN 1 画图正常 + SCREEN 0 PSET 被 ON ERROR 捕获) |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3331/3331 PASS** (3308→3331, +23) |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **83/83**（+test_gfx_screen0） |
| 孤儿检测 | ✅ **891 = 891**, 0 孤儿 0 幽灵 |
| 端到端 6 probe | ✅ SCREEN 0 PSET/CIRCLE → Error; reclen 40000 → Error; filenum 0 → Error; SCREEN 1 PSET → OK; reclen 32767 → OK |
| 交互 RUN smoke | ✅ FOR 1..3 → IT=1/2/3 |

---
## 当前轮: CODE_REVIEW v153 复核确认轮 — 0 问题 (基线确认)

- v153 报告: 0 CRITICAL/HIGH/MEDIUM/LOW; 自 v152 无代码变更
  (仅 v152 报告入库 + mini_open.c 清理), reviewer 独立验证基线
  3308/3308 + 0 警告 + 82/82 保持 + 4 抽样一致
- 观察项: 1 (mini_open.c) ✅ 关闭确认; 2-6 开放 (待 ASM 对照/M2 下一批)
- 下一步 (reviewer 建议): 观察项 3 (classic filenum/reclen 上限
  ASM 对照) + 观察项 4 (交互 RUN smoke) + M2 下一批 → v154

---
## 当前轮: CODE_REVIEW v152 复核确认轮 — 0 问题 (基线确认)

- v152 报告: 0 CRITICAL/HIGH/MEDIUM/LOW; 观察项 2/3 关闭确认
  (表达式 filespec 三形式 + reclen RANDOM-only SNERR/FCERR 分层 +
  eval 串借用不 free), reviewer 独立验证 3308/3308 + 0 警告 +
  82/82 全量 .bas + RECLEN probe
- 观察项处置: 1 (tests/mini_open.c 调试残留) → 已删除 ✅;
  2 (表达式 filespec 泄漏, ASM 堆语义一致) / 3 (classic filenum 空#
  与 reclen 32767 上限, 待 ASM 对照) / 4 (交互 RUN smoke, v148) /
  5 (v145 图形观察项) / 6 (run_program_lines exec_rc 简化, v148) → 登记
- 基线保持: 单测 3308/3308, 0 警告, 82/82, 孤儿 872=872

---
## 当前轮: M2 下一批 — v151 观察项 2/3 修复 (ASM GIO86 对照)

### ASM 依据 (GIO86.ASM L59-101, L1462+ NAMSCN, L160+ PORLEN)

- **NAMSCN: CALL FRMEVL** — filespec 是任意字符串表达式 (观察项 2
  依据), 非仅引号字面量
- **PRLENC: 仅 FILMOD==MD_RND 才解析 reclen** — O/I/A 带 reclen parm
  = 语法错误 (观察项 3 依据, 旧实现"强制 RANDOM"是错的)
- **VARECS: reclen <= 0 → FCERR**

### 修复 (gwsts.c: OPEN 抽取为 gwsts_exec_open + 表达式 filespec +
reclen 语义)

| # | 内容 |
|---|------|
| 1 | OPEN 分支抽取为独立函数 gwsts_exec_open (双形式共享 phase1 表达式解析; 修复链结构) |
| 2 | filespec 支持引号字面量 **或任意表达式** (F$ / A$+".TXT"; 引号/括号感知顶层逗号扫描; new-style 以顶层 " FOR "/" AS" 为表达式边界) — classic 与 new-style 两形式 |
| 3 | reclen 仅 RANDOM: O/I/A + ",reclen" 或 LEN= → SNERR (不再强制 RANDOM); reclen <= 0 → FCERR |
| 4 | 所有权契约修复: gweval_eval_string 简单变量返回**变量表内部指针** (非副本) — gwram_free 它 = 堆损坏 (0xC0000374 实证); fs_owned/ns_owned 区分字面量 (free) 与求值结果 (不 free, 与 PRINT/assign 既有契约一致) |
| 5 | 测试: test_gwsts_openmodes.c +6 (classic/new-style/concat 变量 filespec、modeO+reclen SNERR、OUTPUT+LEN= SNERR、reclen0 FCERR) + test_open_varspec.bas/.expected (三种 filespec 形式读写回 KILL 全流程) |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3308/3308 PASS** (3287→3308, +21) |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **82/82**（+test_open_varspec; 首轮 1 瞬时状态失败, 复跑一致无问题） |
| 孤儿检测 | ✅ **879 = 879**, 0 孤儿 0 幽灵 |
| 端到端 6 场景 | ✅ classic_var C1 / newstyle_var N1 / concat K1 (写→读回→KILL); modeO+reclen → SNERR 不建文件; OUTPUT+LEN= → SNERR; reclen 0 → FCERR |

---
## 上一轮: CODE_REVIEW v151 复核确认轮 — 0 问题 (基线确认)

自 v150 无代码变更 (仅 v150 报告入库 d7cefb6 + 残留清理)。reviewer
独立验证: 3287/3287 单测、81/81 基线 + 4 抽样一致、0 警告。
0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW。观察项 1 (残留文件) 关闭。

观察项状态: 1 ✅关闭; 2 filespec 变量 / 3 reclen 组合 (待 ASM GIO86
对照); 4 R 模式 FIELD/GET/PUT 深度; 5 交互 RUN smoke + exec_rc 简化;
6 v145 图形观察项 — 均开放, 随 M2 下一批处理。

---
## 上一轮: CODE_REVIEW v150 复核确认轮 — 0 问题 (6 观察项)

v149 MEDIUM-1 (经典 mode-first OPEN) 修复确认: 双形式共存、坏模式
拒绝不建文件、小写归一、资源无泄漏; 3287/3287 单测、81/81 .bas、
0 警告、865=865。0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW。

观察项登记 (后续批次):
1. ~~tests/A|I|O|R|X 残留~~ — 已清理 (dev probe 垃圾, 单测 cwd 落点)
2. filespec 变量 (OPEN "O",#1,F$) — 待 ASM GIO86 NAMSCN 对照后实现或 SNERR
3. mode O + reclen 组合语义 — ASM PORLEN 对照
4. R 模式 reclen 深度验证 (FIELD/GET/PUT)
5. 交互 RUN 入口自动化 smoke (v148 观察项延续)
6. v145 图形观察项 (PUT 容量校验、CIRCLE radius、SCREEN 0 图形语句)

---
## 上一轮: CODE_REVIEW v149 MEDIUM-1 修复 — 经典 mode-first OPEN

### 根因 (reviewer 实证独立复现: 静默创建文件 O)

gwsts OPEN 分支只实现新式 `OPEN filespec FOR mode AS #n` —
`OPEN "O", #1, "file"` 被解析为打开文件名 "O" (其余参数静默忽略)
→ PRINT #1 失败 + 文件系统垃圾文件。ASM GIO86.ASM L59-101 明确
支持双形式。

### 修复 (gwsts.c OPEN 分支, ASM L64-101 语义)

| # | 内容 |
|---|------|
| 1 | 首个引号表达式后跟逗号 → mode-first 分支: mode 首字符 O/I/A/R (大小写不敏感, ASM AND -1-" ") → 非 O/I/A/R → ERROR (ASM ERBFM0 "Bad file mode") → `,` `#fnum` `,` 引号 filespec `[,reclen]` (reclen → RANDOM, ASM PORLEN) |
| 2 | 新式路径行为不变 (filespec 复用同一表达式存储) |
| 3 | 测试: test_gwsts_openmodes.c (+6: O/I/A/R/坏 mode 无文件创建/小写 o) + test_open_classic.bas/.expected (O 写 → I 读 → A 追加 → I 读回 → KILL) |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3287/3287 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **81/81**（+test_open_classic） |
| 孤儿检测 | ✅ **873 = 873**, 0 孤儿 0 幽灵 |
| 端到端 | `OPEN "O",#1,"T_CLASSIC.TXT"` → T_CLASSIC.TXT 内容 DATA-OK, 无垃圾文件 O; `OPEN "X"` → 错误且不创建文件 |

---
## 上一轮: CODE_REVIEW v148 MEDIUM-1 修复 — RUN 环境复位

### 根因 (reviewer 实证独立复现: A=1 → A=2 + ON ERROR 残留生效)

RUN 仅复位 DATA 指针 — 变量表/string heap/FOR 栈/GOSUB 栈/
ON ERROR 状态/ERR 变量/打开文件全部跨 RUN 残留 (GW-BASIC 语义:
RUN 清所有变量并关闭文件)。

### 修复

| # | 内容 | 文件 |
|---|------|------|
| 1 | gwsts_reset_runtime_environment(): gwdata_init (变量+string heap)、giodsk_close_all (文件)、next86_clear_for_stack/clear_gosub_stack、ON ERROR 关闭 (set_on_error_goto(0,0)) + exit_error_handler + clear_error、bimisc_data_init (DATA)。程序行 (gwlist) 与屏幕/图形状态不清 (GW-BASIC RUN 语义) | gwsts.c |
| 2 | next86 新 API: next86_clear_gosub_stack() / next86_get_gosub_depth() (GOSUB 栈原本无公开复位) | next86.c + gwbasic.h |
| 3 | RUN 命令: bimisc_data_init() → gwsts_reset_runtime_environment() (文件入口不调 — 新进程环境本干净) | gwmain.c |
| 4 | 测试 test_gwmain_resetenv.c (+3): 变量+字符串清空且环境可用 / FOR+GOSUB 栈清空 / ON ERROR+ERR 清空 | tests/ |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3260/3260 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **80/80** |
| 孤儿检测 | ✅ **867 = 867**, 0 孤儿 0 幽灵 |
| 交互 RUN 实证 | 两次 RUN `A=1 → A=1` (复位); ON ERROR 第一次生效 H=11, 第二次 RUN 新程序除零 → Error at line 20 (状态清); FOR 栈残留 → OK2 |

---
## 上一轮: CODE_REVIEW v147 CRITICAL-1 修复 — 统一执行器 + IF..THEN 行号

### 根因 (reviewer 实证 5 项全部独立复现)

交互 RUN 命令分支 (gwmain L166-211) 是未升级的第三份执行循环:
无 g_curlin/current_line、rc!=0 一律当错误 (END -2 与 rc>0 跳转
全部失效)、无 ON ERROR、无 DELETE liveness → FOR 只跑 1 次、
GOTO/END/IF..THEN 报 Error、DELETE 存在 UAF 读。
测试盲区: 单测手动设 g_curlin 掩盖调用方契约; .bas 只走文件路径;
交互 RUN 零自动化覆盖。

### 修复 (gwmain.c + gwsts.c)

| # | 内容 |
|---|------|
| 1 | **抽取统一执行器 run_program_lines()** (以文件路径为蓝本): g_curlin/current_line、END(-2)、rc>0 跳转、ON ERROR/handler-FATAL、DELETE-liveness 重定位 (v126/v127 语义) — RUN 命令与 main 文件执行共用, 删除旧简化循环 |
| 2 | RUN 命令补 bimisc_data_init() (GW-BASIC RUN 语义: DATA 指针每次 RUN 重置) |
| 3 | **IF..THEN <行号> 跳转** (GW-BASIC: THEN/ELSE 后纯数字 = GOTO; 语句形式不变) — 文件与交互路径同样受益 (此前两路径都缺失, v138 只实现语句形式) |
| 4 | 测试: test_gwsts_ifgoto.c (+6: THEN 行号真/假、ELSE 行号、无 ELSE 落空、THEN/ELSE 语句形式保持、混合) + test_if_lineno.bas/.expected + 交互 RUN 端到端 8 场景 |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3248/3248 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **80/80**（+test_if_lineno） |
| 孤儿检测 | ✅ **864 = 864**, 0 孤儿 0 幽灵 |
| **交互 RUN 端到端 (stdin 管道, 8 场景)** | ✅ FOR 1..10 全输出 / END 优雅结束 / GOTO TARGET / IF..THEN 30 → T1 / GOSUB+RETURN SUB→MAIN / ON ERROR HANDLER ERR=11 / FOR-skip REACHED / DELETE liveness A,C — 无一 Error 误报 |

---
## 上一轮: CODE_REVIEW v145 MEDIUM-1 修复 — FOR 0 次 + NEXT 最后一行

### 根因 (v145 实证独立复现: BODY + Error at line 30)

FOR 0 次跳转路径: scan 匹配 NEXT 后无后续行 → 返回 0 → gwmain 把
rc==0 当"顺序继续" → 执行 FOR 的下一行 (循环体) → 随后 NEXT 因
FOR 已弹栈 → NEXT without FOR。

### 修复 (next86.c)

scan 无后续行 (next_line == 0) 时返回 **-2 (END 语义哨兵)** —
复用 gwmain 现有 END 处理 (break, 优雅结束); ASM 语义一致:
NEWSTT 走过 NEXT 后无下一语句 → 程序结束。

### 测试

- test_next86_firstiter.c +2: skip_next_last_line (rc==-2 + I 保持 1)
  / skip_gwsts_end_signal (gwsts 分派层透传 -2)
- test_for_lastnext.bas/.expected (空输出 = 体不执行 + 正常结束, 套件 79)

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3242/3242 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **79/79**（+test_for_lastnext） |
| 孤儿检测 | ✅ **858 = 858**, 0 孤儿 0 幽灵 |
| .bas 实证 | NEXT 最后行: 无输出正常结束; NEXT 中间行: 体不执行 + REACHED 继续 |

---
## 上一轮: FOR 语句审计 — 首次迭代检查 + STEP 0 哨兵

### 审计发现 (行为级 probe fortest1.bas 实证)

- **首次迭代检查缺失**: `FOR M = 3 TO 1` 循环体错误执行一次
  (ASM GWMAIN FOR L1617: JMP NEXTS with NXTFLG=0 — FOR 语句立即用
  **初值**做终止比较 (IFORIN, 不加 step), 失败则 LOOPDN 跳到匹配
  NEXT 之后, 体 0 次, 变量保持初值)
- **STEP 0 语义**: ASM NT0STP — step 符号强制 2, 终止测试永假 =
  ANSI 要求的无限循环; 我们 step_sign=(0>=0)=1 → 语义错误

### 修复 (next86.c)

| # | 内容 |
|---|------|
| 1 | next86_for: STEP 0 → step_sign=2 哨兵 (NEXT 终止测试永假) |
| 2 | next86_for: push 后首次迭代检查 — done = (step>0 && start>limit) ‖ (step<0 && start<limit); step==0 永不 done |
| 3 | done → 弹出 entry + next86_scan_matching_next(g_curlin) (深度感知: 初始 depth=1, FOR++/NEXT--, 跳过 from_line 前所有行) 返回 NEXT 后行号; gwmain rc>0 跳转 |
| 4 | 测试 test_next86_firstiter.c (+4): skip_positive (rc=40 + I 保持 3) / skip_negative / boundary_runs (3 TO 3 执行) / step0_sentinel |
| 5 | test_for_edges.bas/.expected (+expected 13 行: 含嵌套 P done 场景 — 内层 Q 体也不跑, 深度扫描正确) |

### FOR 语义验证矩阵 (全部 ASM 对照通过)

| 场景 | 期望 (GW-BASIC) | 实测 |
|------|------|------|
| 1 TO 3 | 1,2,3; AFTER=4 (超限保留) | ✅ |
| 5 TO 1 STEP -2 | 5,3,1; AFTER=-1 | ✅ |
| 3 TO 3 | 执行 1 次; =4 | ✅ |
| 3 TO 1 (正 step) | 体 0 次; 保持 3 | ✅ (修复) |
| 1 TO 3 STEP -1 | 体 0 次; 保持 1 | ✅ (修复) |
| 1 TO 0 嵌套 | 外层体 0 次 (内层也不跑); P=1 | ✅ (深度扫描) |
| STEP 0 | 进入循环, NEXT 永不终止 (GOTO 逃离验证: ESCAPED=8) | ✅ |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3238/3238 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **78/78**（+test_for_edges） |
| 孤儿检测 | ✅ **856 = 856**, 0 孤儿 0 幽灵 |

---
## 上一轮: CODE_REVIEW v144 CRITICAL-1 修复 — 图形 GET/PUT 标准数组语义

### 根因 (v144 实证 3 项全部独立复现)

图形 GET/PUT 旧实现以"字符串变量"为私有存储 — DIM 数值数组的
value.s.ptr 是数组描述符指针 → GET 覆写描述符 (G(1) 数据丢失 +
array_data 野指针风险); PUT 硬性要求 VT_STRING 拒绝数值数组;
字符串标量变量也被静默覆盖。

### 修复 (ASM GPUTG L864 语义)

| # | 内容 | 文件 |
|---|------|------|
| 1 | GET/PUT 重写为标准数值数组路径: 头 2 元素 = x-bits(word)+y-count(word), 位图 MSB-first 跨行连续 (mode1: 2bpp / mode2: 1bpp) 写入 desc->data 原始字节 | advgrp.c |
| 2 | 容量检查: need_bytes > size*elemsize → FCERR(5) (不越界写) | advgrp.c |
| 3 | 类型检查: 字符串变量/字符串数组 → FCERR; 未 DIM 名字 → 隐式 DIM(10) (biptrg_ensure_array, <11 元素免 DIM 语义) | advgrp.c + biptrg.c |
| 4 | GET 位图区 memset 后写入 (ASM 覆写语义, 非 OR — 修复旧值残留) | advgrp.c |
| 5 | PUT PRESET 动作按 bpp 反转 (旧版固定 -1 只对 1bpp 正确) | advgrp.c |
| 6 | array_desc_t/ARRAY_MAGIC 从 biptrg.c 私有移至 gwbasic.h (跨模块共享, ASM GTARRY 单一访问器语义) | gwbasic.h + biptrg.c |
| 7 | 旧 test_advgrp.c 4 测试按 ASM 真值重写 (字符串私有断言 → 数值数组断言) | test_advgrp.c |
| 8 | 新测试 +4 (DIM preserved/roundtrip/string rejected/too small) + test_gfx_getput.bas/.expected + test_gfx_circle.bas 修 DIM | tests/ |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3230/3230 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **77/77**（+test_gfx_getput; test_gfx_circle 补 DIM） |
| 孤儿检测 | ✅ **852 = 852**, 0 孤儿 0 幽灵 |
| .bas 实证 | DIM roundtrip PP/PX/PS=1 (XOR/PSET); 两次 XOR 抵消=0; 字符串变量 GET → Error (内容保留); 小数组 GET → Error (FCERR); 未 DIM 小块 → 隐式数组可用 |

---
## 上一轮: M2 图形深化 — CIRCLE/PAINT/GET/PUT 语句接线 + fb 统一根因

### ASM 依据 (ADVGRP.ASM L108-1000)

- CIRCLE L460: SCAN1 圆心, CNPNTS=R*SQR(2)/2, 点计数圆周 8*CNPNTS,
  负角画半径线 (CLINEF), end<start 反转极性 (超 360°), aspect<1 压 Y / >1 压 X
- PAINT L121: 边界色停止, fill 填充, 默认 border=fill
- GET/PUT GPUTG L864: 头 4 字节 x-bits+y-bits; PUT 默认 XOR
  (GFUNTB: PSET/PRESET/AND/OR/XOR)

### 实现

| # | 内容 | 文件 |
|---|------|------|
| 1 | CIRCLE/PAINT 语句分支 (表达式坐标, 复用 gwsts_parse_coord_pair) | gwsts.c |
| 2 | GET/PUT 图形分派: '(' → 图形 handler, 其余 → 文件 handler | gwsts.c |
| 3 | gwsts_handle_gfx_get/gfx_put (数组名解析 + action PSET/PRESET/AND/OR/XOR) | gwsts.c |
| 4 | advgrp_circle 弧线: 点计数筛选 (8 反射 × octant 计数) + aspect + 半径线 | advgrp.c |
| 5 | **根因修复: 双 framebuffer 统一** — gengrp_set_screen_mode 同步 g_fb_width/height (ASM SCRN 单一入口); advgrp_paint 全部改走 gengrp API (gengrp_point/direct_pset, 与 gengrp 同源 fb) — 修复 PAINT NULL fb 段错误 + 语句/直调两路径不一致 | gengrp.c + advgrp.c |
| 6 | gengrp_fb_clear() 测试隔离 API (fb 残留跨测试) | gengrp.c + gwbasic.h |
| 7 | test_gwsts_gfx2.c (6 测试) + test_gfx_circle.bas/.expected + 注册 | tests/ + run_bas_tests.sh |

### 实施中定位的根因 (v142 观察项 2 相关)

- **双 fb**: gengrp (g_local_framebuffer) 与 gioiscn (g_framebuffer) 两套,
  get_framebuffer() 优先 gfb → 语句路径 (SCREEN 1 不触发 gioiscn init) 时
  gfb=NULL → advgrp_paint 直用 g_framebuffer 段错误 (mini exe 铁证
  target_fb=0000000000000000)
- **fb 尺寸不同步**: SCREEN 1 → gengrp_set_screen_mode 不设 g_fb_width →
  advgrp 边界检查 x >= 0 恒真静默 return / midpoint 点全部被拒
- 修复后: SCREEN 语句 → g_fb_*=320x200 + gengrp/advgrp 同一 fb

### 新单测 (+24 → 3223)

gfx2: circle_basic / circle_arc / paint_region / get_put_roundtrip /
put_pset_action / file_get_regression (LINE INPUT # 不被图形 GET 遮蔽)

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3223/3223 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 逐个验证（不经脚本，用户指令） | ✅ **76/76**（+test_gfx_circle） |
| 孤儿检测 | ✅ **848 = 848**, 0 孤儿 0 幽灵 |
| .bas 实证 | CIRCLE 四方位点/圆心未画; PAINT 内=2 外=0; GET/PUT roundtrip+XOR 二次擦除; 弧 0..π 北画南不画; PUT PSET 动作 |

---
## 上一轮: CODE_REVIEW v142 (复核确认轮, 0 问题) — v141 修复全部关闭

### 结论: 0 CRITICAL / 0 HIGH / 0 MEDIUM / 0 LOW + 4 观察项

v141 的 2 MEDIUM + 1 LOW + itsa86 flaky 全部独立验证关闭 (probe 全绿:
隐式数组 0..10/越界 ERRBS/嵌套坐标/紧凑写法)。开发方独立核对注册一致性
842=842, 0 孤儿 0 幽灵 (reviewer 记 840 系显示名/函数名口径差异, 结论一致)。

### 观察项 (不计问题, 随后续批次处理)

1. `LINE  INPUT` (双空格) 遮蔽风险 — 排除建议改词法判断
2. malformed 坐标 (无逗号/闭括号/超长) 静默降级 → 建议 SNERR
3. 隐式多维数组 A(1,2) 语义待 ASM 对照
4. itsa86 段值 | 0x1000 — 若未来模拟器逻辑依赖段地址转换需复查

### 基线

3199/3199 单测, 0 警告, .bas 75/75 (逐个直接运行), 842=842, HEAD e6004ba。

---
## 上一轮: CODE_REVIEW v141 修复 — 隐式数组 + 嵌套坐标 + 紧凑写法

### 修复内容 (2 MEDIUM + 1 LOW + 1 flaky)

| # | 问题 | 修复 | 文件 |
|---|------|------|------|
| MEDIUM-1 | 未 DIM 数组隐式声明缺失 (A(1)=5 → Error) | biptrg 6 个数组入口 (get/set 单/多维 + 字符串) 加 biptrg_ensure_array: 未知名自动 DIM(10) 0-10, 类型按后缀 $ # ! % 推断; 下标越界 → ERRBS=9 (ASM FCERRG, 修复旧 clamp 静默) | biptrg.c |
| MEDIUM-2 | PSET/PRESET 坐标裸 strchr 不感知嵌套括号 (PSET(A(1),A(1)),3 静默错位) | 抽取 gwsts_parse_coord_pair (depth/引号感知) 共享 scanner; PSET/PRESET/LINE 两处坐标全部换用 | gwsts.c |
| LOW-1 | LINE( / SCREEN1 紧凑写法 (关键字后无空格) | LINE: starts_with("LINE") + 下一字符 ∈ {空,tab,(} (保持 LINE INPUT 排除); SCREEN: 下一字符 ∈ {空,tab,数字} | gwsts.c |
| flaky | itsa86_mapclc_c_option 随 build 布局随机红 (addr & 0xF000 可为 0 → NEWDS=0) | itsa86_init 段值 |= 0x1000 保证非零 (真实 .exe 加载段非 0) | itsa86.c |

### 旧测试断言修正 (记录 clamp bug 行为 → ASM 真值)

test_biprtu_biptrg.c (A(-1)/A(11) clamp → ERRBS), test_biptrg.c
(clamp_high/clamp_low → ERRBS; set_nonexistent → 隐式 DIM 成功) —
与 v132 教训同型: 旧断言是 bug 产物, 按修正后语义重写。

### 新测试 (+22 → 3199; 3195 红 → 3199 绿)

- test_gwsts_graphics.c +6: implicit_array_assign_read / implicit_string_array
  / implicit_array_oob / pset_array_coords / line_array_coords / compact_line_screen
- test_biptrg.c +? (计数含 clamp 重写) — 注册后 842=842, 0 孤儿
- test_gfx_v141.bas/.expected (套件 74→75): PSET-ARR/LINE-ARR×2/
  COMPACT-SCREEN/LINE/IMPLICIT-NUM/STR 全绿

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3199/3199 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **75/75**（+test_gfx_v141） |
| 孤儿检测 | ✅ **842 = 842** |
| .bas 实证 | PSET(A(1),A(2))→POINT=1; LINE(A(1),A(2))-(B(1),B(2)) 双端点=1; SCREEN1/LINE( 紧凑 OK; C(2)=9 隐式; S$(1)="HI" 隐式; A(11) → Error (ERRBS) |

### 新发现（不计本轮问题，报 reviewer）

- **观察项**: 冒号复合语句第二段赋值丢失 — `X = 1: Y = 2` → Y=0
  (v141 前已存在, 独立既有 bug; 单行赋值正常)

---
## 上一轮: M2 第 2 批 — 图形语句接线 (SCREEN/PSET/PRESET/LINE/POINT)

### ASM 依据 (GENGRP.ASM L149-186 + ADVGRP)

PSET (X,Y)[,attr] 默认 FORCLR; PRESET 默认 BAKCLR; 越界静默 (JAE PSTNOT);
LINE [(x1,y1)]-(x2,y2)[,attr[,B[F]]] 省略首坐标用 graphics cursor;
POINT(x,y) 越界返回 -1。坐标/属性为表达式 (ASM SCAND/FRMEVL)。

### 实现

| # | 内容 | 文件 |
|---|------|------|
| 1 | gengrp_line_coords() 核心重构 (坐标已解析版, cursor=(x1,y1)→执行→(x2,y2)) | gengrp.c |
| 2 | gengrp_preset 加 color 参数 (-1=默认 BAKCLR) + accessor get_forclr/bakclr | gengrp.c + gwbasic.h |
| 3 | gwsts 语句分支: SCREEN/PSET/PRESET/LINE (表达式坐标, LINE 排除 LINE INPUT 遮蔽) | gwsts.c |
| 4 | gweval POINT(x,y) 数字函数 dispatch (越界 -1) | gweval.c |
| 5 | test_gwsts_graphics.c (10 测试) + test_graphics.bas/.expected + 注册 | tests/ + run_bas_tests.sh |

### 实施中修的 2 个接线 bug

1. LINE 分支遮蔽 LINE INPUT (starts_with "LINE " 先于 LINE INPUT 分支命中)
   → file_seq 挂死 (LINE INPUT 失效, WHILE NOT EOF 恒真) — 加
   !starts_with("LINE INPUT ") 排除
2. gengrp_line_coords 旧代码用 g_gxpos 而非 x1 画线 (首坐标设置不同步) —
   统一 g_gxpos=x1; 非框线 draw(x1,y1,x2,y2)

### 新单测 (+30 → 3177)

gfx_screen / pset_point / pset_default / preset_default / pset_oob /
line_simple / line_bf / line_expr / line_default_start / point_oob

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3177/3177 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **74/74**（+test_graphics；test_file_seq 等 LINE INPUT 回归全绿） |
| 孤儿检测 | ✅ **836 = 836** |
| .bas 实证 | PSET-POINT/PSET-DEF/PRESET-DEF=1/1/0, OOB-SILENT, LINE-P1/P2/BF/EXPR/DEF=1, POINT-OOB=-1 |

---
## 上一轮: CODE_REVIEW v139 (复核确认轮, 0 问题) — IF 语义正式闭环

### 复核确认

| v138 修复 | v139 独立验证 |
|----------|--------------|
| MEDIUM-1 IF 复合串条件 (AND/OR 拆分 + AND 优先折叠) | ✅ probe R1N/R2Y/R3Y + 优先级 P1Y/P2N |
| MEDIUM-2 IF..THEN..ELSE | ✅ probe R4N (else 执行) / R5Y |
| 根因: 拼接链丢尾部 (S$=AX) | ✅ |
| 根因: 类型后缀变量不可读 (B=6/C=5) | ✅ |
| 根因: 自赋值清值 | ✅ |

### 独立验证结果（当前 checkout）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单元测试 | ✅ **3147/3147 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 集成套件 | ✅ **73/73 全 PASS, 0 跳过** |
| 孤儿检测 | ✅ **826=826** |

### 观察项（不计问题, 后续批次）

1. IF atom 上限 16 — 超限降级数字回退（实际条件极少超此规模）
2. `IF <cond> THEN <line#>` 行号跳转路径 — M2 IF 深化时验证
3. 后缀与数组/字段组合（`A%()`、`A!$`）+ 无括号字符串函数调用 — 类型系统审计收尾

---
## 上一轮: CODE_REVIEW v138 修复 — 2 MEDIUM + 暴露的 2 个既有根因

### 修复内容

| # | 问题 | 修复 | 文件 |
|---|------|------|------|
| MEDIUM-1 | IF 串复合条件 `"A"="A" AND "B"="C"` 错误 true（旧串比较只取第一个比较符，AND/OR 尾被丢弃） | 条件按顶层 AND/OR 分词（引号/括号感知, GW-BASIC AND 优先于 OR），逐 atom 求值（双侧串 → strcmp 真比较，否则数值），AND 组先折叠再 OR 合并 | gwsts.c |
| MEDIUM-2 | IF..THEN..ELSE 的 ELSE 分支静默不执行（既有缺口） | 引号/括号感知扫描顶层 " THEN "/" ELSE "，else_stmt 分支执行；was_in_handler 快照不影响 | gwsts.c |
| 暴露-1 | **自赋值 `S$ = S$ + "X"` 丢旧值**（probe 实证 LEN=1）— 既有根因 | assign_value 字符串路径先求值 RHS 再 alloc_variable（alloc 对已存在串变量清 ptr=NULL，旧顺序自引用读到 NULL）；数值路径本就先求值 | gwsts.c |
| 暴露-2 | **类型后缀变量 `A%`/`A!`/`A#` 表达式不可读**（parse 在 % 截断 → find_variable("A") 落空 → 恒 0）— 既有根因 | parse_factor 名字字符集补 %/!/# | gweval.c |
| 附带 | .expected 3 个文件系旧 bug 产物（test_gwdata 缺 A% 行/40-char 名字长度不等、test_int_var/let 记录 0、test_gwram 缺 concat PASS） | 按修正后真值更新 + test_gwdata.bas 变量名长度 bug 修正（28≠32） | tests/bas/ |
| 新增 | test_if_else.bas+.expected（ELSE/复合/函数比较 6 场景）+ run_bas_tests.sh 注册 | tests/bas/ + run_bas_tests.sh |

### 新单测 (+9, 3147 总)

if_compound_str_and / if_compound_str_or / if_else_both (+v137 轮 7 个 IF/STRING$ 用例后总计 18)

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3147/3147 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **73/73**（+test_if_else；test_gwdata/let/int_var/gwram 真值修正后全 PASS） |
| 孤儿检测 | ✅ **826 = 826** |
| probe 实证 | ✅ AND(真/假)/OR(假/真)/优先级 (T AND T) OR F=T；ELSE 双分支；自赋值 LEN 1→2→3；A%=100 IF/赋值 |

---
## 上一轮: M2 第 2 批第 1 项 — 字符串函数一致性审计 (复用 INSTR 模式, 8cad67d)

---

## 当前轮: CODE_REVIEW v137 修复 — 2 MEDIUM (类型判定/类型安全)

### 修复内容

| # | 问题 | 修复 | 文件 |
|---|------|------|------|
| MEDIUM-1 | STRING$(3, S$) 字符串变量参数 → 空串 | 第 2 参字母/引号开头 → 完整字符串解析 probe (v134 INSTR 三级判定模式); 失败回退数字形式 (STRING$(3, LEN("XY")) 仍可用) | gweval.c |
| MEDIUM-2 | 数字上下文字符串函数静默 0 (Y = CHR$(256)) | parse_factor: is_str_func → 消费参数至匹配 ')' → set_error(2) SNERR (类型不匹配); 覆盖全部 9 个字符串函数 | gweval.c |
| 附带 | IF 字符串比较假语义 (两侧 accident-0 → 恒真) | gwsts IF: 顶层比较符扫描 (引号/括号感知) + 双侧字符串 → strcmp 真语义; 否则数值路径 | gwsts.c |
| 附带 | test_gwdata.expected 缺 3 行 (LEFT$/RIGHT$/MID$ 比较旧行为被吞) | .expected 补真值 (真实串比较全 PASS) | .expected |

### 新单测 (+7, 3138 总)

string_vararg / string_funcarg / string_numericfuncarg / numeric_ctx_chr / numeric_ctx_left / numeric_ctx_if / strctx_chr_regression

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3138/3138 PASS** |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **72/72** (test_gwdata LEFT$/RIGHT$/MID$ 真串比较全 PASS) |
| 孤儿检测 | ✅ **823 = 823** |
| probe 实证 | ✅ STRING$(3,S$)→LEN=3; Y=CHR$(256)→Error@10; X=LEFT$→Error@10; IF CHR$(65)→Error@10; IF A$="X"→RIGHT-FALSE; IF LEFT$(...)=→FUNC-CMP-OK |

---
### ASM 语义依据 (BISTRS.ASM + GWEVAL.ASM)

| 函数 | ASM 证据 | 语义 |
|------|---------|------|
| CHR$ | CONINT (GWEVAL L1615-1621) | 参数 0..255, 高字节非 0 → FCERR |
| LEFT$/RIGHT$ | PREAM/GETBYT | n 为字节 0..255, 越界 → FCERR; n=0 → ""; n>len → 整串 |
| MID$ | L763-796 | start==0 → FCERR ("IT MUST NOT BE 0"); start/len 字节 0..255; 2-arg 形式 = 剩余串 |
| SPACE$/STRING$ | STRNG$ L658/SPACE$ | n 字节 0..255 → FCERR; STRING$ 第二参 GETYPR: 串 → 首字符, 数值 → CONINT |

### 修复内容

| # | 修复 | 文件 |
|---|------|------|
| 1 | CHR$/LEFT$/RIGHT$/MID$/SPACE$/STRING$ dispatch 范围校验 → set_error(5) (FCERR) | gweval.c |
| 2 | MID$ 2-arg 形式支持 (rest of string) | gweval.c |
| 3 | STRING$ 第二参类型 probe (引号 → 串形式取首字符; 否则数值) | gweval.c |
| 4 | 字符串字面量分配失败 → OM (ERR=14) + dispatch NULL → 错误感知 | gweval.c |
| 5 | assign_value: eval NULL + error 已置 → 不再回退字面量 (修复源码文本污染) | gwsts.c |
| 6 | PRINT: eval NULL + error → 中止 PRINT (不再数值回退打 " 0") | gwsts.c |
| 7 | **错误处理器内错误 = FATAL** (was_in_handler 快照区分 ERROR 语句合法进入) — 消灭 OM 无限循环 | gwmain.c + gwdata.c |
| 8 | test_gwram.expected 更新 (TEST 6 OM 传播新行为: Error at line 800) + TERMERROR_TESTS 注册 | .expected + run_bas_tests.sh |

### 验证结果（一轮全套，全绿）

| 验证项 | 结果 |
|--------|------|
| 干净重建 + 单测 | ✅ **3123/3123 PASS** (+11 strfuncs: chr 256/neg/ok, left neg/zero, right neg, mid start0/2arg, space range, string strarg/range) |
| 编译警告 | ✅ **0 条** |
| .bas 套件 | ✅ **72/72** (test_gwram → termerror 内容匹配; test_string .expected 修正 STRING$(10,"*")="**********" — 旧 expected 系 bug 产物) |
| 孤儿检测 | ✅ **816 = 816** |
| probe 实证 | ✅ CHR$(256)→Error@10; MID$ 2-arg→BCDE; STRING$(3,"AB")→AAA; OM handler→fatal (无 " 0 2" 死循环); test_error_stmt ERR=11 回归通过 |

### 附带修复 (v129 已知缺口部分闭合)

字符串堆耗尽 OM 传播: 字面量分配 → set_error(14); 错误处理器内再错 → FATAL 终止 (GW-BASIC 规则)。剩余: 非 PRINT 路径的 gwram_alloc NULL 全面审计 (中期项)。

---

## 下一步 (M2 第 2 批续)

| 优先级 | 任务 |
|--------|------|
| 1 | GENGRP 深化: PSET/PRESET/LINE (ASM 对照 + TDD) |
| 2 | 中期: gwram_alloc NULL 全面 OM 审计; INSTR 数字变量 start .bas 场景; CI 门禁 |

## 更新时间
2026-10-21

## 原则
严格按 PLAN_FRESH.md "每模块 7 步 TDD 工作流" 执行。实现前先查 ASM 源码定位；添加测试/实现前先查是否已存在。回归同时跑 mingw32-make test + ./tests/test_gwbasic.exe + run_bas_tests.sh。每逻辑组只跑一轮全套验证 (v131 教训)。
