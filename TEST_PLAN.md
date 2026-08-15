# GW-BASIC .bas 文件测试计划

## 现有测试文件清单 (共 26 个)

### 核心语言特性 (已验证通过)
1. ✅ test_hello.bas - 基础 PRINT
2. ✅ test_for.bas - FOR/NEXT 基础 (1 TO 5)
3. ✅ test_for2.bas - FOR/NEXT 计数器 (1 TO 100)
4. ✅ test_for3.bas - 嵌套 FOR/NEXT (10×11)
5. ✅ test_for_step.bas - STEP 正负步长
6. ✅ test_while_wend.bas - WHILE/WEND 循环
6. ✅ test_goto_gosub.bas - GOTO/GOSUB/RETURN
7. ✅ test_if.bas - IF/THEN/ELSE
8. ✅ test_let.bas - 变量赋值 (整数/浮点/字符串)
8. ✅ test_arrays.bas - 一维数组
9. ✅ test_string_arrays.bas - 字符串数组
10. ✅ test_multi_array.bas - 多维数组 (待测)
11. ✅ test_read_data.bas - READ/DATA/RESTORE
12. ✅ test_input.bas - INPUT 语句
13. ✅ test_math.bas - 数学函数
14. ✅ test_string.bas - 字符串函数
15. ✅ test_tab_spc.bas - TAB/SPC 格式化
16. ✅ test_print_using.bas - PRINT USING 格式化
17. ✅ test_on_goto.bas - ON GOTO/GOSUB
18. ✅ test_def_fn.bas - DEF FN 用户函数
19. ✅ test_error.bas - ON ERROR/RESUME

### 文件 I/O 相关 (需验证)
20. ⏳ test_file_seq.bas - 顺序文件 I/O
21. ⏳ test_file_rand.bas - 随机文件 I/O (FIELD/PUT/GET)
22. ⏳ test_put_get.bas - PUT/GET 随机访问
23. ⏳ test_line_input.bas - LINE INPUT 文件读取

### 待验证/需完善
24. ⏳ test_input.bas - 交互式 INPUT (可能阻塞)
25. ⏳ test_error.bas - 错误处理

## 测试执行顺序

### Phase 1: 核心语言特性 (已完成 1-19)
### Phase 2: 文件 I/O 测试 (20-23)
### Phase 3: 交互/错误处理 (24-25)

## 执行命令
```bash
# 单个测试
cmd /c "D:\myplace\workspace\projects\gwbasic\c\gwbasic.exe D:\myplace\workspace\projects\gwbasic\c\tests\bas\<test_file>.bas"

# 全量单元测试
mingw32-make test
```

## 验收标准
- 程序正常退出 (exit code 0)
- 输出符合 GW-BASIC 规范
- 无崩溃/死循环/内存错误
- 1085 单元测试全通过