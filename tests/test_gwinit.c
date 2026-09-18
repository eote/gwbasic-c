/* GW-BASIC C Port - GWINIT Tests */
/* Based on GWINIT.ASM: INIT 主流程、INITFG、CMDERR、栈计算 */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwinit_basic(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_ASSERT(g_state->running == true, "running flag");
    TEST_ASSERT(g_state->in_direct_mode == true, "direct mode");
    TEST_PASS("gwinit basic init");
}

void test_gwinit_constants(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    /* Test that built-in constants are set */
    struct variable *pi = gwdata_find_variable("PI");
    TEST_ASSERT(pi != NULL, "PI constant exists");
    TEST_ASSERT(pi->type == VT_SINGLE, "PI is single precision");
    TEST_ASSERT(pi->value.f > 3.14 && pi->value.f < 3.15, "PI value correct");
    
    TEST_PASS("built-in constants");
}

void test_gwinit_function_table(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    /* Test that function table is populated */
    /* For now just verify init runs without error */
    TEST_PASS("function table init");
}

/* Test: INITFG 初始化完成标志 (ASM GWINIT.ASM L426-427)
 * KEYSOF: MOV AL,LOW 377O / MOV BYTE PTR INITFG,AL
 * init 完成后 INITFG 非零，错误不再退出 OS */
void test_gwinit_initfg(void) {
    gwdata_init();
    gwram_init();
    
    (void)gwinit_get_initfg();  // Before-value not asserted (init resets it)
    gwinit_init();
    int after = gwinit_get_initfg();
    
    TEST_ASSERT(after != 0, "INITFG nonzero after init (ASM L427)");
    TEST_PASS("gwinit initfg flag");
}

/* Test: INITFG 幂等 — 二次调用保持非零 */
void test_gwinit_initfg_idempotent(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    int first = gwinit_get_initfg();
    gwinit_init();
    int second = gwinit_get_initfg();
    
    TEST_ASSERT(first != 0 && second != 0, "INITFG stays nonzero on re-init");
    TEST_PASS("gwinit initfg idempotent");
}

/* Test: KBUF-1 冒号初始化 (ASM L133-134)
 * MOV BX,OFFSET KBUF-1 / MOV BYTE PTR [BX],LOW ":"
 * DIRECT INPUTS RESTART OK */
void test_gwinit_kbuf_colon(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    const char *kbuf = gwinit_get_kbuf();
    TEST_ASSERT(kbuf != NULL, "KBUF pointer exists");
    TEST_ASSERT(kbuf[0] == ':', "KBUF-1 initialized with colon (ASM L134)");
    TEST_PASS("gwinit kbuf colon");
}

/* Test: 栈大小计算 (ASM L359-364)
 * total_free/8, minimum 512 bytes */
void test_gwinit_stack_calc(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    
    int stack_size = gwinit_get_stack_size();
    TEST_ASSERT(stack_size >= 512, "stack size >= 512 (ASM L362-364 min)");
    TEST_PASS("gwinit stack calc min 512");
}

/* Test: CMDERR 初始化期错误处理 (ASM L432-449)
 * CMDERR: 打印标题 + 命令错误消息 + 退出 OS
 * C 实现: 返回错误码供 main 处理 */
void test_gwinit_cmderr(void) {
    gwdata_init();
    gwram_init();
    
    int rc = gwinit_cmderr("Test error message");
    TEST_ASSERT(rc != 0, "CMDERR returns nonzero error (ASM L449 exit)");
    TEST_PASS("gwinit cmderr");
}

/* Test: gwinit_init 幂等无崩溃 (重复初始化) */
void test_gwinit_reinit_no_crash(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwinit_init();
    gwinit_init();
    
    TEST_ASSERT(g_state != NULL, "state stable after re-init");
    struct variable *pi = gwdata_find_variable("PI");
    TEST_ASSERT(pi != NULL, "PI exists after re-init");
    TEST_PASS("gwinit re-init no crash");
}