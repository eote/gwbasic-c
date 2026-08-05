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
void test_dskcom_get_put(void);
void test_math1_init(void);
void test_math1_sin(void);
void test_math1_cos(void);
void test_math1_tan(void);
void test_math1_atn(void);
void test_math1_exp(void);
void test_math1_log(void);
void test_math2_init(void);
void test_math2_sqr(void);
void test_math2_abs(void);
void test_math2_sgn(void);
void test_math2_rnd(void);
void test_math2_int(void);
void test_math2_fix(void);
void test_math_expression_eval(void);

/* ADVGRP test functions */
void test_advgrp_paint_basic(void);
void test_advgrp_paint_border_color(void);
void test_advgrp_paint_out_of_bounds(void);
void test_advgrp_circle_basic(void);
void test_advgrp_circle_zero_radius(void);
void test_advgrp_get_basic(void);
void test_advgrp_get_null_array(void);
void test_advgrp_get_data_correctness(void);
void test_advgrp_get_put_roundtrip(void);
void test_advgrp_put_basic(void);
void test_advgrp_put_xor(void);
void test_advgrp_paint_after_circle(void);

/* CALL86 test functions */
void test_call86_init(void);
void test_call86_call_basic(void);
void test_call86_call_with_params(void);
void test_call86_calls_basic(void);
void test_call86_calls_with_params(void);
void test_call86_call_no_params(void);
void test_call86_usr_basic(void);
void test_call86_usr_all_indices(void);
void test_call86_usr_invalid(void);
void test_call86_defusr_basic(void);
void test_call86_defusr_invalid(void);
void test_call86_call_variable(void);
void test_call86_call_array_params(void);
void test_call86_call_empty_parens(void);

/* GWEVAL HELPER test functions */
void test_gweval_scan_var_basic(void);
void test_gweval_scan_var_array(void);
void test_gweval_scan_var_empty(void);
void test_gweval_frqint_basic(void);
void test_gweval_chrgt2_basic(void);
void test_gweval_chrgt2_empty(void);
void test_gweval_synchr_match(void);
void test_gweval_synchr_no_match(void);
void test_gweval_synchr_eos(void);
void test_gweval_synchr_null(void);

/* ITSA86 test functions */
void test_itsa86_init(void);
void test_itsa86_basvar_read_uninitialized(void);
void test_itsa86_basvar_write_uninitialized(void);
void test_itsa86_basvar_invalid_index(void);
void test_itsa86_basvar_all_indices_uninitialized(void);
void test_itsa86_mapclc_no_options(void);
void test_itsa86_mapclc_m_option_valid(void);
void test_itsa86_mapclc_m_option_too_large(void);
void test_itsa86_mapclc_c_option(void);
void test_itsa86_seg_off_valid(void);
void test_itsa86_seg_off_below_ds(void);
void test_itsa86_seg_off_at_ds(void);
void test_itsa86_seg_off_64k_limit(void);
void test_itsa86_get_memory_info(void);
void test_itsa86_set_c_option(void);
void test_itsa86_set_m_option(void);
void test_itsa86_mapini(void);
void test_itsa86_csend(void);

/* NEXT86 test functions */
void test_next86_init(void);
void test_next86_for_basic(void);
void test_next86_for_with_step(void);
void test_next86_for_nested(void);
void test_next86_next_basic(void);
void test_next86_next_termination(void);
void test_next86_next_no_for(void);
void test_next86_clear_for_stack(void);
void test_next86_get_for_depth(void);
void test_next86_inside_for(void);
void test_next86_for_max_depth(void);
void test_next86_variable_types(void);
void test_next86_step_negative(void);

/* KANJ86 test functions */
void test_kanj86_init(void);
void test_kanj86_ktnfn(void);
void test_kanj86_jisfn(void);
void test_kanj86_klenfn(void);
void test_kanj86_kposfn(void);
void test_kanj86_term(void);

