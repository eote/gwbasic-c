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

void test_variable_reassignment(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init();
    
    /* Test numeric variable reassignment (v3 CRITICAL #1 regression test) */
    int rc = gwsts_exec_statement("A = 42");
    TEST_ASSERT_EQ_INT(rc, 0, "A = 42 executes");
    
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists after first assignment");
    TEST_ASSERT_EQ_INT((int)a->value.f, 42, "A = 42");
    
    rc = gwsts_exec_statement("A = 99");
    TEST_ASSERT_EQ_INT(rc, 0, "A = 99 executes");
    
    a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists after reassignment");
    TEST_ASSERT_EQ_INT((int)a->value.f, 99, "A = 99 after reassignment");
    TEST_ASSERT(!a->is_array, "A is not array after reassignment");
    
    /* Test string variable reassignment */
    rc = gwsts_exec_statement("S$ = \"hello\"");
    TEST_ASSERT_EQ_INT(rc, 0, "S$ = hello executes");
    
    struct variable *s = gwdata_find_variable("S$");
    TEST_ASSERT(s != NULL, "S$ exists");
    TEST_ASSERT_EQ_STR(s->value.s.ptr, "hello", "S$ = hello");
    
    rc = gwsts_exec_statement("S$ = \"world\"");
    TEST_ASSERT_EQ_INT(rc, 0, "S$ = world executes");
    
    s = gwdata_find_variable("S$");
    TEST_ASSERT(s != NULL, "S$ exists after reassignment");
    TEST_ASSERT_EQ_STR(s->value.s.ptr, "world", "S$ = world after reassignment");
    
    TEST_PASS("variable reassignment");
}

void test_array_bounds(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    biptrg_init();
    
    /* Test array bounds clamp behavior */
    int rc = biptrg_dim_array("A", 10, VT_SINGLE);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM A(10)");
    
    /* Valid indices */
    rc = biptrg_set_array_element("A", 0, 1.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set A(0)");
    rc = biptrg_set_array_element("A", 9, 2.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set A(9)");
    
    float val = biptrg_get_array_element("A", 0);
    TEST_ASSERT_EQ_INT((int)val, 1, "get A(0)");
    val = biptrg_get_array_element("A", 9);
    TEST_ASSERT_EQ_INT((int)val, 2, "get A(9)");
    
    /* Negative index clamped to 0 */
    rc = biptrg_set_array_element("A", -1, 3.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set A(-1) clamped");
    val = biptrg_get_array_element("A", 0);
    TEST_ASSERT_EQ_INT((int)val, 3, "A(-1) -> A(0)");
    
    /* Index >= size clamped to size-1 */
    rc = biptrg_set_array_element("A", 10, 4.0f);  /* 10 is now valid (DIM A(10) creates 0..10) */
    TEST_ASSERT_EQ_INT(rc, 0, "set A(10) - valid index");
    val = biptrg_get_array_element("A", 10);
    TEST_ASSERT_EQ_INT((int)val, 4, "A(10) stored at 10");
    
    /* Index > size clamped to size-1 */
    rc = biptrg_set_array_element("A", 11, 5.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set A(11) clamped");
    val = biptrg_get_array_element("A", 10);
    TEST_ASSERT_EQ_INT((int)val, 5, "A(11) -> A(10)");
    
    TEST_PASS("array bounds clamp");
}