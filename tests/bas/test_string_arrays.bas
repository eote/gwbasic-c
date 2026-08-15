' Test: String arrays
10 DIM S$(5)
20 FOR I = 1 TO 5
30 READ S$(I)
40 NEXT I
50 FOR I = 1 TO 5
60 PRINT I; S$(I)
70 NEXT I
80 DATA "Apple", "Banana", "Cherry", "Date", "Elderberry"
90 END