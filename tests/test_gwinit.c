/* GW-BASIC C Port - GWINIT Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwinit_basic(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_ASSERT(g_state->running == true, "running flag");
    TEST_ASSERT(g_state->in_direct_mode == true, "direct mode");
    TEST_PASS("gwinit basic init");
}

void test_gwinit_constants(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    /* Test that built-in constants are set */
    struct variable *pi = gwdata_find_variable("PI");
    TEST_ASSERT(pi != NULL, "PI constant exists");
    TEST_ASSERT(pi->type == VT_SINGLE, "PI is single precision");
    TEST_ASSERT(pi->value.f > 3.14 && pi->value.f < 3.15, "PI value correct");
    
    TEST_PASS("built-in constants");
}

void test_gwinit_function_table(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    /* Test that function table is populated */
    /* For now just verify init runs without error */
    TEST_PASS("function table init");
}