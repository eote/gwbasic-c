/*
 * test_assign_strict2.c — v163: old-value preservation on failed
 * string assignment, raw literal semantics for READ/INPUT, read-path
 * auto-DIM and rank-mismatch checks.
 */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);
extern int assign_value_raw(const char *var_ref, const char *expr);

void test_strict_preserves_old_value(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("A$ = \"old\"");
    /* v163 CRITICAL-1: failed assignment must keep "old" */
    TEST_ASSERT(gwsts_exec_statement("A$ = 5") != 0, "A$=5 errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR");
    gwdata_clear_error();
    struct variable *v = gwdata_find_variable("A$");
    TEST_ASSERT(v && v->value.s.ptr && strcmp(v->value.s.ptr, "old") == 0,
                "A$ keeps 'old' after failed assign");
    gwsts_exec_statement("A$ = \"old2\"");
    TEST_ASSERT(gwsts_exec_statement("A$ = CHR$(256)") != 0, "CHR$(256) errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "FCERR");
    gwdata_clear_error();
    v = gwdata_find_variable("A$");
    TEST_ASSERT(v && v->value.s.ptr && strcmp(v->value.s.ptr, "old2") == 0,
                "A$ keeps 'old2' after failed CHR$");
    TEST_PASS("strict preserves old value");
}

void test_raw_is_literal_not_expr(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("A$ = \"SECRET\"");
    /* v163 HIGH-1: raw mode never evaluates the text */
    TEST_ASSERT_EQ_INT(assign_value_raw("B$", "A$"), 0, "raw A$ text");
    struct variable *b = gwdata_find_variable("B$");
    TEST_ASSERT(b && b->value.s.ptr && strcmp(b->value.s.ptr, "A$") == 0,
                "B$ == literal 'A$' (not SECRET)");
    TEST_ASSERT_EQ_INT(assign_value_raw("C$", "CHR$(65)"), 0, "raw CHR$(65)");
    b = gwdata_find_variable("C$");
    TEST_ASSERT(b && b->value.s.ptr && strcmp(b->value.s.ptr, "CHR$(65)") == 0,
                "C$ == literal 'CHR$(65)'");
    TEST_ASSERT_EQ_INT(assign_value_raw("D$", "1+2"), 0, "raw 1+2");
    b = gwdata_find_variable("D$");
    TEST_ASSERT(b && b->value.s.ptr && strcmp(b->value.s.ptr, "1+2") == 0,
                "D$ == literal '1+2'");
    TEST_PASS("raw literal semantics");
}

void test_autodim_read_path(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v163 HIGH-2: first READ auto-DIMs 2-D (no 1-D alias) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("X = A(1,2)"), 0, "first read");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A(1,1) = 1"), 0, "set 1,1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A(1,2) = 5"), 0, "set 1,2");
    float v1 = biptrg_get_array_element_multi("A", (int[]){1, 1});
    float v2 = biptrg_get_array_element_multi("A", (int[]){1, 2});
    TEST_ASSERT_EQ_INT((int)v1, 1, "A(1,1) == 1");
    TEST_ASSERT_EQ_INT((int)v2, 5, "A(1,2) == 5 (no alias)");
    TEST_PASS("autodim read path");
}

void test_rank_mismatch_errbs(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("DIM B(10)");
    gwsts_exec_statement("B(1) = 1");
    TEST_ASSERT(gwsts_exec_statement("B(1,2) = 5") != 0,
                "1-D array with 2 subscripts errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS = 9");
    gwdata_clear_error();
    float v = biptrg_get_array_element("B", 1);
    TEST_ASSERT_EQ_INT((int)v, 1, "B(1) preserved (no alias write)");
    TEST_PASS("rank mismatch ERRBS");
}
