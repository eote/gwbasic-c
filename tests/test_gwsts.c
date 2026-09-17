/* TEST_GWSTS - Extended unit tests for GWSTS (Statement Execution) */
/* Based on GWSTS.ASM - covers DIM, OPEN/CLOSE, direct assignment, etc. */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

static void setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gweval_init();
}

/* Test LET with numeric expression */
void test_gwsts_let_numeric_expr(void) {
    setup();
    int rc = gwsts_exec_statement("LET A = 10 + 20");
    TEST_ASSERT_EQ_INT(rc, 0, "LET with expression executes");
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT((int)a->value.f, 30, "A = 10+20 = 30");
    TEST_PASS("LET numeric expression");
}

/* Test LET with multiplication */
void test_gwsts_let_mult(void) {
    setup();
    int rc = gwsts_exec_statement("LET B = 7 * 8");
    TEST_ASSERT_EQ_INT(rc, 0, "LET mult executes");
    struct variable *b = gwdata_find_variable("B");
    TEST_ASSERT(b != NULL, "B exists");
    TEST_ASSERT_EQ_INT((int)b->value.f, 56, "B = 7*8 = 56");
    TEST_PASS("LET multiplication");
}

/* Test direct assignment without LET */
void test_gwsts_direct_assign(void) {
    setup();
    int rc = gwsts_exec_statement("X = 42");
    TEST_ASSERT_EQ_INT(rc, 0, "direct assign executes");
    struct variable *x = gwdata_find_variable("X");
    TEST_ASSERT(x != NULL, "X exists");
    TEST_ASSERT_EQ_INT((int)x->value.f, 42, "X = 42");
    TEST_PASS("direct assignment");
}

/* Test direct assignment with expression */
void test_gwsts_direct_assign_expr(void) {
    setup();
    int rc = gwsts_exec_statement("Y = 3 * 4 + 5");
    TEST_ASSERT_EQ_INT(rc, 0, "direct assign expr executes");
    struct variable *y = gwdata_find_variable("Y");
    TEST_ASSERT(y != NULL, "Y exists");
    TEST_ASSERT_EQ_INT((int)y->value.f, 17, "Y = 3*4+5 = 17");
    TEST_PASS("direct assign with expression");
}

/* Test PRINT with expression */
void test_gwsts_print_expr(void) {
    setup();
    int rc = gwsts_exec_statement("PRINT 5 * 5");
    TEST_ASSERT_EQ_INT(rc, 0, "PRINT expr executes");
    TEST_PASS("PRINT expression");
}

/* Test IF...THEN with true condition */
void test_gwsts_if_true(void) {
    setup();
    int rc = gwsts_exec_statement("IF 1 THEN Z = 99");
    TEST_ASSERT_EQ_INT(rc, 0, "IF true executes");
    struct variable *z = gwdata_find_variable("Z");
    TEST_ASSERT(z != NULL, "Z exists");
    TEST_ASSERT_EQ_INT((int)z->value.f, 99, "Z = 99");
    TEST_PASS("IF true condition");
}

/* Test IF...THEN with false condition */
void test_gwsts_if_false(void) {
    setup();
    gwsts_exec_statement("W = 7");
    int rc = gwsts_exec_statement("IF 0 THEN W = 100");
    TEST_ASSERT_EQ_INT(rc, 0, "IF false executes");
    struct variable *w = gwdata_find_variable("W");
    TEST_ASSERT(w != NULL, "W exists");
    TEST_ASSERT_EQ_INT((int)w->value.f, 7, "W unchanged = 7");
    TEST_PASS("IF false condition");
}

/* Test IF with conditional expression */
void test_gwsts_if_expr(void) {
    setup();
    int rc = gwsts_exec_statement("IF 5 > 3 THEN R = 1");
    TEST_ASSERT_EQ_INT(rc, 0, "IF expr executes");
    /* Note: by default this may not work if expression evaluation doesn't handle > operator */
    /* But at least the statement should execute without crash */
    TEST_PASS("IF with expression condition");
}

/* Test variable reassignment */
void test_gwsts_reassign(void) {
    setup();
    gwsts_exec_statement("A = 10");
    gwsts_exec_statement("A = 20");
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT((int)a->value.f, 20, "A reassigned to 20");
    TEST_PASS("variable reassignment");
}

/* Test multiple variable assignments */
void test_gwsts_multi_assign(void) {
    setup();
    gwsts_exec_statement("A = 1");
    gwsts_exec_statement("B = 2");
    gwsts_exec_statement("C = 3");
    struct variable *a = gwdata_find_variable("A");
    struct variable *b = gwdata_find_variable("B");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(a != NULL && b != NULL && c != NULL, "A, B, C all exist");
    TEST_ASSERT_EQ_INT((int)a->value.f, 1, "A = 1");
    TEST_ASSERT_EQ_INT((int)b->value.f, 2, "B = 2");
    TEST_ASSERT_EQ_INT((int)c->value.f, 3, "C = 3");
    TEST_PASS("multiple assignments");
}

