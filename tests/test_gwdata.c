/* TEST_GWDATA - Unit tests for GWDATA module */
/* Based on GWDATA.ASM - Data structures and runtime state */

#include "test_framework.h"
#include "../src/gwbasic.h"
/* DO NOT include gwdata.c - it's linked separately */

static void setup(void) {
    gwdata_init();
    gwram_init();
}

static void teardown(void) {
    /* gwdata_cleanup and gwram_cleanup don't exist yet */
    (void)0;
}

/* Test: gwdata_init initializes runtime state correctly */
void test_gwdata_init_state(void) {
    setup();
    
    /* Verify global state pointer is allocated */
    TEST_ASSERT(g_state != NULL, "g_state should be allocated");
    TEST_ASSERT(g_state->running == true, "running should be true");
    TEST_ASSERT(g_state->in_direct_mode == true, "in_direct_mode should be true");
    TEST_ASSERT(g_state->current_line == 0, "current_line should be 0");
    TEST_ASSERT(g_state->var_table != NULL, "var_table should be allocated");
    TEST_ASSERT(g_state->str_heap.data != NULL, "string heap should be allocated");
    TEST_ASSERT(g_state->prog_buf.data != NULL, "program buffer should be allocated");
    
    teardown();
    TEST_PASS("gwdata_init initializes state correctly");
}

/* Test: Variable allocation and lookup */
void test_gwdata_variable_alloc_find(void) {
    setup();
    
    /* Allocate a single precision variable */
    struct variable *v1 = gwdata_alloc_variable("A", VT_SINGLE);
    TEST_ASSERT(v1 != NULL, "variable allocation should succeed");
    TEST_ASSERT_EQ_STR(v1->name, "A", "variable name should be 'A'");
    TEST_ASSERT_EQ_INT(v1->type, VT_SINGLE, "variable type should be VT_SINGLE");
    TEST_ASSERT(v1->value.f == 0.0f, "default value should be 0");
    
    /* Find the same variable */
    struct variable *v1_found = gwdata_find_variable("A");
    TEST_ASSERT(v1_found == v1, "find_variable should return same pointer");
    
    /* Allocate integer variable */
    struct variable *v2 = gwdata_alloc_variable("I%", VT_INTEGER);
    TEST_ASSERT(v2 != NULL, "integer variable allocation should succeed");
    TEST_ASSERT_EQ_STR(v2->name, "I%", "variable name should be 'I%'");
    TEST_ASSERT_EQ_INT(v2->type, VT_INTEGER, "variable type should be VT_INTEGER");
    
    /* Allocate string variable */
    struct variable *v3 = gwdata_alloc_variable("S$", VT_STRING);
    TEST_ASSERT(v3 != NULL, "string variable allocation should succeed");
    TEST_ASSERT_EQ_STR(v3->name, "S$", "variable name should be 'S$'");
    TEST_ASSERT_EQ_INT(v3->type, VT_STRING, "variable type should be VT_STRING");
    TEST_ASSERT(v3->value.s.ptr == NULL, "string value should be NULL initially");
    
    /* Find non-existent variable */
    struct variable *v_notfound = gwdata_find_variable("X");
    TEST_ASSERT(v_notfound == NULL, "find_variable should return NULL for non-existent");
    
    teardown();
    TEST_PASS("variable alloc/find works correctly");
}

/* Test: Variable type handling */
void test_gwdata_variable_type_handling(void) {
    setup();
    
    struct variable *v = gwdata_alloc_variable("X", VT_SINGLE);
    TEST_ASSERT(v != NULL, "alloc should succeed");
    
    /* Assign single precision value */
    v->value.f = 3.14f;
    TEST_ASSERT(v->value.f == 3.14f, "single precision value should be stored");
    
    /* Test integer variable */
    struct variable *vi = gwdata_alloc_variable("I%", VT_INTEGER);
    vi->value.i = 42;
    TEST_ASSERT_EQ_INT(vi->value.i, 42, "integer value should be stored");
    
    /* Test string variable */
    struct variable *vs = gwdata_alloc_variable("S$", VT_STRING);
    TEST_ASSERT(vs->type == VT_STRING, "type should be VT_STRING");
    TEST_ASSERT(vs->value.s.ptr == NULL, "string should be NULL initially");
    
    teardown();
    TEST_PASS("variable type handling works");
}

