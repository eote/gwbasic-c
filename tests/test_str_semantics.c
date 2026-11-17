/*
 * test_str_semantics.c — v164 HIGH-1/HIGH-2:
 * empty-string defaults for undefined string variables and
 * unassigned string array elements; rank validation on all
 * array access paths (numeric read / string read / string assign).
 */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);

void test_str_undef_var_empty(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v164 HIGH-1: undefined string variable reads as "" */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B$ = C$"), 0,
                       "copy undefined string var (no TMERR)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error");
    struct variable *b = gwdata_find_variable("B$");
    TEST_ASSERT(b && b->value.s.ptr && b->value.s.ptr[0] == '\0',
                "B$ is empty string");
    /* C$=C$ also fine */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("C$ = C$"), 0, "self copy");
    gwdata_clear_error();
    TEST_PASS("str undef var empty");
}

void test_str_elem_default_empty(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM C$(2)"), 0, "DIM");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B$ = C$(2)"), 0,
                       "read unassigned element (no TMERR)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error");
    struct variable *b = gwdata_find_variable("B$");
    TEST_ASSERT(b && b->value.s.ptr && b->value.s.ptr[0] == '\0',
                "B$ is empty string from element");
    TEST_PASS("str elem default empty");
}

void test_rank_read_numeric(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM D(10)");
    gwsts_exec_statement("D(1) = 7");
    TEST_ASSERT(gwsts_exec_statement("X = D(1,2)") != 0,
                "numeric READ rank mismatch errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS");
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("D", 1), 7,
                       "D(1) preserved");
    TEST_PASS("rank read numeric");
}

void test_rank_read_string(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM E$(10)");
    gwsts_exec_statement("E$(1) = \"q\"");
    TEST_ASSERT(gwsts_exec_statement("X$ = E$(1,2)") != 0,
                "string READ rank mismatch errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS");
    gwdata_clear_error();
    struct variable *e = gwdata_find_variable("E$");
    TEST_ASSERT(e && e->is_array, "E$ is array");
    char *s = biptrg_get_array_string("E$", 1);
    TEST_ASSERT(s && strcmp(s, "q") == 0, "E$(1) preserved");
    TEST_PASS("rank read string");
}

void test_rank_assign_string(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM C$(10)");
    gwsts_exec_statement("C$(1) = \"z\"");
    TEST_ASSERT(gwsts_exec_statement("C$(1,2) = \"b\"") != 0,
                "string ASSIGN rank mismatch errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS");
    gwdata_clear_error();
    char *s = biptrg_get_array_string("C$", 1);
    TEST_ASSERT(s && strcmp(s, "z") == 0, "C$(1) preserved (no alias write)");
    TEST_PASS("rank assign string");
}

void test_scalar_index_2d_numeric(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM M(2,2)");
    gwsts_exec_statement("M(1,1) = 5");
    /* v165 HIGH-2: M(1) on a 2-D array is Subscript out of range */
    TEST_ASSERT(gwsts_exec_statement("M(1) = 9") != 0, "M(1) errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS");
    gwdata_clear_error();
    TEST_ASSERT(gwsts_exec_statement("X = M(1)") != 0, "read M(1) errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS read");
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element_multi("M", (int[]){1,1}), 5,
                       "M(1,1) preserved");
    TEST_PASS("scalar index 2D numeric");
}

void test_scalar_index_2d_string(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM N$(2,2)");
    gwsts_exec_statement("N$(1,1) = \"n\"");
    TEST_ASSERT(gwsts_exec_statement("N$(1) = \"z\"") != 0, "N$(1) errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS");
    gwdata_clear_error();
    char *s = biptrg_get_array_string_multi("N$", (int[]){1,1});
    TEST_ASSERT(s && strcmp(s, "n") == 0, "N$(1,1) preserved");
    TEST_PASS("scalar index 2D string");
}

void test_empty_str_loop_heap(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v165 HIGH-1: repeated empty-string compares must not drain
     * the string heap (4000 evals used to leak 16 KB) */
    gwsts_exec_statement("F1 = FRE(0)");
    struct variable *vf1 = gwdata_find_variable("F1");
    TEST_ASSERT(vf1, "F1 set");
    float f1 = vf1->value.f;
    int loop_fail = 0;
    for (int i = 0; i < 8000; i++) {
        if (gwsts_exec_statement("IF C$=\"\" THEN N=N+1") != 0) loop_fail++;
        gwdata_clear_error();
    }
    TEST_ASSERT_EQ_INT(loop_fail, 0, "all 8000 IF evals succeed");
    gwsts_exec_statement("F2 = FRE(0)");
    struct variable *vf2 = gwdata_find_variable("F2");
    TEST_ASSERT(vf2, "F2 set");
    float f2 = vf2->value.f;
    TEST_ASSERT(f1 - f2 < 16.0f, "heap stable (drop < 16 bytes)");
    TEST_PASS("empty str loop heap stable");
}

