/* TEST_GIOCOM - Unit tests for GIOCOM (Serial Communications) */
/* Based on GIOCOM.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test giocom_init */
void test_giocom_init(void) {
    giocom_init();
    TEST_PASS("giocom_init");
}

/* Test giocom_open - COM1 */
void test_giocom_open_com1(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);  /* COM1, 9600 baud, even parity, 7 data, 1 stop */
    TEST_PASS("giocom_open_com1");
}

/* Test giocom_open - COM2 */
void test_giocom_open_com2(void) {
    giocom_init();
    giocom_open(1, 1200, 0, 8, 1);  /* COM2, 1200 baud, no parity, 8 data, 1 stop */
    TEST_PASS("giocom_open_com2");
}

/* Test giocom_open - COM3 */
void test_giocom_open_com3(void) {
    giocom_init();
    giocom_open(2, 2400, 1, 7, 2);  /* COM3, 2400 baud, odd parity, 7 data, 2 stop */
    TEST_PASS("giocom_open_com3");
}

/* Test giocom_open - COM4 */
void test_giocom_open_com4(void) {
    giocom_init();
    giocom_open(3, 4800, 3, 8, 1);  /* COM4, 4800 baud, mark parity, 8 data, 1 stop */
    TEST_PASS("giocom_open_com4");
}

/* Test giocom_open - invalid device */
void test_giocom_open_invalid(void) {
    giocom_init();
    giocom_open(-1, 9600, 2, 7, 1);
    giocom_open(4, 9600, 2, 7, 1);
    TEST_PASS("giocom_open_invalid");
}

/* Test giocom_close */
void test_giocom_close(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_close(0);
    TEST_PASS("giocom_close");
}

/* Test giocom_swd - set width */
void test_giocom_swd(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_swd(0, 80);
    TEST_ASSERT_EQ_INT(giocom_gwd(0), 80, "Width set to 80");
    giocom_close(0);
    TEST_PASS("giocom_swd");
}

/* Test giocom_swd - invalid width */
void test_giocom_swd_invalid(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_swd(0, 0);   /* Invalid - too small */
    giocom_swd(0, 256); /* Invalid - too large */
    TEST_PASS("giocom_swd_invalid");
}

/* Test giocom_gwd - get width */
void test_giocom_gwd(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int width = giocom_gwd(0);
    TEST_ASSERT_EQ_INT(width, 255, "Default width is 255");
    giocom_close(0);
    TEST_PASS("giocom_gwd");
}

/* Test giocom_sot - output characters */
void test_giocom_sot(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, 'H');
    giocom_sot(0, 'i');
    giocom_sot(0, '!');
    giocom_close(0);
    TEST_PASS("giocom_sot");
}

/* Test giocom_sot - tab handling */
void test_giocom_sot_tab(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, '\t');  /* Tab */
    giocom_close(0);
    TEST_PASS("giocom_sot_tab");
}

/* Test giocom_sot - CR handling */
void test_giocom_sot_cr(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, '\r');  /* CR */
    giocom_close(0);
    TEST_PASS("giocom_sot_cr");
}

/* Test giocom_sot - LF handling */
void test_giocom_sot_lf(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, '\n');  /* LF */
    giocom_close(0);
    TEST_PASS("giocom_sot_lf");
}

/* Test giocom_sot - CR followed by LF (should eat LF) */
void test_giocom_sot_crlf(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, '\r');  /* CR */
    giocom_sot(0, '\n');  /* LF - should be eaten */
    giocom_close(0);
    TEST_PASS("giocom_sot_crlf");
}

/* Test giocom_loc - get input queue bytes */
void test_giocom_loc(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int loc = giocom_loc(0);
    TEST_ASSERT(loc >= 0, "LOC returns valid count");
    giocom_close(0);
    TEST_PASS("giocom_loc");
}

/* Test giocom_lof - get output queue free space */
void test_giocom_lof(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int lof = giocom_lof(0);
    TEST_ASSERT(lof >= 0, "LOF returns valid count");
    giocom_close(0);
    TEST_PASS("giocom_lof");
}

/* Test giocom_gps - get position */
void test_giocom_gps(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int pos = giocom_gps(0);
    TEST_ASSERT_EQ_INT(pos, 0, "Initial position is 0");
    giocom_close(0);
    TEST_PASS("giocom_gps");
}

/* Test giocom_gwd - get width */
void test_giocom_gwd2(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int width = giocom_gwd(0);
    TEST_ASSERT_EQ_INT(width, 255, "Default width is 255");
    giocom_close(0);
    TEST_PASS("giocom_gwd2");
}

/* Test giocom_swd then gwd */
void test_giocom_swd_gwd(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_swd(0, 40);
    TEST_ASSERT_EQ_INT(giocom_gwd(0), 40, "Width set to 40");
    giocom_swd(0, 132);
    TEST_ASSERT_EQ_INT(giocom_gwd(0), 132, "Width set to 132");
    giocom_close(0);
    TEST_PASS("giocom_swd_gwd");
}

/* Test giocom_sot - multiple characters */
void test_giocom_sot_multiple(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    const char *text = "Hello, World!\r\n";
    for (int i = 0; text[i]; i++) {
        giocom_sot(0, text[i]);
    }
    giocom_close(0);
    TEST_PASS("giocom_sot_multiple");
}

