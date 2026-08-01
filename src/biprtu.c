/* BIPRTU - PRINT USING Formatting */
/* Based on BIPRTU.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void biprtu_init(void) {
    /* No initialization needed */
}

/* Simple PRINT USING for numeric values */
void biprtu_print_using(const char *fmt, double val) {
    if (!fmt) return;
    
    /* Count # characters for integer part */
    int int_digits = 0;
    int frac_digits = 0;
    bool in_frac = false;
    
    for (const char *p = fmt; *p; p++) {
        if (*p == '#') {
            if (in_frac) frac_digits++;
            else int_digits++;
        } else if (*p == '.') {
            in_frac = true;
        }
    }
    
    /* Build format string for printf */
    char printf_fmt[32];
    if (frac_digits > 0) {
        snprintf(printf_fmt, sizeof(printf_fmt), "%%.%df", frac_digits);
    } else {
        snprintf(printf_fmt, sizeof(printf_fmt), "%%.0f");
    }
    
    printf(printf_fmt, val);
}

/* PRINT USING for string values */
void biprtu_print_using_str(const char *fmt, const char *str) {
    if (!fmt || !str) return;
    
    /* Count \ characters for string field width */
    int width = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p == '\\') width++;
    }
    
    if (width > 0) {
        printf("%-*s", width, str);
    } else {
        printf("%s", str);
    }
}