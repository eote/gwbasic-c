#include "test_framework.h"
#include "../src/gwbasic.h"
#include <math.h>

void test_math_debug(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gweval_init();
    
    float result;
    
    result = gweval_eval_numeric("SIN(0)");
    DEBUG_PRINT("SIN(0) = %f", result);
    TEST_ASSERT(fabsf(result - 0.0f) < 0.0001f, "SIN(0) in expression");
    
    result = gweval_eval_numeric("COS(0)");
    DEBUG_PRINT("COS(0) = %f", result);
    TEST_ASSERT(fabsf(result - 1.0f) < 0.0001f, "COS(0) in expression");
    
    result = gweval_eval_numeric("SQR(16)");
    DEBUG_PRINT("SQR(16) = %f", result);
    TEST_ASSERT(fabsf(result - 4.0f) < 0.0001f, "SQR(16) in expression");
    
    TEST_PASS("math_debug");
}