/* test_gwmain_resetenv.c — v148 MEDIUM-1: RUN must reset the runtime
 * environment (GW-BASIC: RUN clears all variables, closes files, resets
 * FOR/GOSUB stacks and ON ERROR state; only the program text survives).
 * reset_runtime_environment() is exported from gwmain.c for both the RUN
 * command and unit tests. */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "../src/gwbasic.h"

extern void gwsts_reset_runtime_environment(void);
extern int next86_inside_for(void);
extern int next86_get_gosub_depth(void);

/* Test: variables + string heap are cleared */
void test_gwmain_resetenv_variables(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_exec_statement("A = 42");
    gwsts_exec_statement("S$ = \"HELLO\"");
    struct variable *a = gwdata_find_variable("A");
    TEST_ASSERT(a && (int)a->value.f == 42, "A set before reset");
    gwsts_reset_runtime_environment();
    a = gwdata_find_variable("A");
    struct variable *s = gwdata_find_variable("S$");
    TEST_ASSERT(a == NULL, "numeric variable cleared");
    TEST_ASSERT(s == NULL, "string variable cleared");
    /* environment still usable after reset */
    gwsts_exec_statement("B = 7");
    struct variable *b = gwdata_find_variable("B");
    TEST_ASSERT(b && (int)b->value.f == 7, "usable after reset");
    TEST_PASS("resetenv variables");
}

/* Test: FOR stack + GOSUB stack cleared */
void test_gwmain_resetenv_stacks(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    next86_clear_for_stack();
    /* pollute FOR stack via a real FOR statement */
    const char *t1 = "I = 1 TO 5";
    extern int next86_for(const char **text);
    (void)next86_for(&t1);
    TEST_ASSERT(next86_inside_for(), "FOR stack polluted");
    /* pollute GOSUB stack (needs the target line to exist + g_curlin) */
    gwlist_add_line(40, "PRINT \"SUB\"");
    extern WORD g_curlin;
    g_curlin = 10;
    const char *t2 = "40";
    extern int next86_gosub(const char **text);
    int grc = next86_gosub(&t2);
    TEST_ASSERT_EQ_INT(grc, 40, "GOSUB pushes and jumps");
    TEST_ASSERT(next86_get_gosub_depth() > 0, "GOSUB stack polluted");
    gwsts_reset_runtime_environment();
    TEST_ASSERT(!next86_inside_for(), "FOR stack cleared");
    TEST_ASSERT_EQ_INT(next86_get_gosub_depth(), 0, "GOSUB stack cleared");
    TEST_PASS("resetenv stacks");
}

/* Test: ON ERROR state + ERR variable cleared */
void test_gwmain_resetenv_onerror(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("ON ERROR GOTO 40");
    TEST_ASSERT(gwdata_is_on_error_active(), "ON ERROR active before reset");
    /* ERR variable from a handler */
    struct variable *err = gwdata_alloc_variable("ERR", VT_INTEGER);
    if (err) err->value.i = 11;
    gwsts_reset_runtime_environment();
    TEST_ASSERT(!gwdata_is_on_error_active(), "ON ERROR state cleared");
    err = gwdata_find_variable("ERR");
    TEST_ASSERT(err == NULL, "ERR variable cleared");
    TEST_PASS("resetenv onerror");
}
