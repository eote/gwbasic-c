/* GW-BASIC C Port - GWSTS/GWEVAL Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gweval_literal(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gweval_init();
    
    /* Test numeric literal evaluation */
    float result = gweval_eval_numeric("123");
    TEST_ASSERT_EQ_INT((int)result, 123, "integer literal");
    
    result = gweval_eval_numeric("3.14");
    TEST_ASSERT(result > 3.13 && result < 3.15, "float literal");
    
    TEST_PASS("literal evaluation");
}

void test_gweval_binary_ops(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gweval_init();
    
    float result;
    
    result = gweval_eval_numeric("2+3");
    TEST_ASSERT_EQ_INT((int)result, 5, "addition");
    
    result = gweval_eval_numeric("10-4");
    TEST_ASSERT_EQ_INT((int)result, 6, "subtraction");
    
    result = gweval_eval_numeric("6*7");
    TEST_ASSERT_EQ_INT((int)result, 42, "multiplication");
    
    result = gweval_eval_numeric("20/4");
    TEST_ASSERT_EQ_INT((int)result, 5, "division");
    
    result = gweval_eval_numeric("2^3");
    TEST_ASSERT_EQ_INT((int)result, 8, "power");
    
    TEST_PASS("binary operators");
}

void test_gweval_precedence(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gweval_init();
    
    float result;
    
    result = gweval_eval_numeric("2+3*4");
    TEST_ASSERT_EQ_INT((int)result, 14, "mul before add");
    
    result = gweval_eval_numeric("(2+3)*4");
    TEST_ASSERT_EQ_INT((int)result, 20, "parentheses");
    
    result = gweval_eval_numeric("10-2*3");
    TEST_ASSERT_EQ_INT((int)result, 4, "mul before sub");
    
    TEST_PASS("operator precedence");
}

void test_gweval_variables(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gweval_init();
    
    /* Set variable A = 10 */
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    a->value.f = 10.0f;
    
    float result = gweval_eval_numeric("A*2");
    TEST_ASSERT_EQ_INT((int)result, 20, "variable in expression");
    
    TEST_PASS("variable evaluation");
}

void test_gwsts_let(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init();
    
    /* LET A = 42 */
    int rc = gwsts_exec_statement("LET A = 42");
    TEST_ASSERT_EQ_INT(rc, 0, "LET executes");
    
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT((int)a->value.f, 42, "A has correct value");
    
    TEST_PASS("LET statement");
}

void test_gwsts_print(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init();
    
    /* PRINT 2+2 */
    int rc = gwsts_exec_statement("PRINT 2+2");
    TEST_ASSERT_EQ_INT(rc, 0, "PRINT executes");
    
    TEST_PASS("PRINT statement");
}

void test_gwsts_if(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init();
    
    /* IF 1 THEN A = 5 */
    int rc = gwsts_exec_statement("IF 1 THEN A = 5");
    TEST_ASSERT_EQ_INT(rc, 0, "IF executes");
    
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT((int)a->value.f, 5, "A set by IF");
    
    /* IF 0 THEN A = 10 (should not execute) */
    rc = gwsts_exec_statement("IF 0 THEN A = 10");
    TEST_ASSERT_EQ_INT(rc, 0, "IF false executes");
    
    a = gwdata_find_variable("A");
    TEST_ASSERT_EQ_INT((int)a->value.f, 5, "A unchanged by false IF");
    
    TEST_PASS("IF statement");
}