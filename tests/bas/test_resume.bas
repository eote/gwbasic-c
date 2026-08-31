10 ON ERROR GOTO 100
20 PRINT "Before error"
30 X = 1 / 0
40 PRINT "After error - should not print"
50 END
100 PRINT "Error handler, ERR="; ERR
110 X = 1
120 RESUME
130 PRINT "After RESUME - X is now"; X
140 END
