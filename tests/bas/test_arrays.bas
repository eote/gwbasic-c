' Test: Arrays - DIM and array operations (known v60 baseline array corruption)
10 DIM A(10)
20 FOR I = 1 TO 10
30 A(I) = I * 2
40 NEXT I
50 FOR I = 1 TO 10
60 PRINT "I=";I;" A(I)=";A(I)
70 IF A(I) <> I * 2 THEN GOTO 100
80 NEXT I
90 PRINT "PASS"
95 END
100 PRINT "FAIL"
110 END
