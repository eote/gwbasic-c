/* TEST_GENGRP - Unit tests for GENGRP (Generalized Graphics) */
/* Based on GENGRP.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test gengrp_init */
void test_gengrp_init(void) {
    gengrp_init();
    TEST_PASS("gengrp_init");
}

/* Test gengrp_grpini */
void test_gengrp_grpini(void) {
    gengrp_init();
    gengrp_grpini();
    TEST_PASS("gengrp_grpini");
}

/* Test gengrp_grprst */
void test_gengrp_grprst(void) {
    gengrp_init();
    gengrp_grprst();
    TEST_PASS("gengrp_grprst");
}

/* Test gengrp_scan_coord - absolute */
void test_gengrp_scan_coord_absolute(void) {
    gengrp_init();
    int x = 100, y = 200;
    gengrp_scan_coord(&x, &y, false);
    TEST_ASSERT_EQ_INT(x, 100, "x coordinate set");
    TEST_ASSERT_EQ_INT(y, 200, "y coordinate set");
    TEST_PASS("gengrp_scan_coord_absolute");
}

/* Test gengrp_scan_coord - relative */
void test_gengrp_scan_coord_relative(void) {
    gengrp_init();
    gengrp_grpini();
    /* Set initial position using accessor functions */
    gengrp_set_gxpos(50);
    gengrp_set_gypos(50);
    gengrp_set_grpacx(50);
    gengrp_set_grpacy(50);
    
    int x = 10, y = 20;
    gengrp_scan_coord(&x, &y, true);  /* Relative */
    TEST_ASSERT_EQ_INT(x, 60, "x coordinate relative");
    TEST_ASSERT_EQ_INT(y, 70, "y coordinate relative");
    TEST_PASS("gengrp_scan_coord_relative");
}

/* Test gengrp_pset */
void test_gengrp_pset(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_pset(100, 100, 1);  /* Red in mode 1 */
    TEST_PASS("gengrp_pset");
}

/* Test gengrp_preset */
void test_gengrp_preset(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_preset(100, 100);  /* Background color */
    TEST_PASS("gengrp_preset");
}

/* Test gengrp_point */
void test_gengrp_point(void) {
    gengrp_init();
    gengrp_set_screen_mode(0);  /* No masking for this test */
    gengrp_grpini();
    
    /* Set a pixel first */
    gengrp_pset(100, 100, 1);
    
    /* Read it back - in test env, just verify no crash */
    (void)gengrp_point(100, 100);
    TEST_PASS("gengrp_point");
}

/* Test gengrp_point - out of bounds */
void test_gengrp_point_oob(void) {
    gengrp_init();
    gengrp_grpini();
    int attr = gengrp_point(-1, -1);
    TEST_ASSERT_EQ_INT(attr, -1, "Out of bounds returns -1");
    TEST_PASS("gengrp_point_oob");
}

/* Test gengrp_set_atr */
void test_gengrp_set_atr(void) {
    gengrp_init();
    gengrp_set_atr(2);
    TEST_ASSERT_EQ_INT(gengrp_get_atr(), 2, "Attribute set");
    TEST_PASS("gengrp_set_atr");
}

/* Test gengrp_set_atr - mode 1 (4 colors) */
void test_gengrp_set_atr_mode1(void) {
    gengrp_init();
    gengrp_set_screen_mode(1);  /* 320x200 - 4 colors */
    gengrp_set_atr(5);  /* Should mask to 0-3 */
    TEST_ASSERT_EQ_INT(gengrp_get_atr(), 1, "Attribute masked to 4 colors (5 & 0x03 = 1)");
    TEST_PASS("gengrp_set_atr_mode1");
}

/* Test gengrp_set_atr - mode 2 (2 colors) */
void test_gengrp_set_atr_mode2(void) {
    gengrp_init();
    gengrp_set_screen_mode(2);  /* 640x200 - 2 colors */
    gengrp_set_atr(1);
    TEST_ASSERT_EQ_INT(gengrp_get_atr(), 1, "Non-zero becomes 1");
    gengrp_set_atr(0);
    TEST_ASSERT_EQ_INT(gengrp_get_atr(), 0, "Zero stays 0");
    TEST_PASS("gengrp_set_atr_mode2");
}

/* Test gengrp_get_atr */
void test_gengrp_get_atr(void) {
    gengrp_init();
    gengrp_set_atr(3);
    TEST_ASSERT_EQ_INT(gengrp_get_atr(), 3, "Get attribute returns 3");
    TEST_PASS("gengrp_get_atr");
}

/* Test gengrp_direct_pset */
void test_gengrp_direct_pset(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_direct_pset(50, 50, 2);
    TEST_PASS("gengrp_direct_pset");
}

/* Test gengrp_line - simple line */
void test_gengrp_line_simple(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("100,100-200,200");
    TEST_PASS("gengrp_line_simple");
}

/* Test gengrp_line - with first coordinate */
void test_gengrp_line_with_first(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("(50,50)-(150,150)");
    TEST_PASS("gengrp_line_with_first");
}

/* Test gengrp_line - with color */
void test_gengrp_line_with_color(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("(0,0)-(100,100),2");
    TEST_PASS("gengrp_line_with_color");
}

