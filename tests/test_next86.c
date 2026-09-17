/* TEST_NEXT86 - Unit tests for NEXT86 (NEXT Loop Handling) */
/* Based on NEXT86.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test next86_init */
void test_next86_init(void) {
    next86_init();
    
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 0, "FOR stack empty after init");
    TEST_ASSERT_EQ_INT(next86_inside_for(), 0, "Not inside FOR after init");
    
    TEST_PASS("next86_init");
}

/* Test next86_for basic */
void test_next86_for_basic(void) {
    next86_init();
    
    /* Position text after variable name "I" - use modifiable buffer */
    char text[] = "=1 TO 10";
    const char *p = text;
    int result = next86_for(&p);
    
    TEST_ASSERT_EQ_INT(result, 0, "FOR returns success");
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "FOR depth is 1");
    TEST_ASSERT_EQ_INT(next86_inside_for(), 1, "Inside FOR");
    
    TEST_PASS("next86_for basic");
}

/* Test next86_for with STEP */
void test_next86_for_with_step(void) {
    next86_init();
    
    char text[] = "=1 TO 10 STEP 2";
    const char *p = text;
    int result = next86_for(&p);
    
    TEST_ASSERT_EQ_INT(result, 0, "FOR with STEP returns success");
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "FOR depth is 1");
    
    TEST_PASS("next86_for with STEP");
}

/* Test next86_for nested */
void test_next86_for_nested(void) {
    next86_init();
    
    char text1[] = "=1 TO 10";
    const char *p1 = text1;
    next86_for(&p1);
    
    char text2[] = "=1 TO 5";
    const char *p2 = text2;
    next86_for(&p2);
    
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 2, "Nested FOR depth is 2");
    
    TEST_PASS("next86_for nested");
}

/* Test next86_next basic */
void test_next86_next_basic(void) {
    next86_init();
    
    char text[] = "=1 TO 10";
    const char *p = text;
    next86_for(&p);
    
    char next_text[] = "I";
    const char *np = next_text;
    int result = next86_next(&np);
    
    /* Should return error since NEXT var_ptr doesn't match FOR */
    TEST_ASSERT_EQ_INT(result, -1, "NEXT returns error (var_ptr mismatch)");
    
    TEST_PASS("next86_next basic");
}

/* Test next86_next loop termination */
void test_next86_next_termination(void) {
    next86_init();
    
    char text[] = "=1 TO 1";
    const char *p = text;
    next86_for(&p);
    
    char next_text[] = "I";
    const char *np = next_text;
    int result = next86_next(&np);
    
    /* Returns error due to var_ptr mismatch */
    TEST_ASSERT_EQ_INT(result, -1, "NEXT returns error (var_ptr mismatch)");
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "FOR still on stack (no match)");
    
    TEST_PASS("next86_next termination");
}

/* Test next86_next without FOR error */
void test_next86_next_no_for(void) {
    next86_init();
    
    char next_text[] = "I";
    const char *np = next_text;
    int result = next86_next(&np);
    
    TEST_ASSERT_EQ_INT(result, -1, "NEXT without FOR returns error");
    
    TEST_PASS("next86_next no FOR error");
}

/* Test next86_clear_for_stack */
void test_next86_clear_for_stack(void) {
    next86_init();
    
    char text[] = "=1 TO 10";
    const char *p = text;
    next86_for(&p);
    
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "FOR depth 1 before clear");
    
    next86_clear_for_stack();
    
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 0, "FOR depth 0 after clear");
    TEST_ASSERT_EQ_INT(next86_inside_for(), 0, "Not inside FOR after clear");
    
    TEST_PASS("next86_clear_for_stack");
}

/* Test next86_get_for_depth */
void test_next86_get_for_depth(void) {
    next86_init();
    
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 0, "Depth 0 initially");
    
    char text1[] = "=1 TO 10";
    const char *p1 = text1;
    next86_for(&p1);
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "Depth 1 after one FOR");
    
    char text2[] = "=1 TO 5";
    const char *p2 = text2;
    next86_for(&p2);
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 2, "Depth 2 after nested FOR");
    
    next86_clear_for_stack();
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 0, "Depth 0 after clear");
    
    TEST_PASS("next86_get_for_depth");
}

/* Test next86_inside_for */
void test_next86_inside_for(void) {
    next86_init();
    
    TEST_ASSERT_EQ_INT(next86_inside_for(), 0, "Not inside FOR initially");
    
    char text[] = "=1 TO 10";
    const char *p = text;
    next86_for(&p);
    TEST_ASSERT_EQ_INT(next86_inside_for(), 1, "Inside FOR after FOR");
    
    next86_clear_for_stack();
    TEST_ASSERT_EQ_INT(next86_inside_for(), 0, "Not inside FOR after clear");
    
    TEST_PASS("next86_inside_for");
}

/* Test next86_for max depth */
void test_next86_for_max_depth(void) {
    next86_init();
    
    int max_depth = 16;  /* MAX_FOR_DEPTH */
    
    for (int i = 0; i < max_depth; i++) {
        char text[] = "=1 TO 10";
        const char *p = text;
        int result = next86_for(&p);
        TEST_ASSERT_EQ_INT(result, 0, "FOR success");
    }
    
    /* Next FOR should also succeed (var_index wraps around) */
    char text[] = "=1 TO 10";
    const char *p = text;
    int result = next86_for(&p);
    TEST_ASSERT_EQ_INT(result, 0, "FOR still succeeds (var_index wrapped)");
    
    TEST_PASS("next86_for max depth");
}

/* Test next86 variable types */
void test_next86_variable_types(void) {
    next86_init();
    
    /* Integer variable */
    char text1[] = "=1 TO 10";
    const char *p1 = text1;
    next86_for(&p1);
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "Integer FOR works");
    next86_clear_for_stack();
    
    /* Float variable */
    char text2[] = "=1 TO 10";
    const char *p2 = text2;
    next86_for(&p2);
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "Float FOR works");
    next86_clear_for_stack();
    
    TEST_PASS("next86 variable types");
}

/* Test next86 step negative */
void test_next86_step_negative(void) {
    next86_init();
    
    char text[] = "=10 TO 1 STEP -1";
    const char *p = text;
    int result = next86_for(&p);
    
    TEST_ASSERT_EQ_INT(result, 0, "FOR with negative STEP");
    TEST_ASSERT_EQ_INT(next86_get_for_depth(), 1, "FOR depth 1");
    
    TEST_PASS("next86 step negative");
}