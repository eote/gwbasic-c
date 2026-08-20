' Test: String functions
10 A$ = "HELLO"
20 PRINT "LEN: "; LEN(A$)
30 PRINT "LEFT$: "; LEFT$(A$, 2); ","; LEFT$(A$, 5)
40 PRINT "RIGHT$: "; RIGHT$(A$, 2); ","; RIGHT$(A$, 5)
50 PRINT "MID$: "; MID$(A$, 2, 3); ","; MID$(A$, 1, 2)
60 PRINT "UCASE$: "; UCASE$("hello")
70 PRINT "LCASE$: "; LCASE$("WORLD")
80 PRINT "STR$: "; STR$(123.45)
90 PRINT "VAL: "; VAL("456.78")
100 PRINT "CHR$: "; CHR$(65)
110 PRINT "CHR$: "; CHR$(66)
120 END