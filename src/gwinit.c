/* GWINIT - Interpreter Initialization */
/* Based on GWINIT.ASM */

#include "gwbasic.h"
#include <math.h>

void gwinit_init(void) {
    if (!g_state) return;
    
    /* Initialize built-in constants */
    struct variable *pi = gwdata_alloc_variable("PI", VT_SINGLE);
    if (pi) {
        pi->value.f = 3.14159265358979323846f;
    }
    
    /* Additional initialization would go here:
     * - Function pointer tables
     * - Operator precedence tables
     * - Default screen mode
     * - etc.
     */
}