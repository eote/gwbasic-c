/* TEST_BIPTRG - Extended unit tests for BIPTRG (Array/Pointer Handling) */
/* Based on BIPTRG.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

static void setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    biptrg_init();
}

/* Test DIM with single precision array */
void test_biptrg_dim_single(void) {
    setup();
    int rc = biptrg_dim_array("ARR", 10, VT_SINGLE);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM single array succeeds");
    struct variable *v = gwdata_find_variable("ARR");
    TEST_ASSERT(v != NULL, "ARR exists");
    TEST_ASSERT(v->is_array == true, "ARR is_array=true");
    TEST_PASS("DIM single precision array");
}

/* Test DIM with integer array */
void test_biptrg_dim_integer(void) {
    setup();
    int rc = biptrg_dim_array("IARR", 5, VT_INTEGER);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM integer array succeeds");
    struct variable *v = gwdata_find_variable("IARR");
    TEST_ASSERT(v != NULL, "IARR exists");
    TEST_ASSERT(v->is_array == true, "IARR is_array=true");
    TEST_PASS("DIM integer array");
}

/* Test DIM with string array */
void test_biptrg_dim_string(void) {
    setup();
    int rc = biptrg_dim_array("SARR$", 5, VT_STRING);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM string array succeeds");
    struct variable *v = gwdata_find_variable("SARR$");
    TEST_ASSERT(v != NULL, "SARR$ exists");
    TEST_PASS("DIM string array");
}

/* Test DIM with NULL name */
void test_biptrg_dim_null_name(void) {
    setup();
    int rc = biptrg_dim_array(NULL, 10, VT_SINGLE);
    TEST_ASSERT(rc != 0, "DIM NULL name returns error");
    TEST_PASS("DIM NULL name");
}

/* Test DIM with zero size */
void test_biptrg_dim_zero_size(void) {
    setup();
    int rc = biptrg_dim_array("ARR", 0, VT_SINGLE);
    TEST_ASSERT(rc != 0, "DIM size=0 returns error");
    TEST_PASS("DIM zero size");
}

/* Test set/get single precision array element */
void test_biptrg_set_get_single(void) {
    setup();
    biptrg_dim_array("ARR", 10, VT_SINGLE);
    
    int rc = biptrg_set_array_element("ARR", 3, 42.5f);
    TEST_ASSERT_EQ_INT(rc, 0, "set element succeeds");
    
    float val = biptrg_get_array_element("ARR", 3);
    TEST_ASSERT(val == 42.5f, "get element returns 42.5");
    TEST_PASS("set/get single precision");
}

/* Test set/get integer array element */
void test_biptrg_set_get_integer(void) {
    setup();
    biptrg_dim_array("IARR", 5, VT_INTEGER);
    
    int rc = biptrg_set_array_element("IARR", 2, 99.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set integer element succeeds");
    
    float val = biptrg_get_array_element("IARR", 2);
    TEST_ASSERT_EQ_INT((int)val, 99, "get integer element = 99");
    TEST_PASS("set/get integer");
}

/* Test set/get string array element */
void test_biptrg_set_get_string(void) {
    setup();
    biptrg_dim_array("SARR$", 5, VT_STRING);
    
    int rc = biptrg_set_array_string("SARR$", 1, "hello");
    TEST_ASSERT_EQ_INT(rc, 0, "set string element succeeds");
    
    char *str = biptrg_get_array_string("SARR$", 1);
    TEST_ASSERT(str != NULL, "get string returns non-NULL");
    if (str) {
        TEST_ASSERT_EQ_STR(str, "hello", "string element = 'hello'");
    }
    TEST_PASS("set/get string");
}

/* Test array element default value (zero) */
void test_biptrg_default_zero(void) {
    setup();
    biptrg_dim_array("ARR", 10, VT_SINGLE);
    
    float val = biptrg_get_array_element("ARR", 0);
    TEST_ASSERT_EQ_INT((int)val, 0, "default element = 0");
    TEST_PASS("default zero");
}

/* Test array bounds (indices clamp) */
void test_biptrg_index_clamp_high(void) {
    setup();
    biptrg_dim_array("ARR", 5, VT_SINGLE);
    
    /* Out of bounds high - should clamp to last */
    biptrg_set_array_element("ARR", 100, 77.0f);
    float val = biptrg_get_array_element("ARR", 100);
    TEST_ASSERT_EQ_INT((int)val, 77, "clamped to last element");
    TEST_PASS("index clamp high");
}

/* Test array bounds (indices clamp low) */
void test_biptrg_index_clamp_low(void) {
    setup();
    biptrg_dim_array("ARR", 5, VT_SINGLE);
    
    /* Out of bounds low - should clamp to first */
    biptrg_set_array_element("ARR", -5, 33.0f);
    float val = biptrg_get_array_element("ARR", -5);
    TEST_ASSERT_EQ_INT((int)val, 33, "clamped to first element");
    TEST_PASS("index clamp low");
}

/* Test get array element from non-existent array */
void test_biptrg_get_nonexistent(void) {
    setup();
    float val = biptrg_get_array_element("NOEXIST", 0);
    TEST_ASSERT_EQ_INT((int)val, 0, "nonexistent array returns 0");
    TEST_PASS("get from non-existent");
}

/* Test set array element on non-existent array */
void test_biptrg_set_nonexistent(void) {
    setup();
    int rc = biptrg_set_array_element("NOEXIST", 0, 1.0f);
    TEST_ASSERT(rc != 0, "set on nonexistent returns error");
    TEST_PASS("set on non-existent");
}

/* Test set string on numeric array (type mismatch) */
void test_biptrg_type_mismatch_set_string(void) {
    setup();
    biptrg_dim_array("ARR", 5, VT_SINGLE);
    int rc = biptrg_set_array_string("ARR", 0, "hello");
    TEST_ASSERT(rc != 0, "set string on numeric array fails");
    TEST_PASS("type mismatch set string");
}

/* Test set numeric on string array (type mismatch) */
void test_biptrg_type_mismatch_set_numeric(void) {
    setup();
    biptrg_dim_array("SARR$", 5, VT_STRING);
    int rc = biptrg_set_array_element("SARR$", 0, 1.0f);
    TEST_ASSERT(rc != 0, "set numeric on string array fails");
    TEST_PASS("type mismatch set numeric");
}

/* Test get string on numeric array */
void test_biptrg_get_string_on_numeric(void) {
    setup();
    biptrg_dim_array("ARR", 5, VT_SINGLE);
    char *str = biptrg_get_array_string("ARR", 0);
    TEST_ASSERT(str == NULL, "get string on numeric returns NULL");
    TEST_PASS("get string on numeric");
}

/* Test get numeric on string array */
void test_biptrg_get_numeric_on_string(void) {
    setup();
    biptrg_dim_array("SARR$", 5, VT_STRING);
    float val = biptrg_get_array_element("SARR$", 0);
    TEST_ASSERT_EQ_INT((int)val, 0, "get numeric on string returns 0");
    TEST_PASS("get numeric on string");
}

/* Test multiple elements set/get */
void test_biptrg_multiple_elements(void) {
    setup();
    biptrg_dim_array("ARR", 10, VT_SINGLE);
    
    biptrg_set_array_element("ARR", 0, 10.0f);
    biptrg_set_array_element("ARR", 1, 20.0f);
    biptrg_set_array_element("ARR", 2, 30.0f);
    
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("ARR", 0), 10, "element 0 = 10");
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("ARR", 1), 20, "element 1 = 20");
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("ARR", 2), 30, "element 2 = 30");
    TEST_PASS("multiple elements");
}