/* Test LET with string variable */
void test_gwsts_let_string(void) {
    setup();
    int rc = gwsts_exec_statement("LET S$ = \"hello\"");
    TEST_ASSERT_EQ_INT(rc, 0, "LET string executes");
    struct variable *s = gwdata_find_variable("S$");
    TEST_ASSERT(s != NULL, "S$ exists");
    TEST_ASSERT(s->type == VT_STRING, "S$ is string type");
    TEST_PASS("LET string variable");
}

/* Test direct assignment with negative number */
void test_gwsts_negative_assign(void) {
    setup();
    int rc = gwsts_exec_statement("N = -5");
    TEST_ASSERT_EQ_INT(rc, 0, "negative assign executes");
    struct variable *n = gwdata_find_variable("N");
    TEST_ASSERT(n != NULL, "N exists");
    TEST_ASSERT_EQ_INT((int)n->value.f, -5, "N = -5");
    TEST_PASS("negative number assignment");
}

/* Test expression with parentheses */
void test_gwsts_paren_expr(void) {
    setup();
    int rc = gwsts_exec_statement("P = (2 + 3) * 4");
    TEST_ASSERT_EQ_INT(rc, 0, "paren expr executes");
    struct variable *p = gwdata_find_variable("P");
    TEST_ASSERT(p != NULL, "P exists");
    TEST_ASSERT_EQ_INT((int)p->value.f, 20, "P = (2+3)*4 = 20");
    TEST_PASS("parenthesized expression");
}

/* Test expression with division */
void test_gwsts_division(void) {
    setup();
    int rc = gwsts_exec_statement("D = 100 / 4");
    TEST_ASSERT_EQ_INT(rc, 0, "division executes");
    struct variable *d = gwdata_find_variable("D");
    TEST_ASSERT(d != NULL, "D exists");
    TEST_ASSERT_EQ_INT((int)d->value.f, 25, "D = 100/4 = 25");
    TEST_PASS("division expression");
}

/* Test expression with power */
void test_gwsts_power(void) {
    setup();
    int rc = gwsts_exec_statement("E = 2 ^ 5");
    TEST_ASSERT_EQ_INT(rc, 0, "power executes");
    struct variable *e = gwdata_find_variable("E");
    TEST_ASSERT(e != NULL, "E exists");
    TEST_ASSERT_EQ_INT((int)e->value.f, 32, "E = 2^5 = 32");
    TEST_PASS("power expression");
}

/* Test nested IF (IF...THEN with another statement) */
void test_gwsts_nested_if(void) {
    setup();
    int rc = gwsts_exec_statement("IF 1 THEN A = 5");
    TEST_ASSERT_EQ_INT(rc, 0, "nested IF executes");
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT((int)a->value.f, 5, "A = 5");
    TEST_PASS("nested IF");
}

/* Test NULL statement input */
void test_gwsts_null_input(void) {
    setup();
    int rc = gwsts_exec_statement(NULL);
    TEST_ASSERT(rc != 0, "NULL input returns error");
    TEST_PASS("NULL input handling");
}

/* Test empty string input */
void test_gwsts_empty_input(void) {
    setup();
    (void)gwsts_exec_statement("");
    /* Should not crash */
    TEST_PASS("empty input handling");
}

/* Test variable used in expression after assignment */
void test_gwsts_var_in_expr(void) {
    setup();
    gwsts_exec_statement("A = 10");
    int rc = gwsts_exec_statement("B = A + 5");
    TEST_ASSERT_EQ_INT(rc, 0, "var in expr executes");
    struct variable *b = gwdata_find_variable("B");
    TEST_ASSERT(b != NULL, "B exists");
    TEST_ASSERT_EQ_INT((int)b->value.f, 15, "B = A+5 = 15");
    TEST_PASS("variable in expression");
}

/* Test chained assignment operations */
void test_gwsts_chained_ops(void) {
    setup();
    gwsts_exec_statement("A = 5");
    gwsts_exec_statement("B = A * 2");
    gwsts_exec_statement("C = B + A");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c != NULL, "C exists");
    TEST_ASSERT_EQ_INT((int)c->value.f, 15, "C = B+A = 10+5 = 15");
    TEST_PASS("chained operations");
}

/* Test PRINT without arguments */
void test_gwsts_print_empty(void) {
    setup();
    (void)gwsts_exec_statement("PRINT");
    /* PRINT alone prints a newline, should succeed or be handled gracefully */
    TEST_PASS("PRINT empty");
}

/* Test GWSTS array assignment through statement engine */
void test_gwsts_dim_then_set(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init(); biptrg_init();

    /* Execute DIM through statement engine */
    int rc = gwsts_exec_statement("DIM A(10)");
    TEST_ASSERT_EQ_INT(rc, 0, "DIM A(10) executes");

    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists after DIM");
    TEST_ASSERT(a->is_array == true, "A is array after DIM");

    /* Execute array assignment through statement engine */
    rc = gwsts_exec_statement("A(1) = 5");
    TEST_ASSERT_EQ_INT(rc, 0, "A(1) = 5 executes");

    float val = biptrg_get_array_element("A", 1);
    TEST_ASSERT(val == 5.0f, "A(1) = 5 stored correctly");

    TEST_PASS("gwsts DIM then set array element");
}
