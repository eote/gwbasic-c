/* GWINIT - Interpreter Initialization */
/* Based on GWINIT.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <math.h>

void gwinit_init(void) {
    if (!g_state) return;
    
    /* Initialize built-in constants */
    printf("DEBUG: gwinit_init start\n");
    fflush(stdout);
    
    struct variable *pi = gwdata_alloc_variable("PI", VT_SINGLE);
    printf("DEBUG: after gwdata_alloc_variable\n");
    fflush(stdout);
    
    if (pi) {
        pi->value.f = 3.14159265358979323846f;
    }
    printf("DEBUG: gwinit_init end\n");
    fflush(stdout);
    
    /* Additional initialization would go here:
     * - Function pointer tables
     * - Operator precedence tables
     * - Default screen mode
     * - etc.
     */
}