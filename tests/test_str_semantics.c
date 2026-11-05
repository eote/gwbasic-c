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
    for (int i = 0; i < 8000; i++) {
        TEST_ASSERT_EQ_INT(gwsts_exec_statement("IF C$=\"\" THEN N=N+1"), 0,
                           "IF C$=\"\"");
        gwdata_clear_error();
    }
    gwsts_exec_statement("F2 = FRE(0)");
    struct variable *vf2 = gwdata_find_variable("F2");
    TEST_ASSERT(vf2, "F2 set");
    float f2 = vf2->value.f;
    TEST_ASSERT(f1 - f2 < 16.0f, "heap stable (drop < 16 bytes)");
    TEST_PASS("empty str loop heap stable");
}
