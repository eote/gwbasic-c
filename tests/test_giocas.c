/* TEST_GIOCAS - Unit tests for GIOCAS (Cassette I/O) */
/* Based on GIOCAS.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test giocas_init */
void test_giocas_init(void) {
    giocas_init();
    
    TEST_PASS("giocas_init");
}

/* Test giocas_term */
void test_giocas_term(void) {
    giocas_term();
    
    TEST_PASS("giocas_term");
}

/* Test giocas_motor returns device unavailable */
void test_giocas_motor(void) {
    const char *text = "test";
    int result = giocas_motor(&text);
    
    TEST_ASSERT_EQ_INT(result, -1, "motor returns device unavailable");
    TEST_PASS("giocas_motor");
}

/* Test giocas_read returns device unavailable */
void test_giocas_read(void) {
    const char *text = "test";
    char buffer[256];
    int result = giocas_read(&text, buffer, 256);
    
    TEST_ASSERT_EQ_INT(result, -1, "read returns device unavailable");
    TEST_PASS("giocas_read");
}

/* Test giocas_write returns device unavailable */
void test_giocas_write(void) {
    const char *text = "test";
    const char buffer[] = "test data";
    int result = giocas_write(&text, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQ_INT(result, -1, "write returns device unavailable");
    TEST_PASS("giocas_write");
}

/* Test giocas_ioctl returns device unavailable */
void test_giocas_ioctl(void) {
    const char *text = "test";
    int result = giocas_ioctl(&text, 0, NULL);
    
    TEST_ASSERT_EQ_INT(result, -1, "ioctl returns device unavailable");
    TEST_PASS("giocas_ioctl");
}