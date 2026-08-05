/* BIBOOT - ASM86 Version Specific Initialization */
/* Based on BIBOOT.ASM */

#include "gwbasic.h"

/* External reference to main entry point */
extern int main(int argc, char *argv[]);

/* BIBOOT entry point - copies DOS control block and jumps to START */
void biboot_init(void) {
    /* In the original ASM, this copies 100H bytes from ES:0 to CS:0
     * and then jumps to START. In our C implementation, the
     * OS loader handles this, so we just call the main entry point. */
    main(0, NULL);
}

/* Last label in data segment - used by BASIC to initialize TXTTAB, etc. */
void *biboot_lstvar(void) {
    /* Return end of data segment marker */
    return NULL;
}