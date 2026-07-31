/* GW-BASIC C Port - GIOCON Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_giocon_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giocon_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("giocon_init");
}

void test_giocon_print(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giocon_init();
    
    /* Test print numeric */
    giocon_print_num(42);
    giocon_print_num(3.14f);
    
    /* Test print string */
    giocon_print_str("HELLO");
    
    TEST_PASS("print functions");
}

void test_giocon_locate(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giocon_init();
    
    /* Test cursor positioning */
    giocon_locate(10, 20);
    giocon_locate(1, 1);
    
    TEST_PASS("locate");
}

void test_giocon_color(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giocon_init();
    
    /* Test color setting */
    giocon_color(7, 0);  /* white on black */
    giocon_color(15, 1); /* bright white on blue */
    
    TEST_PASS("color");
}

void test_giocon_cls(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    giocon_init();
    
    giocon_cls();
    
    TEST_PASS("cls");
}