/* Test multi-dimensional array DIM */
#include "test_framework.h"
#include "../src/gwbasic.h"

static void setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    biptrg_init();
}

void test_biptrg_dim_multi(void) {
    setup();
    int dims[2] = {3, 4};
    int rc = biptrg_dim_array_multi("MARR", dims, 2, VT_SINGLE);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM multi array succeeds");

    struct variable *v = gwdata_find_variable("MARR");
    TEST_ASSERT(v != NULL, "MARR exists");
    TEST_ASSERT(v->is_array == true, "MARR is_array=true");

    TEST_PASS("DIM multi-dimensional array");
}

/* Test multi-dimensional array set/get */
void test_biptrg_multi_set_get(void) {
    setup();
    int dims[2] = {3, 4};
    biptrg_dim_array_multi("MARR", dims, 2, VT_SINGLE);

    /* Total elements = 3 * 4 = 12 */
    int rc = biptrg_set_array_element_multi("MARR", (int[]){1, 2}, 99.0f);
    TEST_ASSERT_EQ_INT(rc, 0, "set multi element succeeds");

    float val = biptrg_get_array_element_multi("MARR", (int[]){1, 2});
    TEST_ASSERT(val == 99.0f, "get multi element = 99");

    TEST_PASS("multi-dimensional set/get");
}

/* Test 3D array */
void test_biptrg_dim_3d(void) {
    setup();
    int dims[3] = {2, 3, 4};
    int rc = biptrg_dim_array_multi("ARR3D", dims, 3, VT_INTEGER);
    TEST_ASSERT_EQ_INT(rc, 0, "DIM 3D array succeeds");

    struct variable *v = gwdata_find_variable("ARR3D");
    TEST_ASSERT(v != NULL, "ARR3D exists");
    TEST_PASS("DIM 3D array");
}