' test_if_else.bas — IF..THEN..ELSE + compound conditions (v138)
10 IF 1 = 2 THEN PRINT "bad-then" ELSE PRINT "else-numeric"
20 IF 2 = 2 THEN PRINT "then-numeric" ELSE PRINT "bad-else"
30 IF "A" = "A" AND "B" = "C" THEN PRINT "bad-and" ELSE PRINT "and-false"
40 IF "A" = "A" OR "B" = "C" THEN PRINT "or-true" ELSE PRINT "bad-or"
50 IF LEFT$("HELLO", 5) = "HELLO" THEN PRINT "func-then" ELSE PRINT "bad-func"
60 A$ = "X"
70 IF A$ = "X" THEN PRINT "var-then" ELSE PRINT "bad-var"
80 END
