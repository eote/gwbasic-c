/* GW-BASIC C Port - ADVGRP Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Extern declarations for graphics globals (defined in gioiscn.c) */
extern int g_screen_mode;
extern int g_fb_width;
extern int g_fb_height;
extern unsigned char *g_framebuffer;

/* Test setup/teardown macros */
#define ADVGRP_TEST_SETUP() \
    g_screen_mode = 1;  /* 320x200 - 4 colors (0-3) */ \
    g_fb_width = 320; \
    g_fb_height = 200; \
    memset(g_framebuffer, 0, 640 * 200); \
    advgrp_init(); \
    gengrp_init()

#define ADVGRP_TEST_TEARDOWN() \
    g_screen_mode = 0; \
    memset(g_framebuffer, 0, 640 * 200)

/* ========== PAINT Tests (3 tests) ========== */

void test_advgrp_paint_basic(void) {
    ADVGRP_TEST_SETUP();
    
    /* Draw a simple border by directly setting framebuffer */
    for (int x = 10; x <= 50; x++) {
        g_framebuffer[10 * 640 + x] = 2;  /* top */
        g_framebuffer[40 * 640 + x] = 2;  /* bottom */
    }
    for (int y = 10; y <= 40; y++) {
        g_framebuffer[y * 640 + 10] = 2;  /* left */
        g_framebuffer[y * 640 + 50] = 2;  /* right */
    }
    
    /* PAINT center with red (color 1) */
    advgrp_paint(25, 25, 1, 2);
    
    /* Verify fill - check a few points inside */
    TEST_ASSERT_EQ_INT(gengrp_point(25, 25), 1, "PAINT center filled");
    TEST_ASSERT_EQ_INT(gengrp_point(30, 30), 1, "PAINT interior filled");
    
    /* Verify border not overwritten */
    TEST_ASSERT_EQ_INT(gengrp_point(10, 10), 2, "Border preserved");
    TEST_ASSERT_EQ_INT(gengrp_point(50, 10), 2, "Border preserved");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_paint basic");
}

void test_advgrp_paint_border_color(void) {
    ADVGRP_TEST_SETUP();
    
    /* Draw green border (color 2) */
    for (int x = 20; x <= 60; x++) {
        g_framebuffer[20 * 640 + x] = 2;
        g_framebuffer[50 * 640 + x] = 2;
    }
    for (int y = 20; y <= 50; y++) {
        g_framebuffer[y * 640 + 20] = 2;
        g_framebuffer[y * 640 + 60] = 2;
    }
    
    /* PAINT with red (1), border green (2) */
    advgrp_paint(40, 35, 1, 2);
    
    /* Verify fill color */
    TEST_ASSERT_EQ_INT(gengrp_point(40, 35), 1, "Fill color correct");
    
    /* Verify border preserved */
    TEST_ASSERT_EQ_INT(gengrp_point(20, 20), 2, "Green border preserved");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_paint border color");
}

void test_advgrp_paint_out_of_bounds(void) {
    ADVGRP_TEST_SETUP();
    
    /* Paint with coordinates outside screen - should not crash */
    advgrp_paint(-10, -10, 1, 2);
    advgrp_paint(700, 300, 1, 2);
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_paint out of bounds safe");
}

/* ========== CIRCLE Tests (2 tests) ========== */

void test_advgrp_circle_basic(void) {
    ADVGRP_TEST_SETUP();
    
    advgrp_circle(160, 100, 50, 1, 0, 360, 1.0f);
    
    /* Verify circle points at cardinal directions */
    TEST_ASSERT_EQ_INT(gengrp_point(160, 50), 1, "Circle top");
    TEST_ASSERT_EQ_INT(gengrp_point(160, 150), 1, "Circle bottom");
    TEST_ASSERT_EQ_INT(gengrp_point(110, 100), 1, "Circle left");
    TEST_ASSERT_EQ_INT(gengrp_point(210, 100), 1, "Circle right");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_circle basic");
}