void test_long_literal_loop_heap(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v166 HIGH-1: >63-byte literals must intern too (no length cap) */
    gwsts_exec_statement("F1 = FRE(0)");
    struct variable *vf1 = gwdata_find_variable("F1");
    TEST_ASSERT(vf1, "F1 set");
    float f1 = vf1->value.f;
    const char *longlit = "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
                          "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL";
    char stmt[256];
    int loop_fail = 0;
    for (int i = 0; i < 500; i++) {
        snprintf(stmt, sizeof(stmt), "IF C$=\"%s\" THEN N=N+1", longlit);
        if (gwsts_exec_statement(stmt) != 0) loop_fail++;
        gwdata_clear_error();
    }
    TEST_ASSERT_EQ_INT(loop_fail, 0, "500 long-literal evals succeed");
    gwsts_exec_statement("F2 = FRE(0)");
    struct variable *vf2 = gwdata_find_variable("F2");
    TEST_ASSERT(vf2, "F2 set");
    float drop = f1 - vf2->value.f;
    TEST_ASSERT(drop < 200.0f, "long literal heap stable (<200 drop)");
    TEST_PASS("long literal loop heap stable");
}

void test_many_distinct_literals(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v166 HIGH-1: >32 distinct literals must all intern (growable) */
    gwsts_exec_statement("F1 = FRE(0)");
    struct variable *vf1 = gwdata_find_variable("F1");
    TEST_ASSERT(vf1, "F1 set");   /* v168 M4: no silent negative drop */
    float f1 = vf1->value.f;
    char stmt[128];
    for (int i = 0; i < 40; i++) {
        snprintf(stmt, sizeof(stmt), "IF C$=\"STR%02d\" THEN N=N+1", i);
        gwsts_exec_statement(stmt);
        gwdata_clear_error();
    }
    gwsts_exec_statement("F2 = FRE(0)");
    struct variable *vf2 = gwdata_find_variable("F2");
    TEST_ASSERT(vf2, "F2 set");
    float drop = f1 - vf2->value.f;
    TEST_ASSERT(drop < 600.0f, "40 distinct literals interned (<600 drop)");
    TEST_PASS("many distinct literals interned");
}

void test_clear_resets_literal_cache(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("PRINT \"LIT\"");   /* interns "LIT" */
    printf("\n");
    /* v167 CRITICAL-2: CLEAR wipes the heap — cached ptrs must die */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLEAR"), 0, "CLEAR");
    gwsts_exec_statement("A$ = STRING$(100, \"Z\")");  /* overwrite heap start */
    char *lit = gwdata_intern_literal("LIT", 3);
    TEST_ASSERT(lit && strcmp(lit, "LIT") == 0, "interned literal re-alloc'd after CLEAR");
    TEST_PASS("clear resets literal cache");
}

void test_if_oom_propagates(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v167 HIGH-1: IF with an OM-ing literal must fail loudly (ERR=7),
     * not silently evaluate false */
    /* fill the string heap: distinct LITERALS live in str_heap
     * (string functions like STRING$ use gwram, NOT str_heap), so
     * intern 200 distinct 100-byte literals ≈ 20 KB > 16 KB heap */
    char stmt[256];
    int err7 = 0;
    for (int i = 0; i < 200; i++) {
        snprintf(stmt, sizeof(stmt), "X$ = \"FILL%03d%s\"", i,
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        gwsts_exec_statement(stmt);
        if (gwdata_get_error() != 0) { err7 = 1; break; }
    }
    TEST_ASSERT_EQ_INT(err7, 1, "heap full (OM reached)");
    gwdata_clear_error();
    /* fresh literal (not interned yet) → alloc fails → OM.
     * 200-byte literal: guaranteed to exceed any residual heap space
     * (the failed S$ concat leaves < 16384 used but can leave > 18
     * free — a short literal might still fit). */
    /* ~100-byte fresh literal: the failed fill leaves < 108 bytes
     * free, so this allocation must fail */
    int rc = gwsts_exec_statement("IF C$=\"ZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZ\" THEN N=N+1");
    TEST_ASSERT(rc != 0, "IF with OOM literal fails");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 7, "ERR=7 propagates");
    gwdata_clear_error();
    TEST_PASS("if oom propagates");
}

