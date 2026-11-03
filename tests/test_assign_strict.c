/*
 * test_assign_strict.c — v162 HIGH-1/HIGH-2/M5 regressions:
 * strict string assignment (TMERR), auto-DIM multi-dimension,
 * SWAP out-of-range aborts before write.
 */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);

void test_assign_strict_scalar_tmerr(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("X = 5");
    TEST_ASSERT(gwsts_exec_statement("A$ = X") != 0, "A$=X errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR = 13");
    gwdata_clear_error();
    TEST_ASSERT(gwsts_exec_statement("B$ = 1+2") != 0, "B$=1+2 errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR = 13");
    gwdata_clear_error();
    TEST_PASS("assign strict scalar");
}

void test_assign_strict_read_input_raw(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v162: raw-text assignment (INPUT/READ/LINE INPUT path) —
     * assign_value_raw accepts literal text without TMERR */
    extern int assign_value_raw(const char *var_ref, const char *expr);
    TEST_ASSERT_EQ_INT(assign_value_raw("S$", "hello"), 0, "raw assign");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error in raw path");
    struct variable *sv = gwdata_find_variable("S$");
    TEST_ASSERT(sv && sv->value.s.ptr && strcmp(sv->value.s.ptr, "hello") == 0,
                "S$ == hello (raw READ)");
    /* strict mode still rejects expressions */
    TEST_ASSERT(gwsts_exec_statement("T$ = 5") != 0, "strict still on");
    gwdata_clear_error();
    TEST_PASS("assign raw READ");
}

void test_assign_autodim_multidim(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v162 HIGH-2: undeclared 2-D array auto-DIMs 11x11 (no alias) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A(1,1) = 1"), 0, "set 1,1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A(1,2) = 5"), 0, "set 1,2");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B$(1,1) = \"x\""), 0, "str set 1,1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B$(1,2) = \"y\""), 0, "str set 1,2");
    float v1 = biptrg_get_array_element_multi("A", (int[]){1, 1});
    float v2 = biptrg_get_array_element_multi("A", (int[]){1, 2});
    TEST_ASSERT_EQ_INT((int)v1, 1, "A(1,1) == 1 (no alias)");
    TEST_ASSERT_EQ_INT((int)v2, 5, "A(1,2) == 5");
    char *s1 = biptrg_get_array_string_multi("B$", (int[]){1, 1});
    char *s2 = biptrg_get_array_string_multi("B$", (int[]){1, 2});
    TEST_ASSERT(s1 && strcmp(s1, "x") == 0, "B$(1,1) == x");
    TEST_ASSERT(s2 && strcmp(s2, "y") == 0, "B$(1,2) == y");
    TEST_PASS("assign autodim multidim");
}

void test_assign_swap_oob_no_write(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("A$(1) = \"a\"");
    gwsts_exec_statement("DIM B$(2)");
    TEST_ASSERT(gwsts_exec_statement("SWAP A$(1), B$(3)") != 0,
                "SWAP out of range errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 9, "ERRBS = 9");
    gwdata_clear_error();
    /* A$(1) must keep its value (no partial swap) */
    char *s = biptrg_get_array_string("A$", 1);
    TEST_ASSERT(s && strcmp(s, "a") == 0, "A$(1) preserved");
    TEST_PASS("assign swap oob");
}

void test_assign_array_err_no_write(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v162 MEDIUM-3: numeric array eval error → element unchanged */
    gwsts_exec_statement("C(1) = 7");
    TEST_ASSERT(gwsts_exec_statement("C(1) = 1E+39") != 0, "overflow errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 6, "Overflow = 6");
    gwdata_clear_error();
    float v = biptrg_get_array_element("C", 1);
    TEST_ASSERT_EQ_INT((int)v, 7, "C(1) preserved (not 0)");
    /* v162 MEDIUM-4: string array with failing function → old value */
    gwsts_exec_statement("D$(1) = \"keep\"");
    gwsts_exec_statement("ON ERROR GOTO 200");
    gwsts_exec_statement("D$(1) = CHR$(256)");
    char *s2 = biptrg_get_array_string("D$", 1);
    TEST_ASSERT(s2 && strcmp(s2, "keep") == 0, "D$(1) preserved");
    gwdata_clear_error();
    TEST_PASS("assign array err no write");
}
