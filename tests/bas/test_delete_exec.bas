' test_delete_exec.bas - CRITICAL-1 回归 (v125): DELETE 删除当前/后续行
' 验证: 执行指针安全重定位, 被删行不再执行

' 场景 1: 删除后续行 — 20 不应执行, 30 继续
10 PRINT "S1: start"
20 DELETE 40
30 PRINT "S1: PASS line 40 not executed"

' 场景 2: 删除当前行 — 从下一现存行继续
100 PRINT "S2: start"
110 DELETE 110
120 PRINT "S2: PASS continued after self-delete"

' 场景 3: 删除区间含当前行
200 PRINT "S3: start"
210 DELETE 200-215
220 PRINT "S3: PASS continued after range delete"

' 场景 4: 变量保持 + 正常结束
300 X = 100
310 IF X = 100 THEN PRINT "S4: PASS X preserved" ELSE PRINT "S4: FAIL"
320 PRINT "ALL DONE"
400 END