/* Test: String heap allocation */
void test_gwdata_string_heap_impl(void) {
    setup();
    
    /* Allocate string */
    char *s1 = gwdata_alloc_string(10);
    TEST_ASSERT(s1 != NULL, "string allocation should succeed");
    strcpy(s1, "HELLO");
    TEST_ASSERT_EQ_STR(s1, "HELLO", "string content should match");
    
    /* Allocate another */
    char *s2 = gwdata_alloc_string(20);
    TEST_ASSERT(s2 != NULL, "second allocation should succeed");
    strcpy(s2, "WORLD");
    
    /* Free first (no-op in current impl) */
    gwdata_free_string(s1);
    
    /* Allocate again */
    char *s3 = gwdata_alloc_string(5);
    TEST_ASSERT(s3 != NULL, "third allocation should succeed");
    
    teardown();
    TEST_PASS("string heap alloc works");
}

/* Compatibility wrapper for test_runner.c */
void test_gwdata_string_heap(void) {
    test_gwdata_string_heap_impl();
}

/* Test: Program buffer operations (basic) */
void test_gwdata_program_buffer(void) {
    setup();
    
    /* Current implementation doesn't have program buffer API yet */
    /* Just verify buffers are initialized */
    TEST_ASSERT(g_state->prog_buf.data != NULL, "program buffer should be allocated");
    TEST_ASSERT(g_state->prog_buf.used == 0, "program buffer should be empty initially");
    
    teardown();
    TEST_PASS("program buffer initialized correctly");
}

/* Test: Error handling basics */
void test_gwdata_error_handling(void) {
    setup();
    
    /* Set error */
    gwdata_set_error(11, 100, 42);
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 11, "error code should be 11");
    TEST_ASSERT_EQ_INT(gwdata_get_error_line(), 100, "error line should be 100");
    TEST_ASSERT_EQ_INT(gwdata_get_error_text_ptr(), 42, "error text ptr should be 42");
    
    /* Clear error */
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "error code should be cleared");
    
    /* Test ON ERROR active flag */
    TEST_ASSERT(!gwdata_is_on_error_active(), "ON ERROR should not be active initially");
    gwdata_set_on_error_goto(10, 42);
    TEST_ASSERT(gwdata_is_on_error_active(), "ON ERROR should be active after set");
    TEST_ASSERT_EQ_INT(gwdata_get_on_error_line(), 10, "handler line should be 10");
    
    gwdata_clear_on_error();
    TEST_ASSERT(!gwdata_is_on_error_active(), "ON ERROR should be cleared");
    
    teardown();
    TEST_PASS("error handling basics work");
}

/* Test: DEF FN support */
void test_gwdata_fn_def(void) {
    setup();
    
    /* Allocate a FN variable */
    struct variable *fn = gwdata_alloc_variable("FNA", VT_SINGLE);
    TEST_ASSERT(fn != NULL, "FN variable allocation should succeed");
    
    /* Mark as FN def */
    fn->is_fn_def = true;
    fn->fn_param_name[0] = 'X';
    fn->fn_param_name[1] = '\0';
    fn->fn_text_ptr = 1234;
    fn->fn_param_type = VT_SINGLE;
    
    /* Find it */
    struct variable *found = gwdata_find_fn_def("FNA");
    TEST_ASSERT(found == fn, "find_fn_def should find the FN definition");
    TEST_ASSERT(found->is_fn_def, "is_fn_def should be true");
    TEST_ASSERT_EQ_INT(found->fn_text_ptr, 1234, "fn_text_ptr should match");
    
    /* Non-existent FN */
    struct variable *notfound = gwdata_find_fn_def("FNB");
    TEST_ASSERT(notfound == NULL, "find_fn_def should return NULL for non-existent");
    
    teardown();
    TEST_PASS("DEF FN support works");
}

/* NO main() function - test_runner.c provides the main entry point */

/* Compatibility wrappers for test_runner.c expected test names */
void test_gwdata_init(void) {
    test_gwdata_init_state();
}

void test_gwdata_variable_table(void) {
    test_gwdata_variable_alloc_find();
}

void test_gwdata_string_heap_wrapper(void) {
    test_gwdata_string_heap();
}