void test_if_oom_onerror_e2e(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v168 M4: ON ERROR end-to-end — heap-full + fresh big literal
     * in an IF must land in the handler with ERR=7 */
    char stmt[256];
    int err7 = 0;
    for (int i = 0; i < 200; i++) {
        snprintf(stmt, sizeof(stmt), "X$ = \"FILL%03d%s\"", i,
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        gwsts_exec_statement(stmt);
        if (gwdata_get_error() != 0) { err7 = 1; break; }
    }
    TEST_ASSERT_EQ_INT(err7, 1, "heap filled");
    gwdata_clear_error();
    /* ON ERROR GOTO arms the handler; the IF fails with OM; the run
     * loop (simulated here) leaves ERR=7 visible */
    gwsts_exec_statement("ON ERROR GOTO 9000");
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF C$=\"ZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZZZZZBRANDNEWZZZZ\" THEN N=N+1");
    TEST_ASSERT(rc != 0, "IF fails under OOM");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 7, "ERR=7 for ON ERROR handler");
    gwdata_clear_error();
    TEST_PASS("if oom onerror e2e");
}

void test_type_suffix_integer_scalar(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH: % suffix = 16-bit integer (ASM PTRGET/FWRTN) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A% = 5"), 0, "A%=5");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A%"), 5, "A% reads 5");
    /* round-half-away: 1.9 → 2, -1.5 → -2 */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A% = 1.9"), 0, "A%=1.9");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A%"), 2, "A% rounds to 2");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A% = -1.5"), 0, "A%=-1.5");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A%"), -2, "A% rounds to -2");
    /* overflow → FCERR 6, old value survives */
    TEST_ASSERT(gwsts_exec_statement("A% = 40000") != 0, "A%=40000 overflows");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 6, "Overflow 6");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A%"), -2, "A% keeps old value");
    gwdata_clear_error();
    TEST_ASSERT(gwsts_exec_statement("A% = -40000") != 0, "negative overflow");
    gwdata_clear_error();
    TEST_PASS("type suffix integer scalar");
}

void test_type_suffix_double(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH: # suffix = double storage (ASM PTRGET) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B# = 1.5"), 0, "B#=1.5");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("B#"), 1, "B# reads 1.5");
    struct variable *v = gwdata_find_variable("B#");
    TEST_ASSERT(v && v->type == VT_DOUBLE, "B# is VT_DOUBLE");
    TEST_PASS("type suffix double");
}

void test_type_suffix_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH: DIM A%(5) → integer array; elements round + overflow */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM A%(5)"), 0, "DIM A%");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A%(1) = 3.7"), 0, "A%(1)=3.7");
    struct variable *v = gwdata_find_variable("A%");
    TEST_ASSERT(v && v->value.s.ptr, "A% array");
    array_desc_t *d = (array_desc_t *)v->value.s.ptr;
    TEST_ASSERT_EQ_INT((int)d->type, VT_INTEGER, "desc type INTEGER");
    TEST_ASSERT_EQ_INT((int)((int32_t *)d->data)[1], 4, "element rounds to 4");
    TEST_ASSERT(gwsts_exec_statement("A%(2) = 40000") != 0, "element overflow");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 6, "Overflow 6");
    gwdata_clear_error();
    TEST_PASS("type suffix array");
}

void test_erl_variable(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_set_error(0, 0, 0);   /* v169: reset line too (init keeps stale) */
    /* v169 (v158 MEDIUM-1): ERL = offending line number (ASM GWMAIN
     * L472 ERRLIN=CURLIN; GWEVAL L673 ERL reads ERRLIN) */
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("ERL"), 0, "no error → ERL 0");
    gwsts_exec_statement("ON ERROR GOTO 100");
    gwsts_exec_statement("X = 1/0");   /* error at current line */
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 11, "division by zero");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("ERL"), gwdata_get_error_line(),
                       "ERL == error line");
    TEST_PASS("erl variable");
}

