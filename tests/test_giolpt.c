/* TEST_GIOLPT - Unit tests for GIOLPT (Line Printer) */
/* Based on GIOLPT.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test giolpt_init */
void test_giolpt_init(void) {
    giolpt_init();
    TEST_PASS("giolpt_init");
}

/* Test giolpt_open - LPT1 */
void test_giolpt_open_lpt1(void) {
    giolpt_init();
    giolpt_open(0, 2);  /* LPT1, output mode */
    TEST_PASS("giolpt_open_lpt1");
}

/* Test giolpt_open - LPT2 */
void test_giolpt_open_lpt2(void) {
    giolpt_init();
    giolpt_open(1, 2);  /* LPT2, output mode */
    TEST_PASS("giolpt_open_lpt2");
}

/* Test giolpt_open - LPT3 */
void test_giolpt_open_lpt3(void) {
    giolpt_init();
    giolpt_open(2, 2);  /* LPT3, output mode */
    TEST_PASS("giolpt_open_lpt3");
}

/* Test giolpt_open - invalid device */
void test_giolpt_open_invalid(void) {
    giolpt_init();
    giolpt_open(-1, 2);  /* Invalid */
    giolpt_open(3, 2);   /* Invalid */
    TEST_PASS("giolpt_open_invalid");
}

/* Test giolpt_close */
void test_giolpt_close(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_close(0);
    TEST_PASS("giolpt_close");
}

/* Test giolpt_swd - set width */
void test_giolpt_swd(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_swd(0, 132);  /* Set width to 132 */
    TEST_ASSERT_EQ_INT(giolpt_gwd(0), 132, "Width set to 132");
    giolpt_close(0);
    TEST_PASS("giolpt_swd");
}

/* Test giolpt_swd - invalid width */
void test_giolpt_swd_invalid(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_swd(0, 0);   /* Invalid - too small */
    giolpt_swd(0, 256); /* Invalid - too large */
    TEST_PASS("giolpt_swd_invalid");
}

/* Test giolpt_gwd - get width */
void test_giolpt_gwd(void) {
    giolpt_init();
    giolpt_open(0, 2);
    int width = giolpt_gwd(0);
    TEST_ASSERT_EQ_INT(width, 80, "Default width is 80");
    giolpt_close(0);
    TEST_PASS("giolpt_gwd");
}

/* Test giolpt_sot - output characters */
void test_giolpt_sot(void) {
    giolpt_init();
    giolpt_open(0, 2);
    /* Don't actually write to printer - just verify function doesn't crash */
    giolpt_sot(0, 'H');
    giolpt_sot(0, 'i');
    giolpt_sot(0, '!');
    giolpt_close(0);
    TEST_PASS("giolpt_sot");
}

/* Test giolpt_sot - tab handling */
void test_giolpt_sot_tab(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, '\t');  /* Tab */
    giolpt_close(0);
    TEST_PASS("giolpt_sot_tab");
}

/* Test giolpt_sot - CR handling */
void test_giolpt_sot_cr(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, '\r');  /* CR */
    giolpt_close(0);
    TEST_PASS("giolpt_sot_cr");
}

/* Test giolpt_sot - LF handling */
void test_giolpt_sot_lf(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, '\n');  /* LF */
    giolpt_close(0);
    TEST_PASS("giolpt_sot_lf");
}

/* Test giolpt_sot - CR followed by LF (should eat LF) */
void test_giolpt_sot_crlf(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, '\r');  /* CR */
    giolpt_sot(0, '\n');  /* LF - should be eaten */
    giolpt_close(0);
    TEST_PASS("giolpt_sot_crlf");
}

/* Test giolpt_gps - get position */
void test_giolpt_gps(void) {
    giolpt_init();
    giolpt_open(0, 2);
    int pos = giolpt_gps(0);
    TEST_ASSERT_EQ_INT(pos, 0, "Initial position is 0");
    giolpt_close(0);
    TEST_PASS("giolpt_gps");
}

