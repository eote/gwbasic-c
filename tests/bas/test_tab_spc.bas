' Test: PRINT formatting with TAB and SPC
10 PRINT TAB(10); "Column 10"
20 PRINT "Start"; SPC(5); "After 5 spaces"
30 FOR I = 1 TO 5
40 PRINT TAB(I * 5); I
50 NEXT I
60 END