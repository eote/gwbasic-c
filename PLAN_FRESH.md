# GW-BASIC C Port - 全新开发计划

> **基于**：原始 Microsoft GW-BASIC 1983 ASM 源码 (39 模块) + 项目规范 (AGENTS.md)
> **原则**：严格 1:1 ASM 行为移植、TDD、零警告、MinGW-w64、中文响应

---

## 1. 项目范围与目标

| 维度 | 定义 |
|------|------|
| **核心目标** | 39 个 ASM 模块 1:1 对应 C 模块，行为完全一致 |
| **语言标准** | C11 (GCC/MinGW-w64) |
| **构建工具** | `mingw32-make` |
| **测试标准** | 单元测试 100% 通过、核心 .bas 集成测试全绿 |
| **质量门槛** | 零警告、零 CRITICAL/HIGH、单模块提交 |

---

## 2. 模块依赖拓扑 (按实现顺序)

```
GW-BASIC 核心层 (必须先完成)
├── GWDATA   → 变量表、字符串堆、程序缓冲区、运行时状态
├── GWRAM    → 内存管理 alloc/free/realloc/calloc
├── GWINIT   → 解释器初始化、常量 PI、数学模块 init
├── GWMAIN   → REPL 主循环、命令分发、行编辑
├── GWSTS    → 语句引擎 (DIM/LET/PRINT/IF/OPEN/CLOSE/LINE INPUT/GET/PUT/赋值/GOTO/GOSUB/RETURN/FOR/NEXT/WHILE/WEND/STOP/END)
├── GWEVAL   → 表达式解析器、数学函数、变量/数组/字符串操作
├── BIMISC   → NEW/CLEAR/RUN/RESTORE/STOP/END/CONT/TRON/TROFF/SWAP/ERASE/错误陷阱/栈管理
└── GENGRP   → LINE/PSET/PRESET/POINT/坐标扫描/图形初始化

I/O 子系统层 (可并行)
├── GIOCON   → 控制台 I/O (PRINT/LOCATE/COLOR/CLS) - Windows CONIO
├── GIOKYB   → 键盘输入 (INKEY$/INPUT$)
├── GIOSCN   → 屏幕模式/图形 (SCREEN/PSET/PRESET/POINT) - 帧缓冲模拟
├── SCNDRV   → 文本屏幕驱动 - 光标/换行/滚动/控制字符
├── GIOLPT   → 打印机 (LPRINT/LLIST/LPOS/WIDTH) - LPT1/2/3
├── GIOCOM   → 串口 (OPEN COM/baud/parity/data/stop/异步 I/O/超时)
├── GIODSK   → 磁盘 I/O 入口 - 封装 DSKCOM
├── GIOCAS   → 磁带 I/O (存根)
├── GIO86    → 8086 专用 I/O (存根)
└── GIOTBL   → I/O 表 - 设备名/分发表/初始化表

语言特性层 (依赖核心+I/O)
├── GWLIST   → 程序行存储/LIST/行增删/重编号
├── BISTRS   → 字符串函数 (LEFT$/RIGHT$/MID$/LEN/ASC/CHR$/STR$/VAL/STRING$/SPACE$/UCASE$/LCASE$)
├── BIPRTU   → PRINT USING 格式化 (完整格式符集)
├── BIPTRG   → 数组维度/数值字符串元素获取设置/GET/PUT 图形
├── MATH1    → 基础数学 (ABS/SGN/SQR/RND/INT/FIX)
├── MATH2    → 超越函数 (SIN/COS/TAN/ATN/LOG/EXP)
├── FIVEO    → 文件 I/O 语句 (合并入 DSKCOM)
├── DSKCOM   → 磁盘命令 (OPEN/CLOSE/PRINT#/INPUT#/LINE INPUT#/GET/PUT/SEEK/KILL/NAME)
└── ADVGRP   → 高级语句 (PAINT/CIRCLE/GET/PUT 图形/洪水填充)

OEM 抽象层 (最后)
├── OEM      → OEM 抽象 (CLREOL/SCROLL/SCRINP/NSETCX/NREAD/NWRITE/CLRSCN/视口)
├── OEMCBK   → Ctrl-Break/Pause 处理 (INT 1BH/15H)
├── OEMEV    → 事件 (ON KEY/STRIG/TIMER/POLLEV/手柄/光笔)
├── OEMSND   → 声音 (SOUND/PLAY) - Windows Beep API
├── IBMRES   → IBM PC 资源 (保留字/令牌/扩展令牌 FE/FD)
├── CALL86   → CALL 绝对/USR/DEF USR/CALLS
├── ITSA86   → 驻留初始化/内存映射/BASVAR/SEGOFF/命令行
├── NEXT86   → NEXT 循环/FOR-NEXT 栈/16层嵌套/STEP
├── KANJ86   → 日文支持 (存根返回语法错误)
├── MACLNG   → 宏语言驱动/命令解析/变量处理
├── SCNEDT   → 屏幕编辑器/行输入/光标/编辑键/EDIT/ERREDT
└── BIBOOT   → 启动/引导
```

---

## 3. 里程碑定义 (8 个里程碑，约 48 天)

