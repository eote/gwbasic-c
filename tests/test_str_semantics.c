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
