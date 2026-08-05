/* TEST_GIO86 - Unit tests for GIO86 (8086-specific I/O) */
/* Based on GIO86.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test gio86_init */
void test_gio86_init(void) {
    gio86_init();
    
    TEST_PASS("gio86_init");
}

/* Test gio86_term */
void test_gio86_term(void) {
    gio86_term();
    
    TEST_PASS("gio86_term");
}

/* Test gio86_open returns device unavailable */
void test_gio86_open(void) {
    const char *text = "test";
    int result = gio86_open(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "open returns device unavailable");
    TEST_PASS("gio86_open");
}

/* Test gio86_close returns device unavailable */
void test_gio86_close(void) {
    const char *text = "test";
    int result = gio86_close(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "close returns device unavailable");
    TEST_PASS("gio86_close");
}

/* Test gio86_width returns device unavailable */
void test_gio86_width(void) {
    const char *text = "test";
    int result = gio86_width(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "width returns device unavailable");
    TEST_PASS("gio86_width");
}

/* Test gio86_bsave returns device unavailable */
void test_gio86_bsave(void) {
    const char *text = "test";
    int result = gio86_bsave(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "bsave returns device unavailable");
    TEST_PASS("gio86_bsave");
}

/* Test gio86_bload returns device unavailable */
void test_gio86_bload(void) {
    const char *text = "test";
    int result = gio86_bload(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "bload returns device unavailable");
    TEST_PASS("gio86_bload");
}

/* Test gio86_lprint returns device unavailable */
void test_gio86_lprint(void) {
    const char *text = "test";
    int result = gio86_lprint(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "lprint returns device unavailable");
    TEST_PASS("gio86_lprint");
}

/* Test gio86_print returns device unavailable */
void test_gio86_print(void) {
    const char *text = "test";
    int result = gio86_print(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "print returns device unavailable");
    TEST_PASS("gio86_print");
}

/* Test gio86_bparms returns device unavailable */
void test_gio86_bparms(void) {
    const char *text = "test";
    int result = gio86_bparms(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "bparms returns device unavailable");
    TEST_PASS("gio86_bparms");
}

/* Test gio86_opnfil returns device unavailable */
void test_gio86_opnfil(void) {
    int result = gio86_opnfil();
    
    TEST_ASSERT_EQ_INT(result, -1, "opnfil returns device unavailable");
    TEST_PASS("gio86_opnfil");
}

/* Test gio86_pofnum returns device unavailable */
void test_gio86_pofnum(void) {
    const char *text = "test";
    int result = gio86_pofnum(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "pofnum returns device unavailable");
    TEST_PASS("gio86_pofnum");
}

/* Test gio86_porlen returns device unavailable */
void test_gio86_porlen(void) {
    const char *text = "test";
    int result = gio86_porlen(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "porlen returns device unavailable");
    TEST_PASS("gio86_porlen");
}

/* Test gio86_pnrlen returns device unavailable */
void test_gio86_pnrlen(void) {
    const char *text = "test";
    int result = gio86_pnrlen(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "pnrlen returns device unavailable");
    TEST_PASS("gio86_pnrlen");
}

/* Test gio86_getwdt returns device unavailable */
void test_gio86_getwdt(void) {
    const char *text = "test";
    int result = gio86_getwdt(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "getwdt returns device unavailable");
    TEST_PASS("gio86_getwdt");
}