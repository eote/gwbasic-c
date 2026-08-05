/* KANJ86 - Kanji String Function Support for Basic-86 */
/* Based on KANJ86.ASM */

#include "gwbasic.h"

/* External references */
extern void gweval_init(void);
extern void gweval_synchr(const char **text, char expected);
extern void gweval_scan_var(const char **text, int *var_ptr);

/* Module initialization */
void kanj86_init(void) {
    /* Initialize Kanji support module */
    /* For now, all Kanji functions will return syntax error as in original */
}

/* KTN - Kanji True Name function */
/* Returns syntax error - not implemented in this version */
int kanj86_ktnfn(const char **text) {
    (void)text;
    /* In original GW-BASIC, this would call SNERR */
    return -1;  /* Syntax error */
}

/* JIS - Japanese Industrial Standard function */
/* Returns syntax error - not implemented in this version */
int kanj86_jisfn(const char **text) {
    (void)text;
    /* In original GW-BASIC, this would call SNERR */
    return -1;  /* Syntax error */
}

/* KLEN - Kanji Length function */
/* Returns syntax error - not implemented in this version */
int kanj86_klenfn(const char **text) {
    (void)text;
    /* In original GW-BASIC, this would call SNERR */
    return -1;  /* Syntax error */
}

/* KPOS - Kanji Position function */
/* Returns syntax error - not implemented in this version */
int kanj86_kposfn(const char **text) {
    (void)text;
    /* In original GW-BASIC, this would call SNERR */
    return -1;  /* Syntax error */
}

/* Module termination */
void kanj86_term(void) {
    /* Nothing to clean up */
}