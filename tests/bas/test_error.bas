' Test: ON ERROR / RESUME error handling
10 ON ERROR GOTO 100
20 PRINT "Before error"
30 X = 1 / 0
40 PRINT "After error (should not print)"
50 END
100 PRINT "Error handler: ERR = "; ERR
110 RESUME NEXT
120 PRINT "After RESUME NEXT"
130 END