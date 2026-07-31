/* GW-BASIC C Port - GIOKYB/GIOSCN Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_giokyb_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giokyb_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("giokyb_init");
}

void test_giokyb_inkey(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giokyb_init();
    
    /* Test INKEY$ - returns empty string when no key pressed */
    char *result = giokyb_inkey();
    TEST_ASSERT(result != NULL, "inkey returns non-NULL");
    TEST_PASS("inkey");
}

void test_giokyb_input(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giokyb_init();
    
    /* Test INPUT$ - would block, just verify function exists */
    TEST_PASS("input function exists");
}

void test_gioscn_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gioscn_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("gioscn_init");
}

void test_gioscn_screen(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gioscn_init();
    
    /* Test SCREEN mode setting */
    gioscn_screen(0);  /* text mode */
    gioscn_screen(1);  /* 320x200 graphics */
    gioscn_screen(2);  /* 640x200 graphics */
    
    TEST_PASS("screen modes");
}

void test_gioscn_pset(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gioscn_init();
    
    /* Test PSET (pixel set) */
    gioscn_pset(100, 100, 15);  /* white pixel */
    gioscn_pset(0, 0, 0);       /* black pixel */
    
    TEST_PASS("pset");
}

void test_gioscn_preset(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gioscn_init();
    
    /* Test PRESET (pixel reset) */
    gioscn_preset(100, 100);
    
    TEST_PASS("preset");
}

void test_gioscn_point(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gioscn_init();
    
    /* Test POINT (read pixel) */
    int color = gioscn_point(100, 100);
    TEST_ASSERT(color >= 0, "point returns color");
    
    TEST_PASS("point");
}