/* GWINIT - Interpreter Initialization */
/* Based on GWINIT.ASM */

#include "gwbasic.h"
#include <math.h>
#include <string.h>

/* INITFG - initialization complete flag (ASM GWINIT.ASM L426-427).
 * When nonzero, errors no longer exit to the OS. */
static int g_initfg = 0;

/* KBUF-1 initialized with a colon (ASM L133-134) so direct inputs
 * restart OK. KBUF holds ":" followed by the direct-mode buffer. */
static char g_kbuf[KBUF_SIZE];

/* Saved stack size computed during init (ASM L359-364). */
static int g_stack_size = 0;

void gwinit_init(void) {
    if (!g_state) return;
    
    /* Initialize built-in constants */
    struct variable *pi = gwdata_alloc_variable("PI", VT_SINGLE);
    if (pi) {
        pi->value.f = 3.14159265358979323846f;
    }
    
    /* Initialize math modules */
    math1_init();
    math2_init();
    
    /* KBUF-1 colon init (ASM L133-134): direct inputs restart OK */
    g_kbuf[0] = ':';
    
    /* Stack size calc (ASM L359-364): total_free/8, minimum 512.
     * C port: string heap is fixed-size, report its eighth with the
     * ASM 512-byte floor. */
    int total_free = (int)STRING_HEAP_SIZE;
    g_stack_size = total_free / 8;
    if (g_stack_size < 512) {
        g_stack_size = 512;
    }
    
    /* INITFG set last (ASM L426-427): initialization complete —
     * errors no longer result in an exit to the OS. */
    g_initfg = -1;  /* ASM LOW 377O = 0xFF */
}

/* INITFG accessor (ASM L426-427) */
int gwinit_get_initfg(void) {
    return g_initfg;
}

/* KBUF accessor (ASM L133-134) — returns KBUF-1 (colon at [0]) */
const char *gwinit_get_kbuf(void) {
    return g_kbuf;
}

/* Computed stack size accessor (ASM L359-364) */
int gwinit_get_stack_size(void) {
    return g_stack_size;
}

/* CMDERR - error before initialization completes (ASM L432-449).
 * ASM: prints heading + command error message, exits to OS.
 * C port: returns ERROR; main() handles reporting and exit. */
int gwinit_cmderr(const char *message) {
    /* Heading + message are printed by the caller (gwmain.c owns
     * console output); here we only translate to the OS exit path. */
    (void)message;
    return ERROR;  /* ASM L449: JMP SYSTME (exit to OS) */
}