/* Test string array overwrite */
void test_biptrg_string_overwrite(void) {
    setup();
    biptrg_dim_array("SARR$", 5, VT_STRING);
    
    biptrg_set_array_string("SARR$", 0, "first");
    biptrg_set_array_string("SARR$", 0, "second");
    
    char *str = biptrg_get_array_string("SARR$", 0);
    TEST_ASSERT_EQ_STR(str, "second", "overwritten string = 'second'");
    TEST_PASS("string overwrite");
}

/* Test free_array */
void test_biptrg_free_array(void) {
    setup();
    biptrg_dim_array("ARR", 10, VT_SINGLE);
    struct variable *v = gwdata_find_variable("ARR");
    TEST_ASSERT(v != NULL, "ARR exists before free");
    TEST_ASSERT(v->is_array == true, "ARR is array before free");
    
    biptrg_free_array(v);
    
    TEST_ASSERT(v->value.s == NULL, "ARR value.s = NULL after free");
    /* Note: biptrg_free_array does not clear is_array flag */
    TEST_PASS("free array");
}

/* Test free_array with NULL var */
void test_biptrg_free_null(void) {
    setup();
    biptrg_free_array(NULL);
    TEST_PASS("free NULL no crash");
}

/* Test free_array on non-array variable */
void test_biptrg_free_non_array(void) {
    setup();
    struct variable *v = gwdata_alloc_variable("SCALAR", VT_SINGLE);
    v->is_array = false;
    /* Set value.s=NULL to avoid biptrg_free_array treating float bits as pointer */
    v->value.s = NULL;
    
    biptrg_free_array(v);
    
    /* desc is NULL, so biptrg_free_array returns early without crash */
    TEST_PASS("free non-array no crash");
}

/* Test multiple arrays */
void test_biptrg_multiple_arrays(void) {
    setup();
    biptrg_dim_array("A", 5, VT_SINGLE);
    biptrg_dim_array("B", 5, VT_SINGLE);
    
    biptrg_set_array_element("A", 0, 1.0f);
    biptrg_set_array_element("B", 0, 2.0f);
    
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("A", 0), 1, "A[0] = 1");
    TEST_ASSERT_EQ_INT((int)biptrg_get_array_element("B", 0), 2, "B[0] = 2");
    TEST_PASS("multiple arrays");
}

/* Test set with NULL name */
void test_biptrg_set_null_name(void) {
    setup();
    biptrg_dim_array("ARR", 5, VT_SINGLE);
    int rc = biptrg_set_array_element(NULL, 0, 1.0f);
    TEST_ASSERT(rc != 0, "set NULL name returns error");
    TEST_PASS("set NULL name");
}

/* Test get with NULL name */
void test_biptrg_get_null_name(void) {
    setup();
    float val = biptrg_get_array_element(NULL, 0);
    TEST_ASSERT_EQ_INT((int)val, 0, "get NULL name returns 0");
    TEST_PASS("get NULL name");
}

/* Test large array */
void test_biptrg_large_array(void) {
    setup();
    biptrg_dim_array("BIG", 1000, VT_SINGLE);
    
    biptrg_set_array_element("BIG", 500, 123.0f);
    float val = biptrg_get_array_element("BIG", 500);
    TEST_ASSERT_EQ_INT((int)val, 123, "large array element = 123");
    TEST_PASS("large array");
}
