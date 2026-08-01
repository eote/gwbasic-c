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
void test_gweval_literal(void);
void test_gweval_binary_ops(void);
void test_gweval_precedence(void);
void test_gweval_variables(void);
void test_gwsts_let(void);
void test_gwsts_print(void);
void test_gwsts_if(void);
void test_giocon_init(void);
void test_giocon_print(void);
void test_giocon_locate(void);
void test_giocon_color(void);
void test_giocon_cls(void);
void test_giokyb_init(void);
void test_giokyb_inkey(void);
void test_giokyb_input(void);
void test_gioscn_init(void);
void test_gioscn_screen(void);
void test_gioscn_pset(void);
void test_gioscn_preset(void);
void test_gioscn_point(void);
void test_gwlist_init(void);
void test_gwlist_add_line(void);
void test_gwlist_delete_line(void);
void test_gwlist_list(void);
void test_bistrs_init(void);
void test_bistrs_left_right_mid(void);
void test_bistrs_len_asc_chr(void);
void test_bistrs_str_val(void);
void test_bistrs_string_space(void);
void test_biprtu_init(void);
void test_biprtu_print_using(void);
void test_biptrg_init(void);
void test_biptrg_array(void);
void test_biptrg_string_array(void);
void test_variable_reassignment(void);
void test_array_bounds(void);
void test_dskcom_init(void);
void test_dskcom_open_close(void);
void test_dskcom_print(void);
void test_dskcom_input(void);
void test_dskcom_line_input(void);
void test_dskcom_write_seek(void);

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
    failed += run_test("gweval_literal", test_gweval_literal);
    failed += run_test("gweval_binary_ops", test_gweval_binary_ops);
    failed += run_test("gweval_precedence", test_gweval_precedence);
    failed += run_test("gweval_variables", test_gweval_variables);
    failed += run_test("gwsts_let", test_gwsts_let);
    failed += run_test("gwsts_print", test_gwsts_print);
    failed += run_test("gwsts_if", test_gwsts_if);
    failed += run_test("giocon_init", test_giocon_init);
    failed += run_test("giocon_print", test_giocon_print);
    failed += run_test("giocon_locate", test_giocon_locate);
    failed += run_test("giocon_color", test_giocon_color);
    failed += run_test("giocon_cls", test_giocon_cls);
    failed += run_test("giokyb_init", test_giokyb_init);
    failed += run_test("giokyb_inkey", test_giokyb_inkey);
    failed += run_test("giokyb_input", test_giokyb_input);
    failed += run_test("gioscn_init", test_gioscn_init);
    failed += run_test("gioscn_screen", test_gioscn_screen);
    failed += run_test("gioscn_pset", test_gioscn_pset);
    failed += run_test("gioscn_preset", test_gioscn_preset);
    failed += run_test("gioscn_point", test_gioscn_point);
    failed += run_test("gwlist_init", test_gwlist_init);
    failed += run_test("gwlist_add_line", test_gwlist_add_line);
    failed += run_test("gwlist_delete_line", test_gwlist_delete_line);
    failed += run_test("gwlist_list", test_gwlist_list);
    failed += run_test("bistrs_init", test_bistrs_init);
    failed += run_test("bistrs_left_right_mid", test_bistrs_left_right_mid);
    failed += run_test("bistrs_len_asc_chr", test_bistrs_len_asc_chr);
    failed += run_test("bistrs_str_val", test_bistrs_str_val);
    failed += run_test("bistrs_string_space", test_bistrs_string_space);
    failed += run_test("biprtu_init", test_biprtu_init);
    failed += run_test("biprtu_print_using", test_biprtu_print_using);
    failed += run_test("biptrg_init", test_biptrg_init);
    failed += run_test("biptrg_array", test_biptrg_array);
    failed += run_test("biptrg_string_array", test_biptrg_string_array);
    failed += run_test("variable_reassignment", test_variable_reassignment);
    failed += run_test("array_bounds", test_array_bounds);
    failed += run_test("dskcom_init", test_dskcom_init);
    failed += run_test("dskcom_open_close", test_dskcom_open_close);
    failed += run_test("dskcom_print", test_dskcom_print);
    failed += run_test("dskcom_input", test_dskcom_input);
    failed += run_test("dskcom_line_input", test_dskcom_line_input);
    failed += run_test("dskcom_write_seek", test_dskcom_write_seek);
    
    print_test_summary();
    
    return failed ? 1 : 0;
}