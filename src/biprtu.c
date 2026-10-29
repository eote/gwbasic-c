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

/* ===== v158: PRINT USING value-list driver (ASM BIPRTU PRINUS) =====
 * Scan the format string; non-field characters are output verbatim.
 * Fields:  "!" single-char string, "&" whole string, "\" (2+spaces)
 * fixed-width string, "#...[.###]" numeric (with , and + -),
 * leading "+" applies to the following numeric field.
 * Value list exhausted → keep outputting literal characters only.
 * Format exhausted with values remaining → rescan (ASM REUSIN). */

/* Locate the end of a numeric field starting at p (already on '#');
 * returns pointer past the field, sets *out_flags via chars seen. */
static const char *using_scan_numfield(const char *p, int *int_digits,
                                       int *frac_digits, unsigned char *flags) {
    *int_digits = 0;
    *frac_digits = 0;
    *flags = 0;
    bool in_frac = false;
    while (*p) {
        if (*p == '#') {
            if (in_frac) (*frac_digits)++; else (*int_digits)++;
            p++;
        } else if (*p == '.') {
            if (in_frac) break;  /* second dot ends field */
            in_frac = true;
            p++;
        } else if (*p == ',') {
            /* v158: comma is a digit-grouping char only if another
             * '#' follows; otherwise it terminates the field (the
             * comma is a literal, e.g. "X=##, Y=##.##"). */
            const char *peek = p + 1;
            while (*peek == ' ') peek++;
            if (*peek == '#') {
                *flags |= USING_FLAG_COMMA;
                p++;
            } else {
                break;  /* field ends; comma handled as literal */
            }
        } else if (*p == '+') {
            /* leading or trailing sign */
            *flags |= (in_frac || *int_digits || *frac_digits)
                      ? USING_FLAG_TRAILING_PLUS : USING_FLAG_LEADING_PLUS;
            p++;
        } else if (*p == '-') {
            *flags |= USING_FLAG_TRAILING_MINUS;
            p++;
        } else if (*p == '$') {
            *flags |= USING_FLAG_DOLLAR;
            p++;
        } else if (*p == '*') {
            *flags |= USING_FLAG_ASTERISK;
            p++;
        } else if (*p == '^') {
            *flags |= USING_FLAG_SCIENTIFIC;
            while (*p == '^') p++;
            break;  /* scientific field ends here */
        } else {
            break;  /* any other char ends the field */
        }
    }
    return p;
}

/* Format one numeric value into a numeric field; appends to out.
 * GW-BASIC: value wider than field → prefix '%' and print full value. */
static void using_emit_number(char **out, double val, int int_digits,
                              int frac_digits, unsigned char flags,
                              bool leading_plus) {
    char buf[160];
    bool negative = val < 0;
    double abs_val = negative ? -val : val;

    if (flags & USING_FLAG_SCIENTIFIC) {
        /* ^^^^ — scientific notation with frac_digits mantissa digits */
        const char *sgn = negative ? "-" : (leading_plus ? "+" : "");
        int exp_digits = 2;
        sprintf(buf, "%s%.*fE+%0*d", sgn, frac_digits, abs_val,
                exp_digits, 10);
        strcpy(*out, buf);
        *out += strlen(buf);
        return;
    }

    /* Round to fractional digits */
    double round_factor = 1.0;
    for (int i = 0; i < frac_digits; i++) round_factor *= 10.0;
    abs_val = round(abs_val * round_factor) / round_factor;

    long long int_part = (long long)abs_val;
    double frac_part = abs_val - (double)int_part;

    char int_str[40];
    /* comma grouping */
    if (flags & USING_FLAG_COMMA) {
        char raw[40];
        sprintf(raw, "%lld", int_part);
        int n = (int)strlen(raw);
        int pos = 0;
        for (int i = 0; i < n; i++) {
            int rem = n - i;
            buf[pos++] = raw[i];
            if (rem > 1 && (rem - 1) % 3 == 0 && i > 0 ? 0 : 0) { }
            if ((rem - 1) % 3 == 0 && i > 0) {
                /* place comma when remaining digits form a group boundary */
            }
        }
        /* simpler: rebuild with commas */
        pos = 0;
        for (int i = 0; i < n; i++) {
            buf[pos++] = raw[i];
            int remaining = n - 1 - i;
            if (remaining > 0 && remaining % 3 == 0) buf[pos++] = ',';
        }
        buf[pos] = '\0';
        strcpy(int_str, buf);
    } else {
        sprintf(int_str, "%lld", int_part);
    }

    int actual_int = (int)strlen(int_str);
    char *o = *out;

    if (actual_int > int_digits) {
        /* GW-BASIC: overflow → '%' prefix + the full value (decimal,
         * not scientific — ASM PUFOUT full-precision print) */
        *o++ = '%';
        if (negative) *o++ = '-';
        if (frac_digits > 0) {
            sprintf(o, "%.*f", frac_digits, val);
        } else {
            /* GW-BASIC PUFOUT-style: integer-looking values print
             * without decimals; fractional values print with their
             * significant digits (never scientific for this range) */
            if (val == (double)(long long)val) {
                sprintf(o, "%lld", (long long)val);
            } else {
                sprintf(o, "%g", val);
                /* %g may produce scientific notation for >=1e5 —
                 * expand it back to decimal */
                if (strchr(o, 'e')) {
                    sprintf(o, "%.6f", val);
                    char *tail = o + strlen(o) - 1;
                    while (*tail == '0' && tail > o && tail[-1] != '.') {
                        *tail = '\0';
                        tail--;
                    }
                    if (*tail == '.') *tail = '\0';
                }
            }
        }
        o += strlen(o);
        if (flags & USING_FLAG_TRAILING_MINUS) *o++ = negative ? '-' : ' ';
        else if (flags & USING_FLAG_TRAILING_PLUS) *o++ = negative ? '-' : '+';
        *o = '\0';
        *out = o;
        return;
    }

    /* v158: GW-BASIC pads first, sign hugs the digits (" -45.68") */
    for (int i = actual_int; i < int_digits; i++) *o++ = ' ';
    if (negative) *o++ = '-';
    else if (leading_plus || (flags & USING_FLAG_LEADING_PLUS)) *o++ = '+';
    strcpy(o, int_str);
    o += actual_int;

    if (flags & USING_FLAG_DOLLAR) *o++ = '$';

    if (frac_digits > 0) {
        *o++ = '.';
        for (int i = 0; i < frac_digits; i++) {
            frac_part *= 10.0;
            int digit = (int)(frac_part + 1e-9);
            if (digit > 9) digit = 9;
            *o++ = '0' + digit;
            frac_part -= digit;
        }
    }

    if (flags & USING_FLAG_TRAILING_MINUS) *o++ = negative ? '-' : ' ';
    else if (flags & USING_FLAG_TRAILING_PLUS) *o++ = negative ? '-' : '+';
    *o = '\0';
    *out = o;
}

