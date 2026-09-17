/* TEST_CAL86 - Unit tests for CALL/USR functionality */
/* Based on CALL86.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test CALL statement basic parsing */
void test_call86_call_basic(void) {
    char text[] = "CALL MYROUTINE";
    const char *p = text;
    
    /* Test that CALL statement can be parsed without crash */
    call86_call(&p);
    
    TEST_PASS("call86_call basic parsing");
}

/* Test CALL statement with parameters */
void test_call86_call_with_params(void) {
    char text[] = "CALL MYROUTINE(A, B, C)";
    const char *p = text;
    
    call86_call(&p);
    
    TEST_PASS("call86_call with parameters");
}

/* Test CALLS statement (segment:offset format) */
void test_call86_calls_basic(void) {
    char text[] = "CALLS MYROUTINE";
    const char *p = text;
    
    call86_calls(&p);
    
    TEST_PASS("call86_calls basic parsing");
}

/* Test CALLS with parameters */
void test_call86_calls_with_params(void) {
    char text[] = "CALLS MYROUTINE(A, B)";
    const char *p = text;
    
    call86_calls(&p);
    
    TEST_PASS("call86_calls with parameters");
}

/* Test CALL with no parameters */
void test_call86_call_no_params(void) {
    char text[] = "CALL ROUTINE";
    const char *p = text;
    
    call86_call(&p);
    
    TEST_PASS("call86_call no parameters");
}

/* Test USR function interface */
void test_call86_usr_basic(void) {
    float result = call86_usr(0, 5.0f);
    
    /* USR currently returns 0.0f as placeholder */
    TEST_ASSERT_EQ_FLOAT(result, 0.0f, "USR0 returns 0.0f placeholder");
    TEST_PASS("call86_usr basic call");
}

/* Test USR with all valid indices */
void test_call86_usr_all_indices(void) {
    for (int i = 0; i <= 9; i++) {
        float result = call86_usr(i, 1.0f);
        TEST_ASSERT_EQ_FLOAT(result, 0.0f, "USR returns 0.0f");
    }
    TEST_PASS("call86_usr all valid indices");
}

/* Test USR with invalid indices */
void test_call86_usr_invalid(void) {
    float result1 = call86_usr(-1, 1.0f);
    float result2 = call86_usr(10, 1.0f);
    
    TEST_ASSERT_EQ_FLOAT(result1, 0.0f, "USR -1 returns 0.0f");
    TEST_ASSERT_EQ_FLOAT(result2, 0.0f, "USR 10 returns 0.0f");
    TEST_PASS("call86_usr invalid indices");
}

/* Test DEF USR basic */
void test_call86_defusr_basic(void) {
    call86_defusr(0, 0x1234);
    call86_defusr(9, 0xABCD);
    
    TEST_PASS("call86_defusr basic");
}

/* Test DEF USR invalid indices */
void test_call86_defusr_invalid(void) {
    call86_defusr(-1, 0x1234);
    call86_defusr(10, 0x1234);
    
    TEST_PASS("call86_defusr invalid indices");
}

/* Test CALL with variable reference */
void test_call86_call_variable(void) {
    char text[] = "CALL ROUTINE(X, Y%)";
    const char *p = text;
    
    call86_call(&p);
    
    TEST_PASS("call86_call with variable references");
}

/* Test CALL with array element parameters */
void test_call86_call_array_params(void) {
    char text[] = "CALL ROUTINE(A(5), B%(3))";
    const char *p = text;
    
    call86_call(&p);
    
    TEST_PASS("call86_call with array element parameters");
}

/* Test CALL with empty parens */
void test_call86_call_empty_parens(void) {
    char text[] = "CALL ROUTINE()";
    const char *p = text;
    
    call86_call(&p);
    
    TEST_PASS("call86_call empty parentheses");
}

/* Test initialization */
void test_call86_init(void) {
    call86_init();
    
    TEST_PASS("call86_init");
}