| 里程碑 | 名称 | 包含模块 | 交付物 | 验收标准 |
|--------|------|----------|--------|----------|
| **M0** | 基础设施 | GWDATA/GWRAM/GWINIT/GWMAIN | 空解释器启动/退出 | 编译通过、运行即退出码 0 |
| **M1** | 核心引擎 | GWSTS/GWEVAL/BIMISC | 基础语句/表达式 | `PRINT 2+3*4`→`14`、IF/FOR/GOTO 正确 |
| **M2** | 程序与字符串 | GWLIST/GENGRP/BISTRS | 程序存储/图形/字符串 | `LIST`、`PSET`、`LEFT$/MID$` 正确 |
| **M3** | I/O 完整 | 10 个 I/O 模块 | 全 I/O 抽象层 | 顺序/随机文件、串口/打印机存根 |
| **M4** | 高级语言 | 数学/数组/PRINT USING/文件 | 完整语言特性 | SIN/COS、多维数组、USING、随机文件 |
| **M5** | OEM 与平台 | 7 个 OEM 模块 | 平台抽象层 | 图形/声音/事件/启动 实现/存根 |
| **M6** | 语言扩展 | DEF FN / ON ERROR/RESUME | 用户函数/错误处理 | 递归保护、参数作用域、错误捕获/恢复 |
| **M7** | 质量收敛 | 全测试/警告/文档 | 发布就绪 | 单测 100%、集成测全绿、零警告 |

---

## 4. 每模块 TDD 工作流 (标准化)

```
每模块执行步骤：
1. 读对应 .ASM 理解行为
2. **根据 ASM 代码逻辑编写单测** (tests/test_<module>.c) - 先红
   - 测试用例直接对应 ASM 代码路径、边界条件、错误码
   - 关键数据结构、标志位、寄存器用法在测试中验证
3. **根据 ASM 行为编写 .bas 集成测** (tests/bas/test_<feature>.bas + .expected)
   - 输入输出完全对标原版 GW-BASIC 语义
4. 实现 src/<module>.c - 至绿，严格复刻 ASM 逻辑
5. 运行 mingw32-make test - 全绿
6. 交互验证 ./gwbasic.exe 跑 .bas
7. 提交 + CODE_REVIEW 等待批准
```

---

## 5. 关键技术难点 (前置调研)

| 难点 | 所在模块 | 解决方案 |
|------|----------|----------|
| **MBF 浮点格式** | MATH1/MATH2/BIPRTU | 实现 MBF↔IEEE 转换、自定义数学库 |
| **PRINT USING 完整格式符** | BIPRTU | 单次遍历解析、状态机格式化器 |
| **随机文件 FIELD/GET/PUT** | DSKCOM/BIPTRG | 字段缓冲区管理、MSB-first 打包 |
| **FOR/NEXT 栈 16 层** | NEXT86/GWSTS | 独立栈结构、变量名匹配跳转 |
| **字符串堆/垃圾回收** | GWDATA/GWRAM | 碰撞指针/紧缩、field 变量 is_field 标记 |
| **OEM 图形模拟** | GIOSCN/OEM/ADVGRP | 640x200 帧缓冲、Windows GDI 映射 |
| **令牌化程序存储** | GWLIST/IBMRES | 文本存储 + 运行时压缩、LIST 解压 |
| **错误处理/ON ERROR** | GWSTS/BIMISC/GWMAIN | 错误码表、handler 栈、RESUME 类型分发 |

---

## 6. 测试体系

| 层级 | 工具 | 范围 | 门槛 |
|------|------|------|------|
| 单元 | 自制 test_runner | 每模块隔离 | 100% 通过 |
| 集成 | `tests/bas/*.bas` + `.expected` | 端到端 GW-BASIC 程序 | 全绿 |
| 回归 | `mingw32-make test` | 全量 | 零退化 |
| 交互 | 手动 `./gwbasic.exe` | REPL 可用性 | 无崩溃/死循环 |

---

## 7. Git 与提交规范

- **单模块一提交**：一个逻辑功能 = 一个 commit
- **提交信息**：`feat(module): 简述` / `fix(module): 简述`
- **CODE_REVIEW 必过**：Reviewer 读 CODE_REVIEW.md 后显式"批准合并"
- **禁止**：无审批 commit/push/restore/checkout

---

## 8. 执行入口

```
立即开始：M0 - GWDATA
  1. 读 GWDATA.ASM
  2. 写 tests/test_gwdata.c (变量表/字符串堆/程序缓冲区)
  3. 写 tests/bas/test_gwdata.bas (基础变量操作)
  4. 实现 src/gwdata.c + gwbasic.h 结构体定义
  4. 依次完成 GWRAM → GWINIT → GWMAIN
```

---

## 9. 风险与应对

| 风险 | 概率 | 影响 | 应对 |
|------|------|------|------|
| ASM 行为理解偏差 | 高 | 返工 | 每模块先写 .bas 对照原版 GW-BASIC (DOSBox) 验证 |
| MBF 数学精度 | 中 | 数值错误 | 专项单测覆盖边界值、对照 IEEE 754 差异文档 |
| 图形/声音平台差异 | 中 | 功能缺失 | OEM 层彻底抽象、存根优先、后续补全 |
| 测试维护成本 | 中 | 迟滞 | 统一 test_framework.h、自动化 run_bas_tests.sh |

---

*计划版本: v1.0 (全新设计) | 2026-09-04*
*基准: AGENTS.md + 39 个 .ASM 源文件*