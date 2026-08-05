/* TEST_KANJ86 - Unit tests for KANJ86 (Kanji String Functions) */
/* Based on KANJ86.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test kanj86_init */
void test_kanj86_init(void) {
    kanj86_init();
    
    TEST_PASS("kanj86_init");
}

/* Test KTN function returns syntax error */
void test_kanj86_ktnfn(void) {
    const char *text = "test";
    int result = kanj86_ktnfn(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "KTNFN returns syntax error");
    TEST_PASS("kanj86_ktnfn");
}

/* Test JIS function returns syntax error */
void test_kanj86_jisfn(void) {
    const char *text = "test";
    int result = kanj86_jisfn(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "JISFN returns syntax error");
    TEST_PASS("kanj86_jisfn");
}

/* Test KLEN function returns syntax error */
void test_kanj86_klenfn(void) {
    const char *text = "test";
    int result = kanj86_klenfn(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "KLENFN returns syntax error");
    TEST_PASS("kanj86_klenfn");
}

/* Test KPOS function returns syntax error */
void test_kanj86_kposfn(void) {
    const char *text = "test";
    int result = kanj86_kposfn(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "KPOSFN returns syntax error");
    TEST_PASS("kanj86_kposfn");
}

/* Test kanj86_term */
void test_kanj86_term(void) {
    kanj86_term();
    
    TEST_PASS("kanj86_term");
}