/* GW-BASIC C Port - Math Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_math1_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("math1_init");
}

void test_math2_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("math2_init");
}

void test_math1_sin(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    /* sin(0) = 0 */
    float result = math1_sin(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sin(0) = 0");
    
    /* sin(PI/2) = 1 */
    result = math1_sin(1.57079632679f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "sin(PI/2) = 1");
    
    /* sin(PI) = 0 */
    result = math1_sin(3.14159265359f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.001f, "sin(PI) = 0");
    
    TEST_PASS("math1_sin");
}

void test_math1_cos(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    /* cos(0) = 1 */
    float result = math1_cos(0.0f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "cos(0) = 1");
    
    /* cos(PI/2) = 0 */
    result = math1_cos(1.57079632679f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "cos(PI/2) = 0");
    
    /* cos(PI) = -1 */
    result = math1_cos(3.14159265359f);
    TEST_ASSERT(fabsf(result + 1.0f) < 0.001f, "cos(PI) = -1");
    
    TEST_PASS("math1_cos");
}

void test_math1_tan(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    float result = math1_tan(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "tan(0) = 0");
    
    TEST_PASS("math1_tan");
}

void test_math1_atn(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    float result = math1_atn(1.0f);
    TEST_ASSERT(fabsf(result - 0.78539816339f) < 0.0001f, "atn(1) = PI/4");
    
    TEST_PASS("math1_atn");
}

void test_math1_exp(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    float result = math1_exp(0.0f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "exp(0) = 1");
    
    result = math1_exp(1.0f);
    TEST_ASSERT(fabsf(result - 2.718281828f) < 0.001f, "exp(1) = e");
    
    TEST_PASS("math1_exp");
}

void test_math1_log(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    float result = math1_log(1.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "log(1) = 0");
    
    result = math1_log(2.718281828f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.001f, "log(e) = 1");
    
    TEST_PASS("math1_log");
}

void test_math2_sqr(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    float result = math2_sqr(16.0f);
    TEST_ASSERT(fabsf(result - 4.0f) < 0.0001f, "sqr(16) = 4");
    
    result = math2_sqr(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sqr(0) = 0");
    
    TEST_PASS("math2_sqr");
}

void test_math2_abs(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    float result = math2_abs(-5.0f);
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "abs(-5) = 5");
    
    result = math2_abs(3.0f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "abs(3) = 3");
    
    TEST_PASS("math2_abs");
}

void test_math2_sgn(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    float result = math2_sgn(-10.0f);
    TEST_ASSERT(fabsf(result + 1.0f) < 0.0001f, "sgn(-10) = -1");
    
    result = math2_sgn(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sgn(0) = 0");
    
    result = math2_sgn(5.0f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "sgn(5) = 1");
    
    TEST_PASS("math2_sgn");
}

void test_math2_rnd(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* RND returns 0..1 range, just test it executes without crash */
    float result = math2_rnd(0.0f);
    TEST_ASSERT(result >= 0.0f && result <= 1.0f, "rnd in range");
    
    TEST_PASS("math2_rnd");
}

void test_math2_int(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    float result = math2_int(3.7f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "int(3.7) = 3");
    
    result = math2_int(-3.7f);
    TEST_ASSERT(fabsf(result + 4.0f) < 0.0001f, "int(-3.7) = -4 (GW-BASIC INT floors)");
    
    TEST_PASS("math2_int");
}

void test_math2_fix(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    float result = math2_fix(3.7f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "fix(3.7) = 3");
    
    result = math2_fix(-3.7f);
    TEST_ASSERT(fabsf(result + 3.0f) < 0.0001f, "fix(-3.7) = -3 (truncates toward 0)");
    
    TEST_PASS("math2_fix");
}

void test_math_expression_eval(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init(); math2_init();
    gweval_init();
    
    /* Test function evaluation in expressions */
    float result = gweval_eval_numeric("SIN(0)");
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "SIN(0) in expression");
    
    result = gweval_eval_numeric("COS(0)");
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "COS(0) in expression");
    
    result = gweval_eval_numeric("SQR(16)");
    TEST_ASSERT(fabsf(result - 4.0f) < 0.0001f, "SQR(16) in expression");
    
    result = gweval_eval_numeric("ABS(-5)");
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "ABS(-5) in expression");
    
    result = gweval_eval_numeric("SGN(-10)");
    TEST_ASSERT(fabsf(result + 1.0f) < 0.0001f, "SGN(-10) in expression");
    
    result = gweval_eval_numeric("INT(3.7)");
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "INT(3.7) in expression");
    
    result = gweval_eval_numeric("FIX(-3.7)");
    TEST_ASSERT(fabsf(result + 3.0f) < 0.0001f, "FIX(-3.7) in expression");
    
    result = gweval_eval_numeric("SIN(0) + COS(0)");
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "SIN(0) + COS(0) = 1");
    
    result = gweval_eval_numeric("SQR(9) * 2");
    TEST_ASSERT(fabsf(result - 6.0f) < 0.0001f, "SQR(9) * 2 = 6");
    
    TEST_PASS("math_expression_eval");
}

/* Test math functions via gweval (debug test) */
void test_math_debug(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init(); math2_init();
    gweval_init();
    
    float result;
    
    result = gweval_eval_numeric("SIN(0)");
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "SIN(0) in expression");
    
    result = gweval_eval_numeric("COS(0)");
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "COS(0) in expression");
    
    result = gweval_eval_numeric("SQR(16)");
    TEST_ASSERT(fabsf(result - 4.0f) < 0.0001f, "SQR(16) in expression");
    
    TEST_PASS("math_debug");
}