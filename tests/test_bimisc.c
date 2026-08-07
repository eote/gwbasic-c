/* TEST_BIMISC - Unit tests for BIMISC (Miscellaneous Built-in Functions) */
/* Based on BIMISC.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

static void setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    bimisc_init();
}

/* Test init */
void test_bimisc_init(void) {
    setup();
    TEST_ASSERT(bimisc_get_trace_flag() == 0, "trace flag is 0 after init");
    TEST_PASS("bimisc_init");
}

/* Test TRON */
void test_bimisc_tron(void) {
    setup();
    bimisc_tron();
    TEST_ASSERT(bimisc_get_trace_flag() == 1, "trace flag is 1 after TRON");
    TEST_PASS("TRON");
}

/* Test TROFF */
void test_bimisc_troff(void) {
    setup();
    bimisc_tron();
    bimisc_troff();
    TEST_ASSERT(bimisc_get_trace_flag() == 0, "trace flag is 0 after TROFF");
    TEST_PASS("TROFF");
}

/* Test is_letter for uppercase */
void test_bimisc_is_letter_upper(void) {
    setup();
    TEST_ASSERT(bimisc_is_letter('A') == 1, "'A' is a letter");
    TEST_ASSERT(bimisc_is_letter('Z') == 1, "'Z' is a letter");
    TEST_PASS("is_letter uppercase");
}

/* Test is_letter for lowercase */
void test_bimisc_is_letter_lower(void) {
    setup();
    TEST_ASSERT(bimisc_is_letter('a') == 1, "'a' is a letter");
    TEST_ASSERT(bimisc_is_letter('z') == 1, "'z' is a letter");
    TEST_PASS("is_letter lowercase");
}

/* Test is_letter for non-letters */
void test_bimisc_is_letter_nonletter(void) {
    setup();
    TEST_ASSERT(bimisc_is_letter('0') == 0, "'0' is not a letter");
    TEST_ASSERT(bimisc_is_letter(' ') == 0, "' ' is not a letter");
    TEST_ASSERT(bimisc_is_letter('@') == 0, "'@' is not a letter");
    TEST_PASS("is_letter non-letters");
}

/* Test STOP */
void test_bimisc_stop(void) {
    setup();
    g_state->running = true;
    bimisc_stop();
    TEST_ASSERT(g_state->running == false, "running=false after STOP");
    TEST_ASSERT(g_state->exit_code == 0, "exit_code=SUCCESS after STOP");
    TEST_PASS("STOP");
}

/* Test END */
void test_bimisc_end(void) {
    setup();
    g_state->running = true;
    bimisc_end();
    TEST_ASSERT(g_state->running == false, "running=false after END");
    TEST_ASSERT(g_state->exit_code == 0, "exit_code=SUCCESS after END");
    TEST_PASS("END");
}

/* Test CONT */
void test_bimisc_cont(void) {
    setup();
    g_state->running = false;
    bimisc_cont();
    TEST_ASSERT(g_state->running == true, "running=true after CONT");
    TEST_PASS("CONT");
}

/* Test SWAP with numeric variables */
void test_bimisc_swap_numeric(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    struct variable *b = gwdata_alloc_variable("B", VT_SINGLE);
    a->value.f = 10.0f;
    b->value.f = 20.0f;
    
    bimisc_swap("A", "B");
    TEST_ASSERT_EQ_INT((int)a->value.f, 20, "A = 20 after swap");
    TEST_ASSERT_EQ_INT((int)b->value.f, 10, "B = 10 after swap");
    TEST_PASS("SWAP numeric");
}

/* Test SWAP with integer variables */
void test_bimisc_swap_integer(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_INTEGER);
    struct variable *b = gwdata_alloc_variable("B", VT_INTEGER);
    a->value.i = 5;
    b->value.i = 15;
    
    bimisc_swap("A", "B");
    TEST_ASSERT_EQ_INT((int)a->value.i, 15, "A = 15 after swap");
    TEST_ASSERT_EQ_INT((int)b->value.i, 5, "B = 5 after swap");
    TEST_PASS("SWAP integer");
}

/* Test SWAP with mismatched types (should not swap) */
void test_bimisc_swap_type_mismatch(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    struct variable *b = gwdata_alloc_variable("B", VT_INTEGER);
    a->value.f = 10.0f;
    b->value.i = 20;
    
    bimisc_swap("A", "B");
    TEST_ASSERT_EQ_INT((int)a->value.f, 10, "A unchanged (type mismatch)");
    TEST_ASSERT_EQ_INT((int)b->value.i, 20, "B unchanged (type mismatch)");
    TEST_PASS("SWAP type mismatch");
}

/* Test SWAP with NULL vars */
void test_bimisc_swap_null(void) {
    setup();
    bimisc_swap(NULL, "B");
    TEST_PASS("SWAP NULL no crash");
}

/* Test SWAP with non-existent vars */
void test_bimisc_swap_nonexistent(void) {
    setup();
    bimisc_swap("X", "Y");
    TEST_PASS("SWAP nonexistent no crash");
}

/* Test is_file_io */
void test_bimisc_is_file_io(void) {
    setup();
    int result = bimisc_is_file_io();
    TEST_ASSERT_EQ_INT(result, 0, "is_file_io returns 0");
    TEST_PASS("is_file_io");
}

