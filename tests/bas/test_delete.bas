' test_delete.bas - M1: DELETE 语句集成测试 (v124 短期建议-1)
' 验证 DELETE 语句实际运行 + 程序继续

10 PRINT "TEST 1: DELETE single line"
20 A = 1
30 DELETE 20
40 PRINT "after DELETE 20"

100 PRINT "TEST 2: DELETE range"
110 B = 2
120 C = 3
130 DELETE 110-120
140 PRINT "after DELETE 110-120"

200 PRINT "TEST 3: program continues"
210 X = 100
220 IF X = 100 THEN PRINT "PASS: X preserved" ELSE PRINT "FAIL: X"

300 PRINT "ALL DONE"
400 END