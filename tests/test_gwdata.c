/* GW-BASIC C Port - GWDATA Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwdata_init(void) {
    gwdata_init();
    TEST_ASSERT(g_state != NULL, "g_state allocated");
    TEST_ASSERT(g_state->running == true, "running flag set");
    TEST_ASSERT(g_state->in_direct_mode == true, "direct mode set");
    TEST_ASSERT(g_state->str_heap.used == 0, "string heap empty");
    TEST_ASSERT(g_state->prog_buf.used == 0, "program buffer empty");
    TEST_PASS("gwdata_init basic state");
}

void test_gwdata_variable_table(void) {
    gwdata_init();
    struct variable *v = gwdata_alloc_variable("A", VT_SINGLE);
    TEST_ASSERT(v != NULL, "variable allocation");
    TEST_ASSERT_EQ_STR(v->name, "A", "variable name");
    TEST_ASSERT_EQ_INT(v->type, VT_SINGLE, "variable type");
    TEST_PASS("variable table basic ops");
}

void test_gwdata_string_heap(void) {
    gwdata_init();
    char *s = gwdata_alloc_string(10);
    TEST_ASSERT(s != NULL, "string allocation");
    strcpy(s, "HELLO");
    TEST_ASSERT_EQ_STR(s, "HELLO", "string content");
    gwdata_free_string(s);
    TEST_PASS("string heap basic ops");
}