/* Test restore without line number */
void test_bimisc_restore_no_line(void) {
    setup();
    g_state->prog_buf.used = 100;
    bimisc_restore(0);
    TEST_ASSERT_EQ_INT(g_state->prog_buf.used, 0, "prog_buf.used=0 after RESTORE 0");
    TEST_PASS("RESTORE no line");
}

/* Test NEW command */
void test_bimisc_new(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    a->value.f = 42.0f;
    g_state->prog_buf.used = 100;
    
    bimisc_new();
    
    TEST_ASSERT_EQ_INT(g_state->current_line, 0, "current_line=0 after NEW");
    TEST_ASSERT(g_state->in_direct_mode == true, "in_direct_mode=true after NEW");
    TEST_ASSERT_EQ_INT(g_state->prog_buf.used, 0, "prog_buf.used=0 after NEW");
    TEST_PASS("NEW command");
}

/* Test CLEAR command */
void test_bimisc_clear(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    a->value.f = 42.0f;
    g_state->str_heap.used = 100;
    
    bimisc_clear();
    
    TEST_ASSERT_EQ_INT(g_state->str_heap.used, 0, "str_heap cleared after CLEAR");
    TEST_PASS("CLEAR command");
}

/* Test CLEAR with args */
void test_bimisc_clear_with_args(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    a->value.f = 42.0f;
    
    bimisc_clear_with_args(256, 65536);
    
    TEST_ASSERT_EQ_INT(g_state->str_heap.used, 0, "str_heap cleared after CLEAR with args");
    TEST_PASS("CLEAR with args");
}

/* Test run_setup */
void test_bimisc_run_setup(void) {
    setup();
    struct variable *a = gwdata_alloc_variable("A", VT_SINGLE);
    a->value.f = 42.0f;
    g_state->str_heap.used = 100;
    g_state->exit_code = 5;
    
    bimisc_run_setup();
    
    TEST_ASSERT_EQ_INT(g_state->current_line, 0, "current_line=0 after run_setup");
    TEST_ASSERT_EQ_INT(g_state->exit_code, 0, "exit_code=0 after run_setup");
    TEST_ASSERT_EQ_INT(g_state->str_heap.used, 0, "str_heap cleared after run_setup");
    TEST_PASS("run_setup");
}

/* Test ERASE */
void test_bimisc_erase(void) {
    setup();
    biptrg_dim_array("ARR", 10, VT_SINGLE);
    struct variable *v = gwdata_find_variable("ARR");
    TEST_ASSERT(v != NULL, "ARR exists");
    if (v) {
        TEST_ASSERT(v->is_array == true, "ARR is array");
    }
    
    bimisc_erase("ARR");
    
    v = gwdata_find_variable("ARR");
    if (v) {
        TEST_ASSERT(v->is_array == false, "ARR is not array after ERASE");
    }
    TEST_PASS("ERASE");
}

/* Test ERASE non-existent */
void test_bimisc_erase_nonexistent(void) {
    setup();
    bimisc_erase("NONEXIST");
    TEST_PASS("ERASE nonexistent no crash");
}

/* Test ERASE NULL */
void test_bimisc_erase_null(void) {
    setup();
    bimisc_erase(NULL);
    TEST_PASS("ERASE NULL no crash");
}

/* Test control_char (should not crash) */
void test_bimisc_control_char(void) {
    setup();
    bimisc_control_char(3);  /* ^C */
    TEST_PASS("control_char no crash");
}

/* Test get_stack (stub, should not crash) */
void test_bimisc_get_stack(void) {
    setup();
    bimisc_get_stack(8);
    TEST_PASS("get_stack no crash");
}

/* Test stack_init */
void test_bimisc_stack_init(void) {
    setup();
    g_state->current_line = 100;
    
    bimisc_stack_init();
    
    TEST_ASSERT_EQ_INT(g_state->current_line, 0, "current_line=0 after stack_init");
    TEST_ASSERT(g_state->in_direct_mode == true, "in_direct_mode=true after stack_init");
    TEST_PASS("stack_init");
}

/* Test trap_on/trap_off */
void test_bimisc_trap_on_off(void) {
    setup();
    unsigned char flag = 0;
    
    bimisc_trap_on(&flag);
    TEST_ASSERT_EQ_INT(flag & 1, 1, "trap ON bit set");
    
    bimisc_trap_off(&flag);
    TEST_ASSERT_EQ_INT(flag, 0, "trap flag cleared after off");
    TEST_PASS("trap on/off");
}

/* Test trap_stop */
void test_bimisc_trap_stop(void) {
    setup();
    unsigned char flag = 1;  /* ON */
    
    bimisc_trap_stop(&flag);
    TEST_ASSERT(flag & 2, "STOP bit set");
    TEST_PASS("trap stop");
}

/* Test set_trap_address */
void test_bimisc_set_trap_address(void) {
    setup();
    bimisc_set_trap_address(0, 1000);
    TEST_PASS("set_trap_address no crash");
}

/* Test check_traps with no traps */
void test_bimisc_check_traps_empty(void) {
    setup();
    int result = bimisc_check_traps();
    TEST_ASSERT_EQ_INT(result, 0, "check_traps returns 0 with no traps");
    TEST_PASS("check_traps empty");
}