static void using_emit_string(char **out, const char *s, int width) {
    char *o = *out;
    if (width < 0) {           /* & — whole string */
        strcpy(o, s); o += strlen(s);
    } else if (width == 1) {   /* ! — first char */
        *o++ = s[0];
    } else {                   /* \ field — width chars, space pad */
        int n = (int)strlen(s);
        for (int i = 0; i < width; i++) *o++ = (i < n) ? s[i] : ' ';
    }
    *o = '\0';
    *out = o;
}

void biprtu_print_using_values(const char *fmt, const struct using_value *vals, int count) {
    if (!fmt || !*fmt) {
        /* ASM FCERR3: empty USING string → Illegal function call */
        gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
        return;
    }
    char outbuf[1024];
    char *out = outbuf;
    int vi = 0;               /* next value index */
    bool printed_any = false; /* USFLG */
    const char *p = fmt;

scan_restart:
    while (*p) {
        char ch = *p;
        if (ch == '+' || ch == '-') {
            /* leading sign for a following numeric field */
            bool lead_plus = (ch == '+');
            const char *q = p + 1;
            /* allow "++" runs like ASM PLSFIN */
            while (*q == '+') q++;
            if (*q == '#') {
                if (vi >= count) { p = q; continue; }  /* no value: skip field */
                int id, fd; unsigned char fl;
                q = using_scan_numfield(q, &id, &fd, &fl);
                if (vi < count && !vals[vi].is_str) {
                    using_emit_number(&out, vals[vi].num, id, fd, fl, lead_plus);
                    vi++; printed_any = true;
                } else if (vi < count) {
                    /* string into numeric field — GW-BASIC: mismatch error;
                     * print as-is (lenient) */
                    using_emit_string(&out, vals[vi].str ? vals[vi].str : "", -1);
                    vi++; printed_any = true;
                }
                p = q;
                continue;
            }
            /* sign before a non-field: literal '+'/'-' */
            *out++ = ch; p++;
            continue;
        }
        if (ch == '#') {
            if (vi < count) {
                int id, fd; unsigned char fl;
                p = using_scan_numfield(p, &id, &fd, &fl);
                if (!vals[vi].is_str) {
                    using_emit_number(&out, vals[vi].num, id, fd, fl, false);
                } else {
                    using_emit_string(&out, vals[vi].str ? vals[vi].str : "", -1);
                }
                vi++; printed_any = true;
            } else {
                /* value list exhausted: skip the field silently */
                int id, fd; unsigned char fl;
                p = using_scan_numfield(p, &id, &fd, &fl);
            }
            continue;
        }
        if (ch == '!') {
            if (vi < count) {
                using_emit_string(&out, vals[vi].is_str ? (vals[vi].str ? vals[vi].str : "") : "", 1);
                vi++; printed_any = true;
            }
            p++;
            continue;
        }
        if (ch == '&') {
            if (vi < count) {
                using_emit_string(&out, vals[vi].is_str ? (vals[vi].str ? vals[vi].str : "") : "", -1);
                vi++; printed_any = true;
            }
            p++;
            continue;
        }
        if (ch == '\\') {
            /* fixed-width string field: width = 2 + spaces between */
            int width = 2;
            const char *q = p + 1;
            while (*q == ' ') { width++; q++; }
            if (*q == '\\') {
                if (vi < count) {
                    using_emit_string(&out, vals[vi].is_str ? (vals[vi].str ? vals[vi].str : "") : "", width);
                    vi++; printed_any = true;
                }
                p = q + 1;
            } else {
                /* unterminated: literal backslash */
                *out++ = ch; p++;
            }
            continue;
        }
        /* ordinary character → output verbatim */
        *out++ = ch;
        p++;
    }
    *out = '\0';

    /* ASM REUSIN: values remain but format exhausted → rescan fmt */
    if (vi < count && printed_any) {
        p = fmt;
        goto scan_restart;
    }

    fputs(outbuf, stdout);
}
