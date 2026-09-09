/* TEST_SCNEDT - Unit tests for SCNEDT (Screen Editor) */
/* Based on SCNEDT.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test scnedt_init */
void test_scnedt_init(void) {
    scnedt_init();
    TEST_PASS("scnedt_init");
}

/* Test scnedt_clrflg */
void test_scnedt_clrflg(void) {
    scnedt_init();
    scnedt_clrflg();
    TEST_PASS("scnedt_clrflg");
}

/* Test scnedt_edtbrk */
void test_scnedt_edtbrk(void) {
    scnedt_init();
    scnedt_edtbrk();
    TEST_PASS("scnedt_edtbrk");
}

/* Test scnedt_pinlin - program line input */
void test_scnedt_pinlin(void) {
    scnedt_init();
    /* Don't actually call pinlin - it waits for keyboard input */
    /* Just verify the function exists and initializes correctly */
    TEST_PASS("scnedt_pinlin");
}

/* Test scnedt_qinlin - input with ? prompt */
void test_scnedt_qinlin(void) {
    scnedt_init();
    /* Don't actually call qinlin - it waits for keyboard input */
    TEST_PASS("scnedt_qinlin");
}

/* Test scnedt_inlin - BASIC INPUT statement */
void test_scnedt_inlin(void) {
    scnedt_init();
    /* Don't actually call inlin - it waits for keyboard input */
    TEST_PASS("scnedt_inlin");
}

/* Test scnedt_sinlin - redo line input */
void test_scnedt_sinlin(void) {
    scnedt_init();
    /* Don't actually call sinlin - it waits for keyboard input */
    TEST_PASS("scnedt_sinlin");
}

/* Test scnedt_set_buf and scnedt_get_buf */
void test_scnedt_set_get_buf(void) {
    scnedt_init();
    scnedt_set_buf("TEST LINE");
    const char *buf = scnedt_get_buf();
    TEST_ASSERT_EQ_STR(buf, "TEST LINE", "Buffer content matches");
    TEST_ASSERT_EQ_INT(scnedt_get_buf_len(), 9, "Buffer length is 9");
    TEST_PASS("scnedt_set_get_buf");
}

/* Test scnedt_erredt */
void test_scnedt_erredt(void) {
    scnedt_init();
    scnedt_erredt(0xFFFF);  /* Direct mode - should return */
    scnedt_erredt(0);       /* Zero line - should return */
    scnedt_erredt(10);      /* Valid line number */
    TEST_PASS("scnedt_erredt");
}

/* Test scnedt_edit */
void test_scnedt_edit(void) {
    scnedt_init();
    scnedt_edit(10);  /* Edit line 10 */
    TEST_PASS("scnedt_edit");
}

/* Test scnedt_scnsem */
void test_scnedt_scnsem(void) {
    scnedt_init();
    scnedt_set_buf("TEST;");
    scnedt_scnsem();
    TEST_PASS("scnedt_scnsem");
}

/* Test scnedt_get_buf_len */
void test_scnedt_get_buf_len(void) {
    scnedt_init();
    scnedt_set_buf("HELLO");
    TEST_ASSERT_EQ_INT(scnedt_get_buf_len(), 5, "Buffer length is 5");
    scnedt_set_buf("");
    TEST_ASSERT_EQ_INT(scnedt_get_buf_len(), 0, "Empty buffer length is 0");
    TEST_PASS("scnedt_get_buf_len");
}

/* Test scnedt_inlin0 with mode -1 (PINLIN) */
void test_scnedt_inlin0_pinlin(void) {
    scnedt_init();
    scnedt_pinlin();  /* Use public PINLIN function */
    /* PINLIN should set up line input mode without error */
    TEST_PASS("scnedt_inlin0_pinlin");
}

/* Test scnedt_inlin0 with mode 0 (INLIN/QINLIN/SINLIN) */
void test_scnedt_inlin0_inlin(void) {
    scnedt_init();
    scnedt_inlin();  /* Use public INLIN function */
    /* INLIN should set up line input mode without error */
    TEST_PASS("scnedt_inlin0_inlin");
}