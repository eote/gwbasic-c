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
#define USING_FLAG_LEADING_PLUS   0x01
#define USING_FLAG_TRAILING_MINUS 0x02
#define USING_FLAG_TRAILING_PLUS  0x04
#define USING_FLAG_SCIENTIFIC     0x08
#define USING_FLAG_COMMA          0x10
#define USING_FLAG_ASTERISK       0x40

/* ASM BIPRTU.ASM 292-300: leading+trailing digits > 24 → FCERR */
#define USING_MAX_FIELD_DIGITS 24

/* ===== v159: growable output (CRITICAL-1) =====
 * Chunks flush to stdout as they fill; no total-length limit. */
#define USING_OUT_CHUNK 4096
static char *g_out_chunk = NULL;
static size_t g_out_len = 0;

static void using_out_reset(void) {
    if (!g_out_chunk) g_out_chunk = (char *)malloc(USING_OUT_CHUNK);
    g_out_len = 0;
}

static void using_out_flush(void) {
    if (g_out_len > 0) {
        fwrite(g_out_chunk, 1, g_out_len, stdout);
        g_out_len = 0;
    }
}

static void using_out_char(char ch) {
    if (!g_out_chunk) using_out_reset();
    if (g_out_len >= USING_OUT_CHUNK - 1) using_out_flush();
    g_out_chunk[g_out_len++] = ch;
}

static void using_out_str(const char *s, size_t n) {
    if (!g_out_chunk) using_out_reset();
    while (n > 0) {
        if (g_out_len >= USING_OUT_CHUNK - 1) using_out_flush();
        size_t space = USING_OUT_CHUNK - 1 - g_out_len;
        size_t take = (n < space) ? n : space;
        memcpy(g_out_chunk + g_out_len, s, take);
        g_out_len += take;
        s += take;
        n -= take;
    }
}

/* Locate the end of a numeric field starting at p (already on '#');
 * returns pointer past the field. v159 MEDIUM-8/CRITICAL-2: enforces
 * the ASM 24-digit limit via *too_long. */
static const char *using_scan_numfield(const char *p, int *int_digits,
                                       int *frac_digits, unsigned char *flags,
                                       bool *too_long) {
    *int_digits = 0;
    *frac_digits = 0;
    *flags = 0;
    *too_long = false;
    bool in_frac = false;
    while (*p) {
        if (*p == '#') {
            if (in_frac) (*frac_digits)++; else (*int_digits)++;
            if (*int_digits + *frac_digits > USING_MAX_FIELD_DIGITS) {
                *too_long = true;
                return p;
            }
            p++;
        } else if (*p == '.') {
            if (in_frac) break;
            in_frac = true;
            p++;
        } else if (*p == ',') {
            /* comma groups digits only if another '#' follows;
             * otherwise it ends the field (literal comma) */
            const char *peek = p + 1;
            while (*peek == ' ') peek++;
            if (*peek == '#') {
                *flags |= USING_FLAG_COMMA;
                p++;
            } else {
                break;
            }
        } else if (*p == '+') {
            *flags |= (in_frac || *int_digits || *frac_digits)
                      ? USING_FLAG_TRAILING_PLUS : USING_FLAG_LEADING_PLUS;
            p++;
        } else if (*p == '-') {
            *flags |= USING_FLAG_TRAILING_MINUS;
            p++;
        } else if (*p == '^') {
            *flags |= USING_FLAG_SCIENTIFIC;
            while (*p == '^') p++;
            break;
        } else {
            break;
        }
    }
    return p;
}

/* Emit one numeric value into a numeric field.
 * v159: bounded — field digits <= 24 (scanner-enforced), so every
 * intermediate fits a fixed 64-byte scratch; output goes through the
 * growable chunk writer (no unbounded sprintf into fixed buffers).
 * value_too_big → double outside +/-1e37? GW-BASIC overflow; we
 * format with the %g fallback only inside safe bounds. */