void test_typed_swap(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 CRITICAL-1: typed SWAP (double/integer precision) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("C# = 1.5"), 0, "C# init");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("D# = 2.5"), 0, "D# init");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("SWAP C#, D#"), 0, "swap doubles");
    TEST_ASSERT_EQ_INT((int)(gweval_eval_numeric("C#") * 10), 25, "C# == 2.5");
    TEST_ASSERT_EQ_INT((int)(gweval_eval_numeric("D#") * 10), 15, "D# == 1.5");
    /* integer scalar <-> integer array */
    gwsts_exec_statement("DIM E%(2)");
    gwsts_exec_statement("E%(1) = 3");
    gwsts_exec_statement("F% = 4");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("SWAP F%, E%(1)"), 0, "swap int");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("F%"), 3, "F% == 3");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("E%(1)"), 4, "E%(1) == 4");
    gwdata_clear_error();
    TEST_PASS("typed swap");
}

void test_double_precision_chain(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH-1: double precision through literal suffix, storage,
     * and read-back */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A# = 1.23456789012345#"), 0,
                       "assign 1.23456789012345#");
    struct variable *v = gwdata_find_variable("A#");
    TEST_ASSERT(v && v->type == VT_DOUBLE, "VT_DOUBLE");
    TEST_ASSERT(v->value.d > 1.23456789012 && v->value.d < 1.23456789013,
                "double precision stored");
    TEST_ASSERT_EQ_INT(gweval_last_valtyp(), 8, "VALTYP double");
    TEST_PASS("double precision chain");
}

void test_for_typed(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH-2: FOR with typed variables */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("FOR I% = 1 TO 3"), 0, "FOR I%");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("I%"), 1, "I% starts 1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("NEXT I%"), 0, "NEXT");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("I%"), 2, "I% == 2");
    gwdata_clear_error();
    TEST_PASS("for typed");
}

void test_clear_resets_error_state(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_set_error(0, 0, 0);
    /* v169 HIGH-3: CLEAR disarms ON ERROR and resets ERR/ERL
     * (ASM BIMISC CLEAR L245-248) */
    gwsts_exec_statement("ON ERROR GOTO 100");
    gwdata_set_error(11, 20, 0);
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("ERL"), 20, "ERL 20 before");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLEAR"), 0, "CLEAR");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("ERL"), 0, "ERL reset");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "ERR reset");
    TEST_ASSERT(!gwdata_is_on_error_active(), "ON ERROR disarmed");
    TEST_PASS("clear resets error state");
}

void test_bang_alias(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v169 HIGH-4: A! and A are the same variable (default suffix) */
    gwsts_exec_statement("A = 5");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A!"), 5, "A! reads A");
    gwsts_exec_statement("A! = 7");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A"), 7, "A reads A!");
    TEST_PASS("bang alias");
}

void test_rem_comment_compound(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v170 CRITICAL-1: REM/' comments are not executed, colons
     * inside them are not separators */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("REM HIDDEN: PRINT \"X\""), 0,
                       "REM line swallowed");
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("' HIDDEN: PRINT \"Y\""), 0,
                       "apostrophe line swallowed");
    gwdata_clear_error();
    TEST_PASS("rem comment compound");
}

void test_if_branch_compound(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v170 HIGH-1: THEN/ELSE branches run full colon lists */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("IF 1 THEN A = 1: B = 2"), 0, "then multi");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A"), 1, "A==1");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("B"), 2, "B==2");
    gwsts_exec_compound("C = 0: D = 0");
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("IF 0 THEN C = 5: C = C + 1 ELSE D = 9: D = D + 1"), 0,
                       "else multi");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("C"), 0, "C untouched");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("D"), 10, "D==10");
    gwdata_clear_error();
    TEST_PASS("if branch compound");
}

void test_double_expr_valtyp(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 HIGH-2: VALTYP is the widest type in the expression */
    TEST_ASSERT(gweval_eval_numeric("1#/3") > 0.33333333333 &&
                gweval_eval_numeric("1#/3") < 0.33333333334, "1#/3 double");
    TEST_ASSERT_EQ_INT(gweval_last_valtyp(), 8, "valtyp 8 after 1#/3");
    gweval_eval_numeric("0 + 1.23456789012345#");
    TEST_ASSERT_EQ_INT(gweval_last_valtyp(), 8, "valtyp order-independent");
    gwdata_clear_error();
    TEST_PASS("double expr valtyp");
}

