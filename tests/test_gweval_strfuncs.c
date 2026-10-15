/* test_gweval_strfuncs.c — string function consistency audit (v137)
 * ASM semantics per BISTRS.ASM + GWEVAL.ASM:
 * - CHR$: CONINT → 0..255 else FCERR
 * - LEFT$/RIGHT$: GETBYT byte arg → 0..255 else FCERR
 * - MID$: start 0 → FCERR (L766 "IT MUST NOT BE 0"), start 0..255;
 *   2-arg form = rest of string; len byte 0..255
 * - SPACE$/STRING$: n byte 0..255 else FCERR; STRING$ arg2 string → first char
 */
#include <stdio.h>
#include <string.h>
#include "../src/gwbasic.h"
#include "test_framework.h"

/* Test: CHR$(256) → FCERR (ASM CONINT high byte != 0) */
void test_gweval_strfuncs_chr_256_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = CHR$(256)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "CHR$(256) sets FCERR (ERR=5)");
    TEST_PASS("gweval chr 256 fcerr");
}

/* Test: CHR$(-1) → FCERR */
void test_gweval_strfuncs_chr_neg_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = CHR$(-1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "CHR$(-1) sets FCERR");
    TEST_PASS("gweval chr neg fcerr");
}

/* Test: CHR$(65) = "A" (regression, no error) */
void test_gweval_strfuncs_chr_ok(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = CHR$(65)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "CHR$(65) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 1, "CHR$(65) len 1");
    TEST_ASSERT_EQ_INT((unsigned char)a->value.s.ptr[0], 65, "CHR$(65) = 'A'");
    TEST_PASS("gweval chr ok");
}

/* Test: LEFT$("AB", -1) → FCERR (ASM GETBYT/CONINT) */
void test_gweval_strfuncs_left_neg_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = LEFT$(\"AB\", -1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "LEFT$(\"AB\",-1) sets FCERR");
    TEST_PASS("gweval left neg fcerr");
}

