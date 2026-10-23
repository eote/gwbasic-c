
/* test_gwsts_ifgoto.c — v147: IF..THEN <line> / ELSE <line> numeric-line
 * jump semantics (GW-BASIC: THEN followed by a line number is a GOTO). */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "../src/gwbasic.h"

extern int gwsts_exec_statement(const char *text);
extern void gwdata_clear_error(void);

/* IF with true condition and numeric THEN target → returns line number */
void test_gwsts_ifgoto_then_line_true(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=1 THEN 30");
    TEST_ASSERT_EQ_INT(rc, 30, "THEN 30 → jump target 30");
    TEST_PASS("ifgoto then line true");
}

/* IF with false condition and numeric ELSE target → returns line number */
void test_gwsts_ifgoto_else_line_false(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=2 THEN 30 ELSE 40");
    TEST_ASSERT_EQ_INT(rc, 40, "ELSE 40 → jump target 40");
    TEST_PASS("ifgoto else line false");
}

/* IF false without ELSE → success (fall through) */
void test_gwsts_ifgoto_false_no_else(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=2 THEN 30");
    TEST_ASSERT_EQ_INT(rc, 0, "false without ELSE → fall through");
    TEST_PASS("ifgoto false no else");
}

/* IF true with statement form still executes the statement */
void test_gwsts_ifgoto_then_statement(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=1 THEN PRINT \"OK\"");
    TEST_ASSERT_EQ_INT(rc, 0, "THEN PRINT still executes");
    TEST_PASS("ifgoto then statement");
}

/* IF false with statement ELSE still executes the ELSE statement */
void test_gwsts_ifgoto_else_statement(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=2 THEN 30 ELSE PRINT \"EOK\"");
    TEST_ASSERT_EQ_INT(rc, 0, "ELSE PRINT still executes");
    TEST_PASS("ifgoto else statement");
}

/* Mixed: THEN line number, ELSE line number, false cond → ELSE line */
void test_gwsts_ifgoto_mixed_line_and_statement(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    int rc = gwsts_exec_statement("IF 1=2 THEN PRINT \"NO\" ELSE 50");
    TEST_ASSERT_EQ_INT(rc, 50, "false → ELSE 50 line jump");
    TEST_PASS("ifgoto mixed");
}