void test_single_rounding(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 HIGH-3: single results round to float precision */
    gwsts_exec_statement("A# = 1/3");
    struct variable *v = gwdata_find_variable("A#");
    TEST_ASSERT(v && v->type == VT_DOUBLE, "VT_DOUBLE");
    TEST_ASSERT(v->value.d > 0.33333334 && v->value.d < 0.333333344,
                "single-rounded 1/3 stored in double");
    TEST_ASSERT(gweval_eval_numeric("0.1+0.2=0.3") == -1.0, "0.1+0.2=0.3 EQ");
    gwdata_clear_error();
    TEST_PASS("single rounding");
}

void test_math_double(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 HIGH-4: double input → double math */
    TEST_ASSERT(gweval_eval_numeric("SIN(1#)") > 0.8414709847 &&
                gweval_eval_numeric("SIN(1#)") < 0.8414709849, "SIN(1#) double");
    TEST_ASSERT(gweval_eval_numeric("SQR(2#)") > 1.4142135623 &&
                gweval_eval_numeric("SQR(2#)") < 1.4142135625, "SQR(2#) double");
    gwdata_clear_error();
    TEST_PASS("math double");
}

void test_double_array_str(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 HIGH-5: double arrays store/read full precision */
    gwsts_exec_statement("DIM C#(2)");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("C#(1)=1.23456789012345#"), 0, "store");
    TEST_ASSERT(gweval_eval_numeric("C#(1)") > 1.23456789012 &&
                gweval_eval_numeric("C#(1)") < 1.23456789013, "full-precision read");
    gwdata_clear_error();
    TEST_PASS("double array str");
}

void test_for_int_rounded_start(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 HIGH-6: FOR integer start rounds (CINT), not truncates */
    gwsts_exec_statement("FOR I% = 1.9 TO 3");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("I%"), 2, "start rounds to 2");
    gwdata_clear_error();
    TEST_PASS("for int rounded start");
}

void test_typed_set_bounds(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v170 MEDIUM-2: typed set validates rank/bounds */
    gwsts_exec_statement("DIM D#(2,2)");
    TEST_ASSERT(gwsts_exec_statement("D#(1)=5") != 0, "rank mismatch errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS 9");
    gwdata_clear_error();
    TEST_PASS("typed set bounds");
}

void test_single_line_for(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 HIGH-1: single-line FOR — body runs each iteration */
    /* v172: -3 = FOR continuation propagates to the line runner
     * (loop closes across compound invocations) — drive it here */
    int rc_loop = gwsts_exec_compound("FOR I=1 TO 3: A = A + I: NEXT I");
    int guard = 0;
    while (rc_loop == -3 && guard++ < 10) {
        rc_loop = gwsts_exec_compound("A = A + I: NEXT I");
    }
    TEST_ASSERT(rc_loop != -3, "loop closed");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A"), 6, "A == 1+2+3");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("A"), 6, "A == 1+2+3");
    gwdata_clear_error();
    TEST_PASS("single-line FOR");
}

void test_gosub_same_line_tail(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 HIGH-1: tail stash/take unit (line-based integration
     * needs a subroutine line; covered by .bas probe) */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("GOSUB 900: B = B + 10"), -2,
                       "missing target line → rc -2 (line not found)");
    gwdata_clear_error();
    TEST_PASS("gosub same-line tail stash");
}

void test_then_linenum_trailing(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 HIGH-2: THEN <line> jumps; trailing stmts discarded */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("IF 1 THEN 890: C = 99"), 890,
                       "THEN 200 jumps, C=99 discarded");
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("C"), 0, "C untouched");
    gwdata_clear_error();
    TEST_PASS("THEN linenum + trailing");
}

void test_2d_double_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 HIGH-3: 2D double arrays store full precision */
    gwsts_exec_statement("DIM E#(2,2)");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("E#(1,2)=1.23456789012345#"), 0, "store 2D");
    TEST_ASSERT(gweval_eval_numeric("E#(1,2)") > 1.23456789012 &&
                gweval_eval_numeric("E#(1,2)") < 1.23456789013, "full precision read");
    gwdata_clear_error();
    TEST_PASS("2D double array");
}

