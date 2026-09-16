10 PRINT "TEST 1: Memory query functions"
20 PRINT "FRE(0) ="; FRE(0)
30 PRINT "FRE(str) ="; FRE("")
40 IF FRE(0) > 0 THEN PRINT "PASS: FRE(0) returns memory" ELSE PRINT "FAIL: FRE(0)"

100 PRINT "TEST 2: String memory allocation"
110 FOR I = 1 TO 100
120   S$ = S$ + "X"
130 NEXT I
140 IF LEN(S$) = 100 THEN PRINT "PASS: String concatenation" ELSE PRINT "FAIL: String concat"
150 PRINT "FRE after strings:"; FRE("")

200 PRINT "TEST 3: Array memory allocation"
210 DIM A(100)
220 FOR I = 0 TO 100
230   A(I) = I * 1.5
240 NEXT I
250 IF A(50) = 75 THEN PRINT "PASS: Array assignment" ELSE PRINT "FAIL: Array assignment"
260 PRINT "FRE after array:"; FRE(0)

300 PRINT "TEST 4: CLEAR statement"
310 X = 100
320 CLEAR
330 IF X = 0 THEN PRINT "PASS: CLEAR resets variables" ELSE PRINT "FAIL: CLEAR"

400 PRINT "TEST 5: Large array"
410 ON ERROR GOTO 600
420 DIM BIG(10000)
430 BIG(5000) = 3.14
440 IF BIG(5000) = 3.14 THEN PRINT "PASS: Large array" ELSE PRINT "FAIL: Large array"
450 GOTO 700
600 PRINT "FAIL: Out of memory on large array"
610 RESUME NEXT

700 PRINT "TEST 6: String space exhaustion"
710 ON ERROR GOTO 800
720 S$ = ""
730 FOR I = 1 TO 10000
740   S$ = S$ + STRING$(100, "A")
750 NEXT I
760 GOTO 900
800 PRINT "PASS: String space error caught, ERR="; ERR
810 RESUME NEXT

900 END