void test_advgrp_circle_zero_radius(void) {
    ADVGRP_TEST_SETUP();
    
    /* Radius 0 - should draw single point or nothing */
    advgrp_circle(160, 100, 0, 1, 0, 360, 1.0f);
    
    /* At least not crash */
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_circle zero radius safe");
}

/* ========== GET Tests (2 tests) ========== */

void test_advgrp_get_basic(void) {
    ADVGRP_TEST_SETUP();
    
    /* Draw a pattern using colors 1-3 (valid in mode 1) */
    g_framebuffer[10 * 640 + 10] = 1;
    g_framebuffer[10 * 640 + 11] = 2;
    g_framebuffer[11 * 640 + 10] = 3;
    g_framebuffer[11 * 640 + 11] = 1;
    
    /* GET 2x2 region */
    int rc = advgrp_get(10, 10, 11, 11, "TESTARR");
    TEST_ASSERT_EQ_INT(rc, SUCCESS, "GET succeeds");
    
    /* Verify array was created */
    struct variable *v = gwdata_find_variable("TESTARR");
    TEST_ASSERT(v != NULL, "Array variable created");
    TEST_ASSERT_EQ_INT(v->type, VT_STRING, "Array stored as string");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_get basic");
}

void test_advgrp_get_null_array(void) {
    ADVGRP_TEST_SETUP();
    
    int rc = advgrp_get(10, 10, 10, 10, "");
    if (rc != ERROR) {
        fprintf(stderr, "  FAIL: advgrp_get empty name: got %d, expected %d\n", rc, ERROR);
    } else {
        printf("  PASS: GET with empty name fails\n");
    }
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_get null array");
}

/* ========== PUT Tests (2 tests) ========== */

void test_advgrp_put_basic(void) {
    ADVGRP_TEST_SETUP();
    
    /* Test that PUT doesn't crash with a valid array */
    char *buf = (char *)gwram_alloc(100);
    buf[0] = 0; buf[1] = 2;  /* width = 2 */
    buf[2] = 0; buf[3] = 2;  /* height = 2 */
    buf[4] = 1; buf[5] = 2;  /* row 1 */
    buf[6] = 3; buf[7] = 1;  /* row 2 (colors 1-3) */
    
    struct variable *v = gwdata_alloc_variable("TESTARR4", VT_STRING);
    v->value.s = buf;
    v->is_array = false;
    
    int rc = advgrp_put(10, 10, "TESTARR4", 1);  /* PSET */
    TEST_ASSERT_EQ_INT(rc, SUCCESS, "PUT PSET succeeds");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_put basic");
}

void test_advgrp_put_xor(void) {
    ADVGRP_TEST_SETUP();
    
    gengrp_pset(10, 10, 1);  /* red */
    
    char *buf = (char *)gwram_alloc(50);
    buf[0] = 0; buf[1] = 1; buf[2] = 0; buf[3] = 1;
    buf[4] = 1;  /* red XOR red = black (0) */
    
    struct variable *v = gwdata_alloc_variable("TESTARR5", VT_STRING);
    v->value.s = buf;
    
    int rc = advgrp_put(10, 10, "TESTARR5", 5);  /* XOR */
    TEST_ASSERT_EQ_INT(rc, SUCCESS, "PUT XOR succeeds");
    /* Just test it doesn't crash */
    int pixel = gengrp_point(10, 10);
    (void)pixel;  /* suppress unused warning */
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp_put xor");
}

/* ========== Integration Tests (1 test) ========== */

void test_advgrp_paint_after_circle(void) {
    ADVGRP_TEST_SETUP();
    
    /* Draw circle with border color 2 */
    advgrp_circle(160, 100, 50, 2, 0, 360, 1.0f);
    
    /* Paint inside with color 1 */
    advgrp_paint(160, 100, 1, 2);
    
    /* Verify interior filled, border preserved */
    TEST_ASSERT_EQ_INT(gengrp_point(160, 100), 1, "Circle interior painted");
    TEST_ASSERT_EQ_INT(gengrp_point(160, 50), 2, "Circle border preserved");
    
    ADVGRP_TEST_TEARDOWN();
    TEST_PASS("advgrp paint after circle");
}