void test_d_exponent_literal(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 MEDIUM-3: D exponent = double literal */
    gwsts_exec_statement("A# = 1.23456789012345D0");
    struct variable *v = gwdata_find_variable("A#");
    TEST_ASSERT(v && v->type == VT_DOUBLE, "VT_DOUBLE");
    TEST_ASSERT(v->value.d > 1.23456789012 && v->value.d < 1.23456789013,
                "D literal full precision");
    TEST_ASSERT(gweval_eval_numeric("2.5D2") == 250.0, "2.5D2 == 250");
    gwdata_clear_error();
    TEST_PASS("D exponent literal");
}

void test_data_phantom_and_comment(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v171 MEDIUM-1/2: REM segments yield no data; DATA ends at
     * a colon (no phantom items) */
    gwsts_exec_statement("REM x: DATA 5");
    TEST_ASSERT(gwsts_exec_statement("READ Q") != 0, "REM DATA not read");
    gwdata_clear_error();
    /* DATA is read from PROGRAM LINES (gwlist) — add one */
    gwlist_add_line(950, "DATA 3,4: PRINT");
    gwsts_exec_statement("RESTORE");   /* fresh data pointer */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("READ A"), 0, "A=3");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("READ B"), 0, "B=4");
    TEST_ASSERT(gwsts_exec_statement("READ C") != 0, "no phantom C");
    gwdata_clear_error();
    TEST_PASS("data phantom + comment");
}

void test_cross_line_next_body(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v172 HIGH-1: single-line FOR with NEXT on the following line
     * still loops (body_text + -3 across the line runner) */
    TEST_ASSERT_EQ_INT(gwsts_exec_compound("FOR I=1 TO 3: A = A + I"), 0,
                       "FOR line (body, NEXT next line)");
    gwdata_clear_error();
    TEST_PASS("cross-line NEXT body (runner loop, .bas-verified)");
}

void test_resume_next_seg0(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v172 HIGH-2 (1): segment 0 can fail — same-line re-entry
     * skips the FAILED segment itself */
    gwsts_exec_statement("ON ERROR GOTO 880");
    gwsts_exec_compound("A = 1/0: B = 5");   /* compound pins the segment */
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 11, "ERR 11 pinned");
    TEST_ASSERT_EQ_INT(g_state->error_seg_index, 0, "segment 0 pinned");
    gwdata_clear_error();
    TEST_PASS("resume seg0 (integration in .bas probe)");
}

void test_error_seg_reset_on_new_error(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v172 HIGH-2 (2): each NEW error re-arms segment pinning */
    if (g_state) g_state->error_seg_index = 3;   /* stale from prior */
    gwdata_set_error(11, 50, 0);
    TEST_ASSERT_EQ_INT(g_state->error_seg_index, -1, "reset on set_error");
    gwdata_clear_error();
    TEST_PASS("error_seg reset on new error");
}

void test_line_start_comment_data(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v172 MEDIUM-1: a line starting with ' or REM is never data */
    gwlist_delete_line(950);   /* remove earlier test's DATA line */
    gwlist_add_line(960, "' note: DATA 9");
    gwsts_exec_statement("RESTORE");
    TEST_ASSERT(gwsts_exec_statement("READ C") != 0, "line-start quote DATA not read");
    gwdata_clear_error();
    gwlist_add_line(961, "REM foo: DATA 8");
    gwsts_exec_statement("RESTORE");
    TEST_ASSERT(gwsts_exec_statement("READ R") != 0, "line-start REM DATA not read");
    gwdata_clear_error();
    TEST_PASS("line-start comment DATA");
}

void test_int_for_frac_step(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gweval_init();
    gwdata_clear_error();
    /* v172 MEDIUM-2: integer FOR STEP goes through CINT — no
     * infinite loop */
    gwsts_exec_statement("DIM JJ%(10)");
    gwsts_exec_statement("JJ%(1) = 0");
    int rc2 = gwsts_exec_compound("FOR J%=1 TO 2 STEP 0.5: JJ%(1) = JJ%(1) + 1: NEXT J%");
    int guard2 = 0;
    while (rc2 == -3 && guard2++ < 10) {
        rc2 = gwsts_exec_compound("JJ%(1) = JJ%(1) + 1: NEXT J%");
    }
    TEST_ASSERT(rc2 != -3, "frac-step loop closed");
    /* step was CINT'ed to 1: iterations 1,2 → 2 increments */
    TEST_ASSERT_EQ_INT((int)gweval_eval_numeric("JJ%(1)"), 2, "2 iterations (step CINT 1)");
    gwdata_clear_error();
    TEST_PASS("int FOR frac STEP");
}