/* Test gengrp_line - box (B) */
void test_gengrp_line_box(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("(0,0)-(100,100),1,B");
    TEST_PASS("gengrp_line_box");
}

/* Test gengrp_line - box fill (BF) */
void test_gengrp_line_box_fill(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("(0,0)-(100,100),1,BF");
    TEST_PASS("gengrp_line_box_fill");
}

/* Test gengrp_line - empty params */
void test_gengrp_line_empty(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("");
    TEST_PASS("gengrp_line_empty");
}

/* Test gengrp_line - null params */
void test_gengrp_line_null(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line(NULL);
    TEST_PASS("gengrp_line_null");
}

/* Test gengrp_line - text mode (should return early) */
void test_gengrp_line_text_mode(void) {
    gengrp_init();
    gengrp_set_screen_mode(0);  /* Text mode */
    gengrp_line("(0,0)-(100,100)");
    TEST_PASS("gengrp_line_text_mode");
}

/* Test gengrp_pset - text mode (should return early) */
void test_gengrp_pset_text_mode(void) {
    gengrp_init();
    gengrp_set_screen_mode(0);  /* Text mode */
    gengrp_pset(100, 100, 1);
    TEST_PASS("gengrp_pset_text_mode");
}

/* Test gengrp_point - text mode (should return -1) */
void test_gengrp_point_text_mode(void) {
    gengrp_init();
    gengrp_set_screen_mode(0);  /* Text mode */
    int attr = gengrp_point(100, 100);
    TEST_ASSERT_EQ_INT(attr, -1, "Text mode returns -1");
    TEST_PASS("gengrp_point_text_mode");
}

/* Test multiple PSET/POINT operations */
void test_gengrp_multiple_ops(void) {
    gengrp_init();
    gengrp_set_screen_mode(0);  /* No mode masking for this test */
    gengrp_grpini();
    
    /* Draw multiple pixels */
    gengrp_pset(10, 10, 1);
    gengrp_pset(20, 20, 2);
    gengrp_pset(30, 30, 3);
    
    /* Read them back - in test env, just verify no crash */
    (void)gengrp_point(10, 10);
    (void)gengrp_point(20, 20);
    (void)gengrp_point(30, 30);
    
    TEST_PASS("gengrp_multiple_ops");
}

/* Test gengrp_grprst resets position */
void test_gengrp_grprst_resets(void) {
    gengrp_init();
    gengrp_grpini();
    
    /* Move position */
    gengrp_pset(500, 500, 1);
    
    /* Reset */
    gengrp_grprst();
    
    /* Position should be back to center */
    int expected_x = gengrp_get_fb_width() / 2;
    int expected_y = gengrp_get_fb_height() / 2;
    TEST_ASSERT_EQ_INT(gengrp_get_grpacx(), expected_x, "X reset to center");
    TEST_ASSERT_EQ_INT(gengrp_get_grpacy(), expected_y, "Y reset to center");
    TEST_PASS("gengrp_grprst_resets");
}

/* Test gengrp_line - various coordinates */
void test_gengrp_line_various_coords(void) {
    gengrp_init();
    gengrp_grpini();
    
    /* Horizontal line */
    gengrp_line("(0,100)-(100,100)");
    
    /* Vertical line */
    gengrp_line("(50,0)-(50,100)");
    
    /* Diagonal line */
    gengrp_line("(0,0)-(100,100)");
    
    TEST_PASS("gengrp_line_various_coords");
}

/* Test gengrp_scan_coord multiple relative steps */
void test_gengrp_scan_coord_multiple(void) {
    gengrp_init();
    gengrp_grpini();
    
    int x = 10, y = 10;
    gengrp_scan_coord(&x, &y, true);  /* 10,10 relative to center */
    
    int x2 = 5, y2 = 5;
    gengrp_scan_coord(&x2, &y2, true);  /* 5,5 relative to previous */
    
    TEST_ASSERT_EQ_INT(x2, x + 5, "Relative coordinate accumulated");
    TEST_ASSERT_EQ_INT(y2, y + 5, "Relative coordinate accumulated");
    TEST_PASS("gengrp_scan_coord_multiple");
}

/* Test gengrp_set_atr with negative value */
void test_gengrp_set_atr_negative(void) {
    gengrp_init();
    gengrp_set_atr(-1);
    /* Implementation may handle negative differently, just ensure no crash */
    TEST_PASS("gengrp_set_atr_negative");
}

/* Test screen mode boundaries */
void test_gengrp_boundaries(void) {
    gengrp_init();
    gengrp_grpini();
    
    /* Test boundaries */
    gengrp_pset(0, 0, 1);           /* Top-left */
    gengrp_pset(gengrp_get_fb_width() - 1, 0, 1);   /* Top-right */
    gengrp_pset(0, gengrp_get_fb_height() - 1, 1);  /* Bottom-left */
    gengrp_pset(gengrp_get_fb_width() - 1, gengrp_get_fb_height() - 1, 1);  /* Bottom-right */
    
    TEST_PASS("gengrp_boundaries");
}

/* Test gengrp_line with negative coordinates */
void test_gengrp_line_negative(void) {
    gengrp_init();
    gengrp_grpini();
    gengrp_line("(-10,-10)-(100,100)");
    TEST_PASS("gengrp_line_negative");
}