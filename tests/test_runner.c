/* GW-BASIC C Port - Test Runner */
/* Main entry point for test executable */

#include "test_framework.h"

/* Global test statistics definitions */
int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;

/* Run a single test function */
int run_test(const char *name, test_func_t test_func) {
    printf("RUN: %s... ", name);
    fflush(stdout);
    
    int tests_before = g_tests_run;
    int failed_before = g_tests_failed;
    
    /* Run the test */
    test_func();
    
    int tests_in_suite = g_tests_run - tests_before;
    int failed_in_suite = g_tests_failed - failed_before;
    int passed_in_suite = tests_in_suite - failed_in_suite;
    
    if (failed_in_suite == 0) {
        printf("OK (%d passed)\n", passed_in_suite);
    } else {
        printf("FAILED (%d passed, %d failed)\n", passed_in_suite, failed_in_suite);
    }
    
    return failed_in_suite;
}

/* Print test summary */
void print_test_summary(void) {
    printf("\n========================\n");
    printf("TEST SUMMARY\n");
    printf("========================\n");
    printf("Total tests run:   %d\n", g_tests_run);
    printf("Tests passed:      %d\n", g_tests_passed);
    printf("Tests failed:      %d\n", g_tests_failed);
    printf("========================\n");
    
    if (g_tests_failed == 0) {
        printf("ALL TESTS PASSED!\n");
    } else {
        printf("SOME TESTS FAILED!\n");
    }
}

/* External test functions */
void test_gwdata_init(void);
void test_gwdata_variable_table(void);
void test_gwdata_string_heap(void);
void test_gwram_alloc_free(void);
void test_gwram_realloc(void);
void test_gwram_calloc(void);
void test_gwram_oom_handling(void);
void test_gwinit_basic(void);
void test_gwinit_constants(void);
void test_gwinit_function_table(void);

int main(void) {
    printf("GW-BASIC C Port - Test Suite\n");
    printf("============================\n");
    
    int failed = 0;
    failed += run_test("gwdata_init", test_gwdata_init);
    failed += run_test("gwdata_variable_table", test_gwdata_variable_table);
    failed += run_test("gwdata_string_heap", test_gwdata_string_heap);
    failed += run_test("gwram_alloc_free", test_gwram_alloc_free);
    failed += run_test("gwram_realloc", test_gwram_realloc);
    failed += run_test("gwram_calloc", test_gwram_calloc);
    failed += run_test("gwram_oom_handling", test_gwram_oom_handling);
    failed += run_test("gwinit_basic", test_gwinit_basic);
    failed += run_test("gwinit_constants", test_gwinit_constants);
    failed += run_test("gwinit_function_table", test_gwinit_function_table);
    
    print_test_summary();
    
    return failed ? 1 : 0;
}