/* Test giolpt_gwd - get width */
void test_giolpt_gwd2(void) {
    giolpt_init();
    giolpt_open(0, 2);
    int width = giolpt_gwd(0);
    TEST_ASSERT_EQ_INT(width, 80, "Default width is 80");
    giolpt_close(0);
    TEST_PASS("giolpt_gwd2");
}

/* Test giolpt_lpos */
void test_giolpt_lpos(void) {
    giolpt_init();
    giolpt_open(0, 2);
    int lpos = giolpt_lpos(0);
    TEST_ASSERT_EQ_INT(lpos, 1, "LPOS returns 1 (1-based)");
    giolpt_close(0);
    TEST_PASS("giolpt_lpos");
}

/* Test giolpt_swd then gwd */
void test_giolpt_swd_gwd(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_swd(0, 40);
    TEST_ASSERT_EQ_INT(giolpt_gwd(0), 40, "Width set to 40");
    giolpt_swd(0, 132);
    TEST_ASSERT_EQ_INT(giolpt_gwd(0), 132, "Width set to 132");
    giolpt_close(0);
    TEST_PASS("giolpt_swd_gwd");
}

/* Test giolpt_sot - multiple characters */
void test_giolpt_sot_multiple(void) {
    giolpt_init();
    giolpt_open(0, 2);
    const char *text = "Hello, World!\r\n";
    for (int i = 0; text[i]; i++) {
        giolpt_sot(0, text[i]);
    }
    giolpt_close(0);
    TEST_PASS("giolpt_sot_multiple");
}

/* Test giolpt_sot - line overflow */
void test_giolpt_sot_overflow(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_swd(0, 10);  /* Small width */
    for (int i = 0; i < 15; i++) {
        giolpt_sot(0, 'A');
    }
    giolpt_close(0);
    TEST_PASS("giolpt_sot_overflow");
}

/* Test giolpt_close then open again */
void test_giolpt_reopen(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_close(0);
    giolpt_open(0, 2);  /* Reopen */
    giolpt_sot(0, 'X');
    giolpt_close(0);
    TEST_PASS("giolpt_reopen");
}

/* Test invalid device operations */
void test_giolpt_invalid_device(void) {
    giolpt_init();
    giolpt_sot(-1, 'X');
    giolpt_sot(3, 'X');
    giolpt_close(-1);
    giolpt_close(3);
    TEST_PASS("giolpt_invalid_device");
}

/* Test multiple LPT devices */
void test_giolpt_multiple_devices(void) {
    giolpt_init();
    giolpt_open(0, 2);  /* LPT1 */
    giolpt_open(1, 2);  /* LPT2 */
    giolpt_open(2, 2);  /* LPT3 */
    
    giolpt_sot(0, '1');
    giolpt_sot(1, '2');
    giolpt_sot(2, '3');
    
    giolpt_close(0);
    giolpt_close(1);
    giolpt_close(2);
    TEST_PASS("giolpt_multiple_devices");
}

/* Test LPOS after output */
void test_giolpt_lpos_after_output(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, 'A');
    giolpt_sot(0, 'B');
    giolpt_sot(0, 'C');
    int lpos = giolpt_lpos(0);
    TEST_ASSERT_EQ_INT(lpos, 4, "LPOS returns 4 after 3 chars");
    giolpt_close(0);
    TEST_PASS("giolpt_lpos_after_output");
}

/* Test giolpt_sot - CR resets position */
void test_giolpt_sot_cr_resets_pos(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_sot(0, 'A');
    giolpt_sot(0, 'B');
    giolpt_sot(0, '\r');  /* CR */
    int lpos = giolpt_lpos(0);
    TEST_ASSERT_EQ_INT(lpos, 1, "LPOS returns 1 after CR");
    giolpt_close(0);
    TEST_PASS("giolpt_sot_cr_resets_pos");
}

/* Test close then get width */
void test_giolpt_gwd_after_close(void) {
    giolpt_init();
    giolpt_open(0, 2);
    giolpt_swd(0, 132);
    giolpt_close(0);
    int width = giolpt_gwd(0);
    TEST_ASSERT_EQ_INT(width, 80, "Width reset to default after close");
    TEST_PASS("giolpt_gwd_after_close");
}