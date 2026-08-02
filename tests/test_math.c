/* GW-BASIC C Port - Math Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_math1_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("math1_init");
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
    
    /* tan(0) = 0 */
    float result = math1_tan(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "tan(0) = 0");
    
    /* tan(PI/4) = 1 */
    result = math1_tan(0.78539816339f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.001f, "tan(PI/4) = 1");
    
    TEST_PASS("math1_tan");
}

void test_math1_atn(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    /* atn(0) = 0 */
    float result = math1_atn(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "atn(0) = 0");
    
    /* atn(1) = PI/4 */
    result = math1_atn(1.0f);
    TEST_ASSERT(fabsf(result - 0.78539816339f) < 0.001f, "atn(1) = PI/4");
    
    TEST_PASS("math1_atn");
}

void test_math1_exp(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    /* exp(0) = 1 */
    float result = math1_exp(0.0f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "exp(0) = 1");
    
    /* exp(1) = e */
    result = math1_exp(1.0f);
    TEST_ASSERT(fabsf(result - 2.71828182846f) < 0.001f, "exp(1) = e");
    
    TEST_PASS("math1_exp");
}

void test_math1_log(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math1_init();
    
    /* log(1) = 0 */
    float result = math1_log(1.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "log(1) = 0");
    
    /* log(e) = 1 */
    result = math1_log(2.71828182846f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.001f, "log(e) = 1");
    
    /* log(0) should return 0 (error case) */
    result = math1_log(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "log(0) = 0 (error case)");
    
    /* log(-1) should return 0 (error case) */
    result = math1_log(-1.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "log(-1) = 0 (error case)");
    
    TEST_PASS("math1_log");
}

void test_math2_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("math2_init");
}

void test_math2_sqr(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* sqr(0) = 0 */
    float result = math2_sqr(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sqr(0) = 0");
    
    /* sqr(4) = 2 */
    result = math2_sqr(4.0f);
    TEST_ASSERT(fabsf(result - 2.0f) < 0.0001f, "sqr(4) = 2");
    
    /* sqr(9) = 3 */
    result = math2_sqr(9.0f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "sqr(9) = 3");
    
    /* sqr(-1) = 0 (error case) */
    result = math2_sqr(-1.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sqr(-1) = 0 (error case)");
    
    TEST_PASS("math2_sqr");
}

void test_math2_abs(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* abs(5) = 5 */
    float result = math2_abs(5.0f);
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "abs(5) = 5");
    
    /* abs(-5) = 5 */
    result = math2_abs(-5.0f);
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "abs(-5) = 5");
    
    /* abs(0) = 0 */
    result = math2_abs(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "abs(0) = 0");
    
    TEST_PASS("math2_abs");
}

void test_math2_sgn(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* sgn(5) = 1 */
    float result = math2_sgn(5.0f);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "sgn(5) = 1");
    
    /* sgn(-5) = -1 */
    result = math2_sgn(-5.0f);
    TEST_ASSERT(fabsf(result + 1.0f) < 0.0001f, "sgn(-5) = -1");
    
    /* sgn(0) = 0 */
    result = math2_sgn(0.0f);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "sgn(0) = 0");
    
    TEST_PASS("math2_sgn");
}

void test_math2_rnd(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* RND with negative seed should be deterministic */
    float r1 = math2_rnd(-1.0f);
    float r2 = math2_rnd(-1.0f);
    TEST_ASSERT(fabsf(r1 - r2) < 0.0001f, "RND with same negative seed returns same value");
    
    /* RND with positive argument returns random in [0, 1) */
    int passed = 0;
    for (int i = 0; i < 10; i++) {
        float r = math2_rnd(1.0f);
        if (r >= 0.0f && r < 1.0f) passed++;
    }
    TEST_ASSERT_EQ_INT(passed, 10, "All 10 random values in range");
    
    /* RND(0) returns last value */
    float last = math2_rnd(1.0f);
    float r0 = math2_rnd(0.0f);
    TEST_ASSERT(fabsf(r0 - last) < 0.0001f, "RND(0) returns last value");
    
    TEST_PASS("math2_rnd");
}

void test_math2_int(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* int(3.7) = 3 */
    float result = math2_int(3.7f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "int(3.7) = 3");
    
    /* int(-3.7) = -4 (floor) */
    result = math2_int(-3.7f);
    TEST_ASSERT(fabsf(result + 4.0f) < 0.0001f, "int(-3.7) = -4");
    
    /* int(5) = 5 */
    result = math2_int(5.0f);
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "int(5) = 5");
    
    TEST_PASS("math2_int");
}

void test_math2_fix(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    math2_init();
    
    /* fix(3.7) = 3 */
    float result = math2_fix(3.7f);
    TEST_ASSERT(fabsf(result - 3.0f) < 0.0001f, "fix(3.7) = 3");
    
    /* fix(-3.7) = -3 (truncate towards zero) */
    result = math2_fix(-3.7f);
    TEST_ASSERT(fabsf(result + 3.0f) < 0.0001f, "fix(-3.7) = -3");
    
    /* fix(5) = 5 */
    result = math2_fix(5.0f);
    TEST_ASSERT(fabsf(result - 5.0f) < 0.0001f, "fix(5) = 5");
    
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