' Test: DEF FN user-defined functions
10 DEF FN SQUARE(X) = X * X
20 DEF FN CUBE(X) = X * X * X
30 DEF FN AVG(X, Y) = (X + Y) / 2
40 PRINT "SQUARE(5) = "; FN SQUARE(5)
50 PRINT "CUBE(3) = "; FN CUBE(3)
60 PRINT "AVG(10, 20) = "; FN AVG(10, 20)
70 END