10 ON ERROR GOTO 100
20 PRINT "Before error"
30 X = 1 / 0
40 PRINT "Should not print"
50 END
100 PRINT "Error handler, ERR="; ERR
110 ON ERROR GOTO 0
120 X = 1 / 0
130 PRINT "Should not print either"
140 END