static void using_emit_number(double val, int int_digits, int frac_digits,
                              unsigned char flags, bool leading_plus,
                              bool star_fill, bool dollar_float) {
    bool negative = (val < 0.0);

    if (flags & USING_FLAG_SCIENTIFIC) {
        /* v159 HIGH-1: ASM $PUFE — normalized mantissa + exponent.
         * Format: d.dddE+ee (mantissa width = int_digits + frac_digits,
         * exponent 2 digits, sign always). */
        if (val == 0.0) {
            /* GW-BASIC prints E+00 with zero mantissa as digits */
            char m[64];
            int frac = frac_digits > 0 ? frac_digits : 0;
            sprintf(m, "0%s", frac > 0 ? "." : "");
            for (int i = 0; i < frac; i++) strcat(m, "0");
            using_out_str(m, strlen(m));
            using_out_str("E+00", 4);
            return;
        }
        int exp10 = (int)floor(log10(fabs(val)));
        double mant = val / pow(10.0, exp10);
        /* normalize mantissa into [1,10) */
        if (fabs(mant) >= 10.0) { mant /= 10.0; exp10++; }
        if (fabs(mant) < 1.0)  { mant *= 10.0; exp10--; }
        /* round mantissa to frac_digits — rounding may bump to 10 */
        double rf = 1.0;
        for (int i = 0; i < frac_digits; i++) rf *= 10.0;
        mant = round(mant * rf) / rf;
        if (fabs(mant) >= 10.0) { mant /= 10.0; exp10++; }
        int m_int = (int)fabs(mant);
        double m_frac = fabs(mant) - m_int;
        if (negative) using_out_char('-');
        else if (leading_plus || (flags & USING_FLAG_LEADING_PLUS)) using_out_char('+');
        char tmp[64];
        sprintf(tmp, "%d", m_int);
        using_out_str(tmp, strlen(tmp));
        if (frac_digits > 0) {
            using_out_char('.');
            for (int i = 0; i < frac_digits; i++) {
                m_frac *= 10.0;
                int d = (int)(m_frac + 1e-9);
                if (d > 9) d = 9;
                using_out_char((char)('0' + d));
                m_frac -= d;
            }
        }
        using_out_char('E');
        using_out_char(exp10 < 0 ? '-' : '+');
        int ae = exp10 < 0 ? -exp10 : exp10;
        sprintf(tmp, "%02d", ae);
        using_out_str(tmp, strlen(tmp));
        return;
    }

    /* --- fixed-point path --- */
    /* v159 CRITICAL-2: range-check the double→integer conversion.
     * |val| above 1e18 cannot be represented in a 24-digit field
     * anyway — route through the overflow (%) path with %g avoided. */
    if (fabs(val) >= 1e18 || val != val) {  /* NaN too */
        /* print full decimal via bounded manual conversion */
        using_out_char('%');
        if (negative) using_out_char('-');
        /* scientific-free expansion via %f with generous precision,
         * chunked out directly */
        char bigbuf[512];
        snprintf(bigbuf, sizeof(bigbuf), "%.0f", fabs(val));
        using_out_str(bigbuf, strlen(bigbuf));
        if (flags & USING_FLAG_TRAILING_MINUS) using_out_char(negative ? '-' : ' ');
        else if (flags & USING_FLAG_TRAILING_PLUS) using_out_char(negative ? '-' : '+');
        return;
    }

    double round_factor = 1.0;
    for (int i = 0; i < frac_digits; i++) round_factor *= 10.0;
    double abs_val = round(fabs(val) * round_factor) / round_factor;

    long long int_part = (long long)abs_val;
    double frac_part = abs_val - (double)int_part;

    /* integer digits with optional comma grouping */
    char int_str[48];
    {
        char raw[48];
        snprintf(raw, sizeof(raw), "%lld", int_part);
        if (flags & USING_FLAG_COMMA) {
            int n = (int)strlen(raw);
            int pos = 0;
            for (int i = 0; i < n; i++) {
                int_str[pos++] = raw[i];
                int remaining = n - 1 - i;
                if (remaining > 0 && remaining % 3 == 0) int_str[pos++] = ',';
            }
            int_str[pos] = '\0';
        } else {
            strcpy(int_str, raw);
        }
    }
    size_t numlen = strlen(int_str);
    int actual_int = (int)numlen;

    /* field width accounting: comma columns count toward the width
     * (v159 MEDIUM-4: ASM NUMNUM INC DL for each comma) */
    int commas_in_field = 0;
    if (flags & USING_FLAG_COMMA) {
        /* commas the format will place = (digits-1)/3 rounded down
         * positions present in the output when int_part fills the
         * field; count from format width instead */
        int width_digits = int_digits;
        commas_in_field = (width_digits - 1) / 3;
        if (commas_in_field < 0) commas_in_field = 0;
    }
    int field_width = int_digits + commas_in_field +
                      (frac_digits > 0 ? 1 + frac_digits : 0);

    if (actual_int > int_digits) {
        /* v159 HIGH-3: overflow → single '%' prefix, single sign */
        using_out_char('%');
        if (negative) using_out_char('-');
        char tmp[64];
        if (frac_digits > 0) {
            snprintf(tmp, sizeof(tmp), "%.*f", frac_digits, fabs(val));
        } else if (val == (double)(long long)fabs(val)) {
            snprintf(tmp, sizeof(tmp), "%lld", (long long)fabs(val));
        } else {
            snprintf(tmp, sizeof(tmp), "%g", fabs(val));
            if (strchr(tmp, 'e')) {
                snprintf(tmp, sizeof(tmp), "%f", fabs(val));
                char *tail = tmp + strlen(tmp) - 1;
                while (tail > tmp && *tail == '0') { *tail = '\0'; tail--; }
                if (*tail == '.') *tail = '\0';
            }
        }
        using_out_str(tmp, strlen(tmp));
        if (flags & USING_FLAG_TRAILING_MINUS) using_out_char(negative ? '-' : ' ');
        else if (flags & USING_FLAG_TRAILING_PLUS) using_out_char(negative ? '-' : '+');
        return;
    }

    /* v159 HIGH-2: fill prefix (** → stars, $$ → spaces then $) */
    /* compute available leading columns: total field minus digits the
     * number needs minus sign column if applicable */
    /* v159: sign occupies a column within the field (GW-BASIC
     * right-aligns "-45.68" into "###.##" with no extra space;
     * a leading '+' from the format also occupies a column) */
    int sign_col = (negative || leading_plus ||
                    (flags & USING_FLAG_LEADING_PLUS)) ? 1 : 0;
    int dollar_col = dollar_float ? 1 : 0;
    int used = actual_int + (frac_digits > 0 ? 1 + frac_digits : 0);
    int lead = field_width - used - sign_col - dollar_col;
    if (lead < 0) lead = 0;

    if (star_fill) {
        for (int i = 0; i < lead; i++) using_out_char('*');
    } else {
        for (int i = 0; i < lead; i++) using_out_char(' ');
    }
    if (dollar_float) using_out_char('$');

    if (negative) using_out_char('-');
    else if (leading_plus || (flags & USING_FLAG_LEADING_PLUS)) using_out_char('+');
    using_out_str(int_str, numlen);

    if (frac_digits > 0) {
        using_out_char('.');
        for (int i = 0; i < frac_digits; i++) {
            frac_part *= 10.0;
            int digit = (int)(frac_part + 1e-9);
            if (digit > 9) digit = 9;
            using_out_char((char)('0' + digit));
            frac_part -= digit;
        }
    }

    if (flags & USING_FLAG_TRAILING_MINUS) using_out_char(negative ? '-' : ' ');
    else if (flags & USING_FLAG_TRAILING_PLUS) using_out_char(negative ? '-' : '+');
}

