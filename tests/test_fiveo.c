/* TEST_FIVEO - Unit tests for FIVEO (5.0 Features stub) */
/* Based on FIVEO.ASM - WHILE/WEND, CALL, CHAIN, WRITE */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test fiveo_init doesn't crash */
void test_fiveo_init(void) {
    fiveo_init();
    TEST_PASS("fiveo_init");
}

/* Test fiveo_init called multiple times */
void test_fiveo_init_multiple(void) {
    fiveo_init();
    fiveo_init();
    fiveo_init();
    TEST_PASS("fiveo_init multiple calls");
}
