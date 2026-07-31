/* GW-BASIC C Port - BIPRTU/BIPTRG Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_biprtu_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biprtu_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("biprtu_init");
}

void test_biprtu_print_using(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biprtu_init();
    
    /* Test PRINT USING with numeric format */
    biprtu_print_using("##.##", 12.34);
    biprtu_print_using("####", 1234);
    biprtu_print_using("##.##", -5.6);
    
    /* Test PRINT USING with string format */
    biprtu_print_using_str("\\\\  ", "HELLO");
    
    TEST_PASS("print using");
}

void test_biptrg_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biptrg_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("biptrg_init");
}

void test_biptrg_array(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biptrg_init();
    
    /* Test array creation */
    int rc = biptrg_dim_array("A", 10, VT_SINGLE);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM A(10)");
    
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists");
    TEST_ASSERT_EQ_INT(a->type, VT_SINGLE, "A is single array");
    
    /* Test array element access */
    rc = biptrg_set_array_element("A", 5, 42.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set A(5)=42");
    
    float val = biptrg_get_array_element("A", 5);
    TEST_ASSERT(val > 41.9 && val < 42.1, "get A(5)");
    
    TEST_PASS("array operations");
}

void test_biptrg_string_array(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biptrg_init();
    
    /* Test string array */
    int rc = biptrg_dim_array("S$", 5, VT_STRING);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM S$(5)");
    
    rc = biptrg_set_array_string("S$", 0, "HELLO");
    TEST_ASSERT_EQ_INT(rc, 0, "set S$(0)");
    
    char *result = biptrg_get_array_string("S$", 0);
    TEST_ASSERT(result != NULL, "get S$(0)");
    TEST_ASSERT_EQ_STR(result, "HELLO", "S$(0) content");
    
    TEST_PASS("string array");
}