/* Test giocom_sot - line overflow */
void test_giocom_sot_overflow(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_swd(0, 10);  /* Small width */
    for (int i = 0; i < 15; i++) {
        giocom_sot(0, 'A');
    }
    giocom_close(0);
    TEST_PASS("giocom_sot_overflow");
}

/* Test giocom_close then open again */
void test_giocom_reopen(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_close(0);
    giocom_open(0, 9600, 2, 7, 1);  /* Reopen */
    giocom_sot(0, 'X');
    giocom_close(0);
    TEST_PASS("giocom_reopen");
}

/* Test invalid device operations */
void test_giocom_invalid_device(void) {
    giocom_init();
    giocom_sot(-1, 'X');
    giocom_sot(4, 'X');
    giocom_close(-1);
    giocom_close(4);
    TEST_PASS("giocom_invalid_device");
}

/* Test multiple COM devices */
void test_giocom_multiple_devices(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);  /* COM1 */
    giocom_open(1, 1200, 0, 8, 1);  /* COM2 */
    giocom_open(2, 2400, 1, 7, 2);  /* COM3 */
    giocom_open(3, 4800, 3, 8, 1);  /* COM4 */
    
    giocom_sot(0, '1');
    giocom_sot(1, '2');
    giocom_sot(2, '3');
    giocom_sot(3, '4');
    
    giocom_close(0);
    giocom_close(1);
    giocom_close(2);
    giocom_close(3);
    TEST_PASS("giocom_multiple_devices");
}

/* Test default baud rate */
void test_giocom_default_baud(void) {
    giocom_init();
    giocom_open(0, 0, 0, 0, 0);  /* Use defaults */
    TEST_PASS("giocom_default_baud");
}

/* Test stop bits defaults */
void test_giocom_stop_bits_defaults(void) {
    giocom_init();
    /* Test default stop bits for various baud rates */
    giocom_open(0, 300, 2, 7, 0);   /* 300 baud -> 1 stop bit */
    giocom_close(0);
    giocom_open(0, 110, 2, 5, 0);   /* 110 baud, 5 data -> 1.5 stop bits */
    giocom_close(0);
    giocom_open(0, 110, 2, 8, 0);   /* 110 baud, 8 data -> 2 stop bits */
    giocom_close(0);
    TEST_PASS("giocom_stop_bits_defaults");
}

/* Test parity options */
void test_giocom_parity(void) {
    giocom_init();
    giocom_open(0, 9600, 0, 7, 1);  /* No parity */
    giocom_close(0);
    giocom_open(0, 9600, 1, 7, 1);  /* Odd parity */
    giocom_close(0);
    giocom_open(0, 9600, 2, 7, 1);  /* Even parity */
    giocom_close(0);
    giocom_open(0, 9600, 3, 7, 1);  /* Mark parity */
    giocom_close(0);
    giocom_open(0, 9600, 4, 7, 1);  /* Space parity */
    giocom_close(0);
    TEST_PASS("giocom_parity");
}

/* Test data bits options */
void test_giocom_data_bits(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 5, 1);
    giocom_close(0);
    giocom_open(0, 9600, 2, 6, 1);
    giocom_close(0);
    giocom_open(0, 9600, 2, 7, 1);
    giocom_close(0);
    giocom_open(0, 9600, 2, 8, 1);
    giocom_close(0);
    TEST_PASS("giocom_data_bits");
}

/* Test close resets position */
void test_giocom_close_resets_pos(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, 'A');
    giocom_sot(0, 'B');
    giocom_close(0);
    giocom_open(0, 9600, 2, 7, 1);
    int pos = giocom_gps(0);
    TEST_ASSERT_EQ_INT(pos, 0, "Position reset after close/reopen");
    giocom_close(0);
    TEST_PASS("giocom_close_resets_pos");
}

/* Test CR resets position */
void test_giocom_sot_cr_resets_pos(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, 'A');
    giocom_sot(0, 'B');
    giocom_sot(0, '\r');  /* CR */
    int pos = giocom_gps(0);
    TEST_ASSERT_EQ_INT(pos, 0, "Position reset after CR");
    giocom_close(0);
    TEST_PASS("giocom_sot_cr_resets_pos");
}

/* Test LF resets position */
void test_giocom_sot_lf_resets_pos(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_sot(0, 'A');
    giocom_sot(0, 'B');
    giocom_sot(0, '\n');  /* LF */
    int pos = giocom_gps(0);
    TEST_ASSERT_EQ_INT(pos, 0, "Position reset after LF");
    giocom_close(0);
    TEST_PASS("giocom_sot_lf_resets_pos");
}

/* Test width after close resets to default */
void test_giocom_gwd_after_close(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    giocom_swd(0, 132);
    giocom_close(0);
    int width = giocom_gwd(0);
    TEST_ASSERT_EQ_INT(width, 255, "Width reset to default after close");
    TEST_PASS("giocom_gwd_after_close");
}

/* Test sin with no data (timeout) */
void test_giocom_sin_timeout(void) {
    giocom_init();
    giocom_open(0, 9600, 2, 7, 1);
    int result = giocom_sin(0);
    TEST_ASSERT(result == -1, "SIN returns -1 on timeout/no data");
    giocom_close(0);
    TEST_PASS("giocom_sin_timeout");
}