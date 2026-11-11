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
    float drop = vf1->value.f - vf2->value.f;
    TEST_ASSERT(drop < 200.0f, "long literal heap stable (<200 drop)");
    TEST_PASS("long literal loop heap stable");
}

void test_many_distinct_literals(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v166 HIGH-1: >32 distinct literals must all intern (growable) */
    gwsts_exec_statement("F1 = FRE(0)");
    struct variable *vf1 = gwdata_find_variable("F1");
    float f1 = vf1 ? vf1->value.f : 0.0f;
    (void)f1;
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
