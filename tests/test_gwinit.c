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
    printf("DEBUG: test_gwinit_constants start\n");
    fflush(stdout);
    
    printf("DEBUG: calling gwdata_init()\n");
    fflush(stdout);
    gwdata_init();
    
    printf("DEBUG: after gwdata_init\n");
    fflush(stdout);
    
    printf("DEBUG: calling gwram_init()\n");
    fflush(stdout);
    gwram_init();
    
    printf("DEBUG: after gwram_init\n");
    fflush(stdout);
    
    printf("DEBUG: calling gwinit_init()\n");
    fflush(stdout);
    gwinit_init();
    
    printf("DEBUG: after gwinit_init\n");
    fflush(stdout);
    
    /* Test that built-in constants are set */
    printf("DEBUG: calling gwdata_find_variable(\"PI\")\n");
    fflush(stdout);
    
    struct variable *pi = gwdata_find_variable("PI");
    printf("DEBUG: after gwdata_find_variable, pi=%p\n", (void*)pi);
    fflush(stdout);
    
    if (pi) {
        printf("DEBUG: pi->name=%s, pi->type=%d, pi->value.f=%f\n", pi->name, pi->type, pi->value.f);
        fflush(stdout);
    }
    
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