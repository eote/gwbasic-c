/* BIPRTU - PRINT USING Formatting */
/* Based on BIPRTU.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

void biprtu_init(void) {
    /* No initialization needed */
}

/* Format flags for USING numeric fields */
#define USING_FLAG_LEADING_PLUS   0x01  /* + before number */
#define USING_FLAG_TRAILING_MINUS 0x02  /* - after number */
#define USING_FLAG_TRAILING_PLUS  0x04  /* + after number */
#define USING_FLAG_SCIENTIFIC     0x08  /* ^^^^ scientific notation */
#define USING_FLAG_COMMA          0x10  /* , every 3 digits */
#define USING_FLAG_DOLLAR         0x20  /* $ floating dollar sign */
#define USING_FLAG_ASTERISK       0x40  /* * fill */

/* Count digits and parse format string */
static int parse_using_format(const char *fmt, int *int_digits, int *frac_digits, unsigned char *flags) {
    if (!fmt || !int_digits || !frac_digits || !flags) return -1;
    
    *int_digits = 0;
    *frac_digits = 0;
    *flags = 0;
    
    const char *p = fmt;
    bool in_frac = false;
    bool seen_digit = false;
    (void)seen_digit;
    int temp = 0;
    (void)temp;
    
    while (*p) {
        if (*p == '#') {
            if (in_frac) (*frac_digits)++;
            else (*int_digits)++;
            seen_digit = true;
        } else if (*p == '.') {
            in_frac = true;
        } else if (*p == '+') {
            *flags |= USING_FLAG_LEADING_PLUS;
        } else if (*p == '-') {
            *flags |= USING_FLAG_TRAILING_MINUS;
        } else if (*p == '^') {
            /* Check for ^^^^ */
            if (p[1] == '^' && p[2] == '^' && p[3] == '^') {
                *flags |= USING_FLAG_SCIENTIFIC;
                p += 3;
            }
        } else if (*p == ',') {
            *flags |= USING_FLAG_COMMA;
        } else if (*p == '$') {
            *flags |= USING_FLAG_DOLLAR;
        } else if (*p == '*') {
            *flags |= USING_FLAG_ASTERISK;
        }
        p++;
    }
    
    return 0;
}

/* Format number with comma separators */
static void format_with_commas(char *buf, const char *num_str) {
    char temp[64];
    (void)temp;
    int len = strlen(num_str);
    int comma_pos = len % 3;
    if (comma_pos == 0) comma_pos = 3;
    
    int out_pos = 0;
    for (int i = 0; i < len; i++) {
        if (i > 0 && i % 3 == comma_pos) {
            buf[out_pos++] = ',';
        }
        buf[out_pos++] = num_str[i];
    }
    buf[out_pos] = '\0';
}

/* Format number for USING */
static void format_using_number(char *output, double val, int int_digits, int frac_digits, unsigned char flags) {
    /* Handle sign */
    bool negative = val < 0;
    double abs_val = negative ? -val : val;
    
    /* Round to fractional digits */
    double round_factor = 1.0;
    for (int i = 0; i < frac_digits; i++) round_factor *= 10.0;
    abs_val = round(abs_val * round_factor) / round_factor;
    
    /* Format integer part */
    long long int_part = (long long)abs_val;
    double frac_part = abs_val - int_part;
    
    char int_str[32];
    sprintf(int_str, "%lld", int_part);
    int actual_int_digits = strlen(int_str);
    
    /* Check if integer part exceeds format width */
    if (actual_int_digits > int_digits) {
        /* Overflow - fill with * per GW-BASIC spec */
        int total_chars = int_digits + frac_digits + (frac_digits > 0 ? 1 : 0);
        memset(output, '*', total_chars);
        output[total_chars] = '\0';
        return;
    }
    
    /* Build output */
    char *out = output;
    
    /* Handle sign */
    if (flags & USING_FLAG_LEADING_PLUS) {
        *out++ = negative ? '-' : '+';
    } else if (negative) {
        *out++ = '-';
    }
    
    /* Add integer part (no leading zeros - print as-is) */
    strcpy(out, int_str);
    out += actual_int_digits;
    
    /* Add fractional part */
    if (frac_digits > 0) {
        *out++ = '.';
        for (int i = 0; i < frac_digits; i++) {
            frac_part *= 10.0;
            int digit = (int)frac_part;
            *out++ = '0' + digit;
            frac_part -= digit;
        }
    }
    
    /* Handle trailing sign */
    if (flags & (USING_FLAG_TRAILING_MINUS | USING_FLAG_TRAILING_PLUS)) {
        if (negative) {
            *out++ = '-';
        } else {
            *out++ = '+';
        }
    }
    
    *out = '\0';
    
    /* Apply comma formatting if needed */
    if (flags & USING_FLAG_COMMA) {
        char temp[128];
        format_with_commas(temp, output);
        strcpy(output, temp);
    }
}
/* Simple PRINT USING for numeric values */
void biprtu_print_using(const char *fmt, double val) {
    if (!fmt) return;
    
    int int_digits = 0;
    int frac_digits = 0;
    unsigned char flags = 0;
    
    parse_using_format(fmt, &int_digits, &frac_digits, &flags);
    
    char output[128];
    format_using_number(output, val, int_digits, frac_digits, flags);
    
    printf("%s", output);
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