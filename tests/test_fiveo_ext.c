/* TEST_FIVEO - Unit tests for FIVEO (5.0 Features) */
/* Based on FIVEO.ASM - WHILE/WEND, WRITE, CHAIN */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* ========== Init Tests ========== */

void test_fiveo_init(void) {
    fiveo_init();
    TEST_PASS("fiveo_init");
}

void test_fiveo_init_multiple(void) {
    fiveo_init();
    fiveo_init();
    fiveo_init();
    TEST_PASS("fiveo_init multiple calls");
}

void test_fiveo_init_clears_while_stack(void) {
    /* Push something, then re-init should clear it */
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "While stack cleared on init");
    TEST_PASS("fiveo_init clears while stack");
}

/* ========== WHILE/WEND Stack Tests ========== */

void test_fiveo_while_push(void) {
    fiveo_init();
    int rc = fiveo_push_while(100, 200, 10);
    TEST_ASSERT_EQ_INT(rc, 0, "Push while entry succeeds");
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Stack depth 1");
    TEST_PASS("fiveo_while_push");
}

void test_fiveo_while_push_multiple(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_push_while(500, 600, 30);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 3, "Stack depth 3");
    TEST_PASS("fiveo_while_push multiple");
}

void test_fiveo_while_pop(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    struct while_entry entry;
    int rc = fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(rc, 0, "Pop succeeds");
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Stack empty after pop");
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 100, "while_text_ptr preserved");
    TEST_ASSERT_EQ_INT(entry.wend_text_ptr, 200, "wend_text_ptr preserved");
    TEST_ASSERT_EQ_INT(entry.while_line, 10, "while_line preserved");
    TEST_PASS("fiveo_while_pop");
}

void test_fiveo_while_pop_empty(void) {
    fiveo_init();
    struct while_entry entry;
    int rc = fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(rc, -1, "Pop empty stack returns -1");
    TEST_PASS("fiveo_while_pop empty");
}

void test_fiveo_while_pop_order(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_push_while(500, 600, 30);
    
    struct while_entry entry;
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 500, "LIFO: last pushed popped first");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 300, "LIFO: second pushed popped second");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 100, "LIFO: first pushed popped last");
    
    TEST_PASS("fiveo_while_pop LIFO order");
}

void test_fiveo_while_depth(void) {
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Initial depth 0");
    
    fiveo_push_while(100, 200, 10);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Depth 1");
    
    fiveo_push_while(300, 400, 20);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 2, "Depth 2");
    
    struct while_entry entry;
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Depth 1 after pop");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Depth 0 after second pop");
    
    TEST_PASS("fiveo_while_depth tracking");
}

void test_fiveo_while_overflow(void) {
    fiveo_init();
    int rc = 0;
    /* MAX_WHILE_DEPTH is 16 (matching MAX_FOR_DEPTH) */
    for (int i = 0; i < 16; i++) {
        rc = fiveo_push_while(100 + i, 200 + i, 10 + i);
        if (rc != 0) break;
    }
    TEST_ASSERT_EQ_INT(rc, 0, "16 levels OK");
    
    rc = fiveo_push_while(999, 999, 99);
    TEST_ASSERT_EQ_INT(rc, -1, "17th level overflows");
    
    TEST_PASS("fiveo_while overflow protection");
}

void test_fiveo_while_clear(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_clear_while_stack();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Stack cleared");
    TEST_PASS("fiveo_while_clear");
}

void test_fiveo_find_matching_while(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    
    /* Find the entry with wend_text_ptr = 400 */
    int idx = fiveo_find_while(400);
    TEST_ASSERT_EQ_INT(idx, 1, "Found matching WHILE at index 1");
    
    TEST_PASS("fiveo_find_matching_while");
}

void test_fiveo_find_while_not_found(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    
    int idx = fiveo_find_while(999);
    TEST_ASSERT_EQ_INT(idx, -1, "Not found returns -1");
    
    TEST_PASS("fiveo_find_while not found");
}

void test_fiveo_find_while_empty_stack(void) {
    fiveo_init();
    int idx = fiveo_find_while(100);
    TEST_ASSERT_EQ_INT(idx, -1, "Empty stack returns -1");
    TEST_PASS("fiveo_find_while empty stack");
}

/* ========== WRITE Statement Tests ========== */

void test_fiveo_write_init(void) {
    fiveo_init();
    /* WRITE module doesn't need separate init, tested via fiveo_init */
    TEST_PASS("fiveo_write init safe");
}

void test_fiveo_write_clears_state(void) {
    fiveo_init();
    fiveo_write_reset();
    TEST_PASS("fiveo_write_reset safe");
}
