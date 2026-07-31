/* GW-BASIC C Port - GWLIST/BISTRS Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwlist_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwlist_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("gwlist_init");
}

void test_gwlist_add_line(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwlist_init();
    
    /* Add program lines */
    gwlist_add_line(10, "PRINT \"HELLO\"");
    gwlist_add_line(20, "A = 5");
    gwlist_add_line(30, "END");
    
    struct program_line *line = gwlist_find_line(10);
    TEST_ASSERT(line != NULL, "line 10 exists");
    TEST_ASSERT_EQ_INT(line->number, 10, "line number");
    
    line = gwlist_find_line(20);
    TEST_ASSERT(line != NULL, "line 20 exists");
    
    TEST_PASS("add/find lines");
}

void test_gwlist_delete_line(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwlist_init();
    
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    
    gwlist_delete_line(10);
    struct program_line *line = gwlist_find_line(10);
    TEST_ASSERT(line == NULL, "line 10 deleted");
    
    line = gwlist_find_line(20);
    TEST_ASSERT(line != NULL, "line 20 still exists");
    
    TEST_PASS("delete line");
}

void test_gwlist_list(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwlist_init();
    
    gwlist_add_line(10, "PRINT \"HELLO\"");
    gwlist_add_line(20, "END");
    
    /* Just verify it doesn't crash */
    gwlist_list(0, 0);
    gwlist_list(10, 20);
    
    TEST_PASS("list command");
}

void test_bistrs_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    bistrs_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("bistrs_init");
}

void test_bistrs_left_right_mid(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    bistrs_init();
    
    char *result;
    
    result = bistrs_left("HELLO", 2);
    TEST_ASSERT(result != NULL, "LEFT$ returns string");
    TEST_ASSERT_EQ_STR(result, "HE", "LEFT$ result");
    gwram_free(result);
    
    result = bistrs_right("HELLO", 2);
    TEST_ASSERT(result != NULL, "RIGHT$ returns string");
    TEST_ASSERT_EQ_STR(result, "LO", "RIGHT$ result");
    gwram_free(result);
    
    result = bistrs_mid("HELLO", 2, 2);
    TEST_ASSERT(result != NULL, "MID$ returns string");
    TEST_ASSERT_EQ_STR(result, "EL", "MID$ result");
    gwram_free(result);
    
    TEST_PASS("LEFT$/RIGHT$/MID$");
}

void test_bistrs_len_asc_chr(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    bistrs_init();
    
    TEST_ASSERT_EQ_INT(bistrs_len("HELLO"), 5, "LEN");
    TEST_ASSERT_EQ_INT(bistrs_asc("A"), 65, "ASC");
    
    char *result = bistrs_chr(65);
    TEST_ASSERT(result != NULL, "CHR$ returns string");
    TEST_ASSERT_EQ_STR(result, "A", "CHR$ result");
    gwram_free(result);
    
    TEST_PASS("LEN/ASC/CHR$");
}

void test_bistrs_str_val(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    bistrs_init();
    
    char *result = bistrs_str(123);
    TEST_ASSERT(result != NULL, "STR$ returns string");
    /* STR$ adds leading space for positive numbers */
    TEST_PASS("STR$");
    gwram_free(result);
    
    double val = bistrs_val("123.45");
    TEST_ASSERT(val > 123.4 && val < 123.5, "VAL");
    
    TEST_PASS("STR$/VAL");
}

void test_bistrs_string_space(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    bistrs_init();
    
    char *result = bistrs_string(5, '*');
    TEST_ASSERT(result != NULL, "STRING$ returns string");
    TEST_ASSERT_EQ_STR(result, "*****", "STRING$ result");
    gwram_free(result);
    
    result = bistrs_space(3);
    TEST_ASSERT(result != NULL, "SPACE$ returns string");
    TEST_ASSERT_EQ_STR(result, "   ", "SPACE$ result");
    gwram_free(result);
    
    TEST_PASS("STRING$/SPACE$");
}