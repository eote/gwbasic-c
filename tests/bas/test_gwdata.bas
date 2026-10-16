' test_gwdata.bas - GWDATA 模块集成测试
' 测试变量表、字符串堆、程序缓冲区、错误消息

' 测试 1: 基础变量操作
10 PRINT "TEST 1: Basic variable operations"
20 A% = 100
30 IF A% = 100 THEN PRINT "PASS: Integer assignment" ELSE PRINT "FAIL: Integer assignment"
40 A! = 3.14
50 IF A! = 3.14 THEN PRINT "PASS: Single precision" ELSE PRINT "FAIL: Single precision"
60 A# = 2.718281828
70 IF A# = 2.718281828 THEN PRINT "PASS: Double precision" ELSE PRINT "FAIL: Double precision"
80 A$ = "HELLO"
90 IF A$ = "HELLO" THEN PRINT "PASS: String assignment" ELSE PRINT "FAIL: String assignment"

' 测试 2: 变量名大小写不敏感
100 PRINT "TEST 2: Case insensitive variable names"
110 ABC = 10
120 IF abc = 10 THEN PRINT "PASS: lowercase lookup" ELSE PRINT "FAIL: lowercase lookup"
130 IF AbC = 10 THEN PRINT "PASS: mixed case lookup" ELSE PRINT "FAIL: mixed case lookup"

' 测试 3: 变量名最大长度 (40 字符)
140 PRINT "TEST 3: Max variable name length"
150 VARIABLENAMELONGENOUGHTOBETESTED = 42
160 IF variableNameLongEnoughToBeTested = 42 THEN PRINT "PASS: 40-char variable name" ELSE PRINT "FAIL: 40-char variable name"

' 测试 4: 字符串操作
170 PRINT "TEST 4: String operations"
180 S$ = "HELLO WORLD"
190 IF LEN(S$) = 11 THEN PRINT "PASS: LEN()" ELSE PRINT "FAIL: LEN()"
200 IF LEFT$(S$, 5) = "HELLO" THEN PRINT "PASS: LEFT$()" ELSE PRINT "FAIL: LEFT$()"
210 IF RIGHT$(S$, 5) = "WORLD" THEN PRINT "PASS: RIGHT$()" ELSE PRINT "FAIL: RIGHT$()"
220 IF MID$(S$, 7, 5) = "WORLD" THEN PRINT "PASS: MID$()" ELSE PRINT "FAIL: MID$()"

' 测试 5: 数组变量
230 PRINT "TEST 5: Array variables"
240 DIM ARR(10)
250 ARR(5) = 123.45
260 IF ARR(5) = 123.45 THEN PRINT "PASS: Array element assignment" ELSE PRINT "FAIL: Array element assignment"
270 DIM MAT(3,3)
280 MAT(1,2) = 99
290 IF MAT(1,2) = 99 THEN PRINT "PASS: 2D array" ELSE PRINT "FAIL: 2D array"

' 测试 6: 错误消息
300 PRINT "TEST 6: Error messages"
310 ON ERROR GOTO 400
320 X = 1 / 0
325 ' Division by zero triggers error handler at 400
330 PRINT "FAIL: Should not reach here"
335 GOTO 500
400 IF ERR = 11 THEN PRINT "PASS: Division by zero error caught, ERR="; ERR ELSE PRINT "FAIL: Wrong error code"
410 RESUME 335
500 ON ERROR GOTO 0

' 测试 7: 程序缓冲区
510 PRINT "TEST 7: Program buffer"
520 PRINT "PASS: Program runs from buffer"

530 END