/* MACLNG test functions */
void test_maclng_init(void);
void test_maclng_term(void);
void test_maclng_maclng_basic(void);
void test_maclng_mclxeq_basic(void);
void test_maclng_fetchr(void);
void test_maclng_fetchz(void);
void test_maclng_decfet(void);
void test_maclng_valscn(void);
void test_maclng_valsc2(void);
void test_maclng_varget(void);
void test_maclng_negd(void);
void test_maclng_islet2(void);
void test_maclng_scvar(void);
void test_maclng_mclnew(void);
void test_maclng_mclscn(void);
void test_maclng_maclng_main(void);

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
    failed += run_test("dskcom_get_put", test_dskcom_get_put);
    failed += run_test("math1_init", test_math1_init);
        failed += run_test("math1_sin", test_math1_sin);
        failed += run_test("math1_cos", test_math1_cos);
        failed += run_test("math1_tan", test_math1_tan);
        failed += run_test("math1_atn", test_math1_atn);
        failed += run_test("math1_exp", test_math1_exp);
        failed += run_test("math1_log", test_math1_log);
        failed += run_test("math2_init", test_math2_init);
        failed += run_test("math2_sqr", test_math2_sqr);
        failed += run_test("math2_abs", test_math2_abs);
        failed += run_test("math2_sgn", test_math2_sgn);
        failed += run_test("math2_rnd", test_math2_rnd);
        failed += run_test("math2_int", test_math2_int);
        failed += run_test("math2_fix", test_math2_fix);
        failed += run_test("math_expression_eval", test_math_expression_eval);
    
        /* ADVGRP tests */
        failed += run_test("advgrp_paint_basic", test_advgrp_paint_basic);
        failed += run_test("advgrp_paint_border_color", test_advgrp_paint_border_color);
        failed += run_test("advgrp_paint_out_of_bounds", test_advgrp_paint_out_of_bounds);
        failed += run_test("advgrp_circle_basic", test_advgrp_circle_basic);
        failed += run_test("advgrp_circle_zero_radius", test_advgrp_circle_zero_radius);
        failed += run_test("advgrp_get_basic", test_advgrp_get_basic);
        failed += run_test("advgrp_get_null_array", test_advgrp_get_null_array);
        failed += run_test("advgrp_get_data_correctness", test_advgrp_get_data_correctness);
        failed += run_test("advgrp_get_put_roundtrip", test_advgrp_get_put_roundtrip);
        failed += run_test("advgrp_put_basic", test_advgrp_put_basic);
        failed += run_test("advgrp_put_xor", test_advgrp_put_xor);
        failed += run_test("advgrp_paint_after_circle", test_advgrp_paint_after_circle);

        /* CALL86 tests */
        failed += run_test("call86_init", test_call86_init);
        failed += run_test("call86_call_basic", test_call86_call_basic);
        failed += run_test("call86_call_with_params", test_call86_call_with_params);
        failed += run_test("call86_calls_basic", test_call86_calls_basic);
        failed += run_test("call86_calls_with_params", test_call86_calls_with_params);
        failed += run_test("call86_call_no_params", test_call86_call_no_params);
        failed += run_test("call86_usr_basic", test_call86_usr_basic);
        failed += run_test("call86_usr_all_indices", test_call86_usr_all_indices);
        failed += run_test("call86_usr_invalid", test_call86_usr_invalid);
        failed += run_test("call86_defusr_basic", test_call86_defusr_basic);
        failed += run_test("call86_defusr_invalid", test_call86_defusr_invalid);
        failed += run_test("call86_call_variable", test_call86_call_variable);
        failed += run_test("call86_call_array_params", test_call86_call_array_params);
        failed += run_test("call86_call_empty_parens", test_call86_call_empty_parens);

        /* GWEVAL HELPER tests */
                failed += run_test("gweval_scan_var_basic", test_gweval_scan_var_basic);
                failed += run_test("gweval_scan_var_array", test_gweval_scan_var_array);
                failed += run_test("gweval_scan_var_empty", test_gweval_scan_var_empty);
                failed += run_test("gweval_frqint_basic", test_gweval_frqint_basic);
                failed += run_test("gweval_chrgt2_basic", test_gweval_chrgt2_basic);
                failed += run_test("gweval_chrgt2_empty", test_gweval_chrgt2_empty);
                failed += run_test("gweval_synchr_match", test_gweval_synchr_match);
                failed += run_test("gweval_synchr_no_match", test_gweval_synchr_no_match);
                failed += run_test("gweval_synchr_eos", test_gweval_synchr_eos);
                failed += run_test("gweval_synchr_null", test_gweval_synchr_null);

                /* ITSA86 tests */
                        failed += run_test("itsa86_init", test_itsa86_init);
                        failed += run_test("itsa86_basvar_read_uninitialized", test_itsa86_basvar_read_uninitialized);
                        failed += run_test("itsa86_basvar_write_uninitialized", test_itsa86_basvar_write_uninitialized);
                        failed += run_test("itsa86_basvar_invalid_index", test_itsa86_basvar_invalid_index);
                        failed += run_test("itsa86_basvar_all_indices_uninitialized", test_itsa86_basvar_all_indices_uninitialized);
                        failed += run_test("itsa86_mapclc_no_options", test_itsa86_mapclc_no_options);
                        failed += run_test("itsa86_mapclc_m_option_valid", test_itsa86_mapclc_m_option_valid);
                        failed += run_test("itsa86_mapclc_m_option_too_large", test_itsa86_mapclc_m_option_too_large);
                        failed += run_test("itsa86_mapclc_c_option", test_itsa86_mapclc_c_option);
                        failed += run_test("itsa86_seg_off_valid", test_itsa86_seg_off_valid);
                        failed += run_test("itsa86_seg_off_below_ds", test_itsa86_seg_off_below_ds);
                        failed += run_test("itsa86_seg_off_at_ds", test_itsa86_seg_off_at_ds);
                        failed += run_test("itsa86_seg_off_64k_limit", test_itsa86_seg_off_64k_limit);
                        failed += run_test("itsa86_get_memory_info", test_itsa86_get_memory_info);
                        failed += run_test("itsa86_set_c_option", test_itsa86_set_c_option);
                        failed += run_test("itsa86_set_m_option", test_itsa86_set_m_option);
                        failed += run_test("itsa86_mapini", test_itsa86_mapini);
                        failed += run_test("itsa86_csend", test_itsa86_csend);

                        /* NEXT86 tests */
                                failed += run_test("next86_init", test_next86_init);
                                failed += run_test("next86_for_basic", test_next86_for_basic);
                                failed += run_test("next86_for_with_step", test_next86_for_with_step);
                                failed += run_test("next86_for_nested", test_next86_for_nested);
                                failed += run_test("next86_next_basic", test_next86_next_basic);
                                failed += run_test("next86_next_termination", test_next86_next_termination);
                                failed += run_test("next86_next_no_for", test_next86_next_no_for);
                                failed += run_test("next86_clear_for_stack", test_next86_clear_for_stack);
                                failed += run_test("next86_get_for_depth", test_next86_get_for_depth);
                                failed += run_test("next86_inside_for", test_next86_inside_for);
                                failed += run_test("next86_for_max_depth", test_next86_for_max_depth);
                                failed += run_test("next86_variable_types", test_next86_variable_types);
                                failed += run_test("next86_step_negative", test_next86_step_negative);

                                /* KANJ86 tests */
                                failed += run_test("kanj86_init", test_kanj86_init);
                                failed += run_test("kanj86_ktnfn", test_kanj86_ktnfn);
                                failed += run_test("kanj86_jisfn", test_kanj86_jisfn);
                                failed += run_test("kanj86_klenfn", test_kanj86_klenfn);
                                failed += run_test("kanj86_kposfn", test_kanj86_kposfn);
                                failed += run_test("kanj86_term", test_kanj86_term);

                                /* MACLNG tests */
                                failed += run_test("maclng_init", test_maclng_init);
                                failed += run_test("maclng_term", test_maclng_term);
                                failed += run_test("maclng_maclng_basic", test_maclng_maclng_basic);
                                failed += run_test("maclng_mclxeq_basic", test_maclng_mclxeq_basic);
                                failed += run_test("maclng_fetchr", test_maclng_fetchr);
                                failed += run_test("maclng_fetchz", test_maclng_fetchz);
                                failed += run_test("maclng_decfet", test_maclng_decfet);
                                failed += run_test("maclng_valscn", test_maclng_valscn);
                                failed += run_test("maclng_valsc2", test_maclng_valsc2);
                                failed += run_test("maclng_varget", test_maclng_varget);
                                failed += run_test("maclng_negd", test_maclng_negd);
                                failed += run_test("maclng_islet2", test_maclng_islet2);
                                failed += run_test("maclng_scvar", test_maclng_scvar);
                                failed += run_test("maclng_mclnew", test_maclng_mclnew);
                                failed += run_test("maclng_mclscn", test_maclng_mclscn);
                                failed += run_test("maclng_maclng_main", test_maclng_maclng_main);

                                print_test_summary();
    
    return failed ? 1 : 0;
}