/* Emit a string value into a string field.
 * width: -1 = & (whole), 1 = ! (first char, empty → space), n = \ field */
static void using_emit_string(const char *s, int width) {
    if (width < 0) {
        using_out_str(s, strlen(s));
    } else if (width == 1) {
        /* v159 LOW-4: empty string → single space fill */
        using_out_char(s[0] ? s[0] : ' ');
    } else {
        int n = (int)strlen(s);
        for (int i = 0; i < width; i++)
            using_out_char((i < n) ? s[i] : ' ');
    }
}

void biprtu_print_using_values(const char *fmt, const struct using_value *vals, int count) {
    using_out_reset();
    g_out_len = 0;

    if (!fmt || !*fmt) {
        /* ASM FCERR3: empty USING string → Illegal function call;
         * v159 LOW-3: error only, no stray newline */
        gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
        return;
    }

    int vi = 0;
    bool printed_any = false;   /* USFLG */
    bool any_field = false;     /* MEDIUM-1: fmt with no fields */
    const char *p = fmt;
    bool rescan = false;

scan_restart:
    if (rescan) {
        /* v159: REUSIN — rescan the full format; fields consume the
         * remaining values; stop when a field finds no value
         * (observation 4: ASM ends the format there) */
        if (vi >= count) {
            /* no values left: emit literals until first field */
            while (*p) {
                char ch = *p;
                if (ch == '_') { p++; if (*p) { using_out_char(*p); p++; } continue; }
                if (ch == '#' || ch == '!' || ch == '&' || ch == '\\') break;
                if (ch == '$' && p[1] == '$') break;
                if (ch == '*' && p[1] == '*') break;
                if (ch == '+' || ch == '-') {
                    const char *q = p + 1;
                    while (*q == '+') q++;
                    if (*q == '#') break;
                    using_out_char(ch); p++;
                    continue;
                }
                using_out_char(ch);
                p++;
            }
            using_out_flush();
            return;
        }
        printed_any = false;
    }
    while (*p) {
        char ch = *p;
        if (ch == '_') {
            /* v159 MEDIUM-3: escape — next char is a literal */
            p++;
            if (*p) { using_out_char(*p); p++; }
            continue;
        }
        if (ch == '$' && p[1] == '$') {
            /* v159 HIGH-2: $$ floating dollar — numeric field follows */
            if (vi < count) {
                const char *q = p + 2;
                int id, fd; unsigned char fl;
                bool too_long = false;
                if (*q == '#') {
                    q = using_scan_numfield(q, &id, &fd, &fl, &too_long);
                    if (too_long) {
                        gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    if (!vals[vi].is_str) {
                        using_emit_number(vals[vi].num, id + 1, fd, fl, false, false, true);
                        vi++; printed_any = true; any_field = true;
                    } else {
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    p = q;
                    continue;
                }
            }
            /* no value or not numeric field: literal '$$' */
            using_out_char('$'); p++;
            continue;
        }
        if (ch == '*' && (p[1] == '*' )) {
            /* v159 HIGH-2: ** star fill — numeric field follows
             * (**$ → star fill + floating dollar) */
            const char *q = p + 2;
            bool dol = (*q == '$');
            if (dol) q++;
            if (*q == '#') {
                if (vi < count) {
                    int id, fd; unsigned char fl;
                    bool too_long = false;
                    q = using_scan_numfield(q, &id, &fd, &fl, &too_long);
                    if (too_long) {
                        gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    if (!vals[vi].is_str) {
                        /* field width includes the 2 star columns and
                         * the $ column (ASM **$ = star-filled dollar) */
                        using_emit_number(vals[vi].num, id + 2 + (dol ? 1 : 0), fd,
                                          fl, false, true, dol);
                        vi++; printed_any = true; any_field = true;
                    } else {
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    p = q;
                    continue;
                }
            }
            /* literal '**' */
            using_out_char('*'); using_out_char('*');
            p += 2;
            if (dol) { using_out_char('$'); p++; }
            continue;
        }
        if (ch == '+' || ch == '-') {
            bool lead_plus = (ch == '+');
            const char *q = p + 1;
            while (*q == '+') q++;
            if (*q == '#') {
                int id, fd; unsigned char fl;
                bool too_long = false;
                q = using_scan_numfield(q, &id, &fd, &fl, &too_long);
                if (too_long) {
                    gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    using_out_flush();
                    return;
                }
                any_field = true;
                if (vi < count) {
                    if (!vals[vi].is_str) {
                        using_emit_number(vals[vi].num, id, fd, fl, lead_plus, false, false);
                    } else {
                        /* v159 HIGH-8: string into numeric field → TMERR */
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    vi++; printed_any = true;
                }
                p = q;
                continue;
            }
            using_out_char(ch); p++;
            continue;
        }
        if (ch == '#') {
            int id, fd; unsigned char fl;
            bool too_long = false;
            p = using_scan_numfield(p, &id, &fd, &fl, &too_long);
            if (too_long) {
                gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
                using_out_flush();
                return;
            }
            any_field = true;
            if (vi < count) {
                if (!vals[vi].is_str) {
                    using_emit_number(vals[vi].num, id, fd, fl, false, false, false);
                } else {
                    /* v159 HIGH-8: TMERR */
                    gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    using_out_flush();
                    return;
                }
                vi++; printed_any = true;
            }
            continue;
        }
        if (ch == '!') {
            any_field = true;
            if (vi < count) {
                if (vals[vi].is_str) {
                    using_emit_string(vals[vi].str ? vals[vi].str : "", 1);
                } else {
                    /* v159 HIGH-8: number into string field → TMERR */
                    gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    using_out_flush();
                    return;
                }
                vi++; printed_any = true;
            }
            p++;
            continue;
        }
        if (ch == '&') {
            any_field = true;
            if (vi < count) {
                if (vals[vi].is_str) {
                    using_emit_string(vals[vi].str ? vals[vi].str : "", -1);
                } else {
                    gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    using_out_flush();
                    return;
                }
                vi++; printed_any = true;
            }
            p++;
            continue;
        }
        if (ch == '\\') {
            int width = 2;
            const char *q = p + 1;
            while (*q == ' ') { width++; q++; }
            if (*q == '\\') {
                any_field = true;
                if (vi < count) {
                    if (vals[vi].is_str) {
                        using_emit_string(vals[vi].str ? vals[vi].str : "", width);
                    } else {
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        using_out_flush();
                        return;
                    }
                    vi++; printed_any = true;
                }
                p = q + 1;
            } else {
                using_out_char(ch); p++;
            }
            continue;
        }
        /* ordinary character → verbatim */
        using_out_char(ch);
        p++;
    }

    /* ASM REUSST/FCERR3:
     * - format has no fields at all + values → FCERR (MEDIUM-1)
     * - values remain and format was fully used → REUSIN rescan;
     *   the second pass emits literals and stops at the first field */
    if (vi < count) {
        if (!any_field) {
            gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
            using_out_flush();
            return;
        }
        if (printed_any) {
            rescan = true;
            p = fmt;
            goto scan_restart;
        }
    }
    using_out_flush();
}

/* Legacy single-value helpers (kept for API compatibility) */
void biprtu_print_using(const char *fmt, double val) {
    struct using_value v;
    v.is_str = false;
    v.num = val;
    v.str = NULL;
    biprtu_print_using_values(fmt, &v, 1);
}

void biprtu_print_using_str(const char *fmt, const char *str) {
    struct using_value v;
    v.is_str = true;
    v.num = 0;
    v.str = (char *)str;
    biprtu_print_using_values(fmt, &v, 1);
}
