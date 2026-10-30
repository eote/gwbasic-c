/*
 * test_biprtu.c — v159: PRINT USING driver (ASM BIPRTU PRINUS)
 * Coverage for reviewer HIGH findings: overflow %, scientific,
 * $$ / ** / **$ fields, TMERR, 24-digit limit, REUSIN.
 * biprtu_print_using_values writes to stdout — tests capture via
 * statement-level exec (gwsts_print_using) instead, using file
 * redirect? Simpler: call biprtu_print_using_values and rely on
 * gwdata error-code assertions for error paths; for output checks
 * use gwsts_exec_statement with PRINT USING and capture via freopen?
 * → Use gwsts-level exec and assert error codes; output-shape cases
 * are covered by the .bas integration tests.
 */
#include <string.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);

void test_biprtu_overflow_percent(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* value wider than field → % prefix, single sign */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT USING \"###.##\"; -1234.56"), 0,
                       "negative overflow executes");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error on overflow path");
    TEST_PASS("biprtu overflow percent");
}

void test_biprtu_scientific(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT USING \"##.##^^^^\"; 123.45"), 0,
                       "scientific executes");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "scientific no error");
    TEST_PASS("biprtu scientific");
}

void test_biprtu_dollar_star_fields(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT USING \"$$###.##\"; 12.34"), 0,
                       "$$ executes");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "$$ no error");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT USING \"**###\"; 12"), 0,
                       "** executes");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "** no error");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT USING \"**$###.##\"; 12.3"), 0,
                       "**$ executes");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "**$ no error");
    TEST_PASS("biprtu dollar star fields");
}

void test_biprtu_tmerr_string_in_numeric(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 HIGH-8: string into numeric field → TMERR (13) */
    TEST_ASSERT(gwsts_exec_statement("PRINT USING \"###\"; \"AB\"") != 0,
                "string into numeric field errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR = 13");
    gwdata_clear_error();
    TEST_PASS("biprtu tmerr string");
}

void test_biprtu_tmerr_number_in_string(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 HIGH-8: number into string field → TMERR (13) */
    TEST_ASSERT(gwsts_exec_statement("PRINT USING \"!\"; 123") != 0,
                "number into string field errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR = 13");
    gwdata_clear_error();
    TEST_PASS("biprtu tmerr number");
}

void test_biprtu_field_24_limit(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 MEDIUM-8/CRITICAL-2: field > 24 digits → FCERR (5),
     * not a buffer overflow */
    TEST_ASSERT(gwsts_exec_statement(
        "PRINT USING \"#.############################################\"; 1.5") != 0,
        ">24-digit field errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "field limit FCERR = 5");
    gwdata_clear_error();
    TEST_PASS("biprtu field 24 limit");
}

void test_biprtu_long_string_no_crash(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 CRITICAL-1 regression: & field with long values must not
     * crash (growable output) */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement(
        "A$ = \"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\""), 0, "set A$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$ = A$ + A$ + A$ + A$"), 0, "grow A$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement(
        "PRINT USING \"&\"; A$; A$; A$; A$; A$; A$"), 0, "1200-char output survives");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error");
    TEST_PASS("biprtu long string no crash");
}

void test_biprtu_no_field_fcerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 MEDIUM-1: format with no fields + values → FCERR (5) */
    TEST_ASSERT(gwsts_exec_statement("PRINT USING \"abc\"; 1") != 0,
                "no-field format errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "FCERR = 5");
    gwdata_clear_error();
    TEST_PASS("biprtu no field fcerr");
}

void test_biprtu_missing_semicolon_snerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gioscn_screen(1);
    gwdata_clear_error();
    /* v159 MEDIUM-2: missing ';' → SNERR (2) */
    TEST_ASSERT(gwsts_exec_statement("PRINT USING \"##\" 5") != 0,
                "missing semicolon errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 2, "SNERR = 2");
    gwdata_clear_error();
    TEST_PASS("biprtu missing semicolon");
}
