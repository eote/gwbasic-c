# GW-BASIC Test Categories

This document categorizes the `.bas` integration tests by feature implementation status.

## ✅ Implemented Features (Tests Should PASS)

### Core Language
| Test | Feature |
|------|---------|
| test_hello.bas | Basic PRINT |
| test_basic.bas | Basic PRINT |
| test_simple.bas | Simple PRINT |
| test_empty_stmt.bas | Empty statements |

### Variables & Assignment
| Test | Feature |
|------|---------|
| test_let.bas | LET statement, multiple types |
| test_assign.bas | Variable assignment |
| test_assign2.bas | Cross assignment |
| test_cross_assign.bas | Variable cross-assignment |
| test_expr.bas | Expression evaluation |
| test_expr2.bas | Expression with variable update |
| test_self_assign.bas | Self-assignment (I = I + 1) |
| test_simple_assign.bas | Simple assignment |
| test_scalar.bas | Scalar variable |
| test_int_var.bas | Integer variable (I%) |

### Control Flow - FOR/NEXT
| Test | Feature |
|------|---------|
| test_for.bas | Basic FOR/NEXT |
| test_for2.bas | FOR to 100 |
| test_for3.bas | Nested FOR |
| test_for_step.bas | FOR with STEP (+/-) |
| test_next_no_var.bas | NEXT without variable |

### Control Flow - GOTO/GOSUB
| Test | Feature |
|------|---------|
| test_goto_gosub.bas | GOTO and GOSUB/RETURN |

### Control Flow - IF/THEN/ELSE
| Test | Feature |
|------|---------|
| test_if.bas | IF/THEN/ELSE |
| test_if2.bas | IF/THEN/ELSE variants |
| test_cond.bas | Comparison operators (-1/0 boolean) |

### Arrays
| Test | Feature |
|------|---------|
| test_arrays.bas | 1D array DIM, assign, read |
| test_multi_array.bas | 2D array (5×5) read/write |
| test_string_arrays.bas | String array DIM, READ/DATA |

### Math Functions
| Test | Feature |
|------|---------|
| test_math.bas | SIN, COS, TAN, ATN, EXP, LOG, SQR, ABS, SGN, INT, FIX, RND |

### File I/O (Sequential)
| Test | Feature |
|------|---------|
| test_print_file.bas | PRINT # / LINE INPUT # |
| test_file_seq.bas | Sequential file I/O |
| test_line_input.bas | LINE INPUT with file |
| test_read_data.bas | READ/DATA/RESTORE |

### File I/O (Random Access)
| Test | Feature |
|------|---------|
| test_file_rand.bas | Random file with FIELD/PUT/GET |
| test_put_get.bas | PUT/GET random access |

### PRINT Formatting
| Test | Feature |
|------|---------|
| test_print_bare.bas | Bare PRINT (newline) |
| test_print_hello.bas | PRINT literal |
| test_print_literal.bas | PRINT literal |
| test_tab_spc.bas | TAB() and SPC() |

## ⏳ Not Yet Implemented (Tests Expected to FAIL)

### String Functions (All return 0/empty)
| Test | Missing Feature |
|------|-----------------|
| test_string.bas | LEN, LEFT$, RIGHT$, MID$, UCASE$, LCASE$, STR$, VAL, CHR$, ASC, SPACE$, STRING$ |
| test_string_funcs.bas | String functions |
| test_string_simple.bas | LEN, LEFT$ |
| test_string_var.bas | String variable + LEN |
| test_len.bas | LEN |
| test_len2.bas | LEN with variable |
| test_len_literal.bas | LEN literal |
| test_left.bas | LEFT$ |
| test_left_assign.bas | LEFT$ assignment |
| test_left_direct.bas | LEFT$ direct |
| test_left_direct2.bas | LEFT$ direct |
| test_left_literal.bas | LEFT$ literal |
| test_left_literal2.bas | LEFT$ literal |
| test_left_only.bas | LEFT$ only |
| test_left_print.bas | LEFT$ in PRINT |
| test_left_simple.bas | LEFT$ simple |

### DEF FN User Functions
| Test | Missing Feature |
|------|-----------------|
| test_def_fn.bas | DEF FN / FN |

### Error Handling
| Test | Missing Feature |
|------|-----------------|
| test_error.bas | ON ERROR / RESUME |

### ON GOTO/GOSUB
| Test | Missing Feature |
|------|-----------------|
| test_on_goto.bas | ON ... GOTO |

### PRINT USING
| Test | Missing Feature |
|------|-----------------|
| test_using_simple.bas | PRINT USING basic |
| test_using_very_simple.bas | PRINT USING simple |
| test_print_using.bas | PRINT USING full |

### WHILE/WEND
| Test | Missing Feature |
|------|-----------------|
| test_while_wend.bas | WHILE/WEND (marked future) |
| test_while_debug.bas | WHILE debug |

## 📝 Notes

- **Total .bas tests**: ~60
- **Implemented category**: ~30 tests (should PASS)
- **Not implemented category**: ~30 tests (expected to FAIL with syntax errors or wrong output)

## Running Tests

```bash
# Run all implemented tests (should all pass)
for f in tests/bas/test_{hello,for,if,let,goto_gosub,for_step,next_no_var,simple,scalar,empty_stmt,print_bare,print_hello,print_literal,basic,assign,assign2,cross_assign,expr,expr2,self_assign,simple_assign,for2,for3,int_var,arrays,multi_array,math,cond,print_file,file_seq,line_input,put_get,read_data,tab_spc,string_arrays}.bas; do
    ./gwbasic.exe "$f" 2>&1 | grep -v DEBUG
done

# Check not-implemented tests (expected to show syntax errors)
for f in tests/bas/test_{def_fn,error,on_goto,string,string_funcs,string_simple,string_var,len,len2,len_literal,left,left_assign,left_direct,left_direct2,left_literal,left_literal2,left_only,left_print,left_simple,using_simple,using_very_simple,print_using,while_wend,while_debug}.bas; do
    ./gwbasic.exe "$f" 2>&1 | grep -v DEBUG | head -5
done
```

## Adding New Tests

When implementing a new feature:
1. Move relevant test(s) from "Not Implemented" to "Implemented" section
2. Verify test passes
3. Update this document