/* Test GWSTS array assignment through statement engine */
#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwsts_dim_then_set(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    gwsts_init(); biptrg_init();

    /* Execute DIM through statement engine */
    int rc = gwsts_exec_statement("DIM A(10)");
    TEST_ASSERT_EQ_INT(rc, 0, "DIM A(10) executes");

    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a != NULL, "A exists after DIM");
    TEST_ASSERT(a->is_array == true, "A is array after DIM");

    /* Execute array assignment through statement engine */
    rc = gwsts_exec_statement("A(1) = 5");
    TEST_ASSERT_EQ_INT(rc, 0, "A(1) = 5 executes");

    float val = biptrg_get_array_element("A", 1);
    TEST_ASSERT(val == 5.0f, "A(1) = 5 stored correctly");

    TEST_PASS("gwsts DIM then set array element");
}