/* Test: LEFT$("AB", 0) = "" (legal) */
void test_gweval_strfuncs_left_zero_ok(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = LEFT$(\"AB\", 0)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "LEFT$(\"AB\",0) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 0, "LEFT$(\"AB\",0) empty");
    TEST_PASS("gweval left zero ok");
}

/* Test: RIGHT$("AB", -1) → FCERR */
void test_gweval_strfuncs_right_neg_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = RIGHT$(\"AB\", -1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "RIGHT$(\"AB\",-1) sets FCERR");
    TEST_PASS("gweval right neg fcerr");
}

/* Test: MID$(s, 0, 1) → FCERR (ASM L766: start must not be 0) */
void test_gweval_strfuncs_mid_start0_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = MID$(\"AB\", 0, 1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "MID$(\"AB\",0,1) sets FCERR");
    TEST_PASS("gweval mid start0 fcerr");
}

/* Test: MID$(s, start) 2-arg form → rest of string */
void test_gweval_strfuncs_mid_2arg_rest(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = MID$(\"ABCDE\", 2)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "MID$(\"ABCDE\",2) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 4, "MID$ 2-arg rest len 4");
    TEST_ASSERT_EQ_STR(a->value.s.ptr, "BCDE", "MID$(\"ABCDE\",2) = BCDE");
    TEST_PASS("gweval mid 2arg rest");
}

/* Test: SPACE$(-1) → FCERR; SPACE$(256) → FCERR; SPACE$(2) = 2 spaces */
void test_gweval_strfuncs_space_range(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = SPACE$(-1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "SPACE$(-1) sets FCERR");

    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = SPACE$(256)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "SPACE$(256) sets FCERR");

    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = SPACE$(2)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "SPACE$(2) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 2, "SPACE$(2) len 2");
    TEST_PASS("gweval space range");
}

/* Test: STRING$(3, "AB") = "AAA" (ASM GETYPR: string arg → first char) */
void test_gweval_strfuncs_string_strarg(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = STRING$(3, \"AB\")");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "STRING$(3,\"AB\") no error");
    TEST_ASSERT_EQ_STR(a->value.s.ptr, "AAA", "STRING$(3,\"AB\") = AAA");
    TEST_PASS("gweval string strarg");
}

/* Test: STRING$(3, S$) string variable (v137 MEDIUM-1) */
void test_gweval_strfuncs_string_vararg(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *s = gwdata_alloc_variable("S$", VT_STRING);
    TEST_ASSERT(s, "S$ allocated");
    s->value.s.ptr = gwdata_alloc_string(2);
    if (s->value.s.ptr) {
        strcpy(s->value.s.ptr, "B");
        s->value.s.len = 1;
    }
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    TEST_ASSERT(a, "A$ allocated");
    gwsts_exec_statement("A$ = STRING$(3, S$)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "STRING$(3,S$) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 3, "STRING$(3,S$) len 3");
    TEST_ASSERT_EQ_STR(a->value.s.ptr, "BBB", "STRING$(3,S$) = BBB");
    TEST_PASS("gweval string vararg");
}

/* Test: STRING$(3, LEFT$("CD",1)) string function arg (v137 MEDIUM-1) */
void test_gweval_strfuncs_string_funcarg(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    TEST_ASSERT(a, "A$ allocated");
    gwsts_exec_statement("A$ = STRING$(3, LEFT$(\"CD\", 1))");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "STRING$(3,LEFT$) no error");
    TEST_ASSERT_EQ_STR(a->value.s.ptr, "CCC", "STRING$(3,LEFT$(\"CD\",1)) = CCC");
    TEST_PASS("gweval string funcarg");
}

/* Test: STRING$(3, LEN(A$)) numeric func still works (MEDIUM-1 regression) */
void test_gweval_strfuncs_string_numericfuncarg(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    TEST_ASSERT(a, "A$ allocated");
    gwsts_exec_statement("A$ = STRING$(3, LEN(\"XY\"))");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "STRING$(3,LEN) no error");
    TEST_ASSERT_EQ_INT((unsigned char)a->value.s.ptr[0], 2, "STRING$(3,LEN(\"XY\")) = chr(2)");
    TEST_PASS("gweval string numericfuncarg");
}

/* Test: Y = CHR$(256) numeric context (v137 MEDIUM-2) — type mismatch SNERR */
void test_gweval_strfuncs_numeric_ctx_chr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("Y = CHR$(256)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 2, "Y = CHR$(256) sets SNERR (type mismatch)");
    TEST_PASS("gweval numeric ctx chr");
}

/* Test: X = LEFT$("AB",1) numeric context — SNERR */
void test_gweval_strfuncs_numeric_ctx_left(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("X = LEFT$(\"AB\", 1)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 2, "X = LEFT$(\"AB\",1) sets SNERR");
    TEST_PASS("gweval numeric ctx left");
}

/* Test: IF CHR$(65) THEN — numeric context via condition — SNERR */
void test_gweval_strfuncs_numeric_ctx_if(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("IF CHR$(65) THEN Z = 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 2, "IF CHR$(65) THEN sets SNERR");
    TEST_PASS("gweval numeric ctx if");
}

/* Test: A$ = CHR$(256) string context still FCERR=5 (MEDIUM-2 regression) */
void test_gweval_strfuncs_strctx_chr_regression(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = CHR$(256)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "A$ = CHR$(256) still FCERR (string ctx)");
    TEST_PASS("gweval strctx chr regression");
}

/* Test: STRING$(256, 65) → FCERR; STRING$(0, 65) = "" */
void test_gweval_strfuncs_string_range(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("A$ = STRING$(256, 65)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "STRING$(256,65) sets FCERR");

    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    struct variable *a = gwdata_alloc_variable("A$", VT_STRING);
    gwsts_exec_statement("A$ = STRING$(0, 65)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "STRING$(0,65) no error");
    TEST_ASSERT_EQ_INT(a->value.s.len, 0, "STRING$(0,65) empty");
    TEST_PASS("gweval string range");
}
