/* Minimal test - only gwinit_constants */
#include "test_framework.h"
#include "../src/gwbasic.h"

int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;

int main(void) {
    printf("=== Isolation test: gwinit_constants only ===\n");
    fflush(stdout);

    printf("Step 1: gwdata_init()\n");
    fflush(stdout);
    gwdata_init();

    printf("Step 2: gwram_init()\n");
    fflush(stdout);
    gwram_init();

    printf("Step 3: gwinit_init()\n");
    fflush(stdout);
    gwinit_init();

    printf("Step 4: find PI\n");
    fflush(stdout);
    struct variable *pi = gwdata_find_variable("PI");
    printf("PI = %p\n", (void*)pi);
    fflush(stdout);

    if (pi) {
        printf("PI->name=%s, PI->type=%d, PI->value.f=%f\n",
               pi->name, pi->type, pi->value.f);
        fflush(stdout);
    }

    printf("DONE - no crash\n");
    return 0;
}