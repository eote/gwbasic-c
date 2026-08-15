' Test: Multi-dimensional arrays
10 DIM A(5, 5)
20 FOR I = 1 TO 5
30 FOR J = 1 TO 5
40 A(I, J) = I * 10 + J
50 NEXT J
60 NEXT I
70 FOR I = 1 TO 5
80 FOR J = 1 TO 5
90 PRINT "A(";I;",";J;") = "; A(I, J)
100 NEXT J
110 NEXT I
120 END