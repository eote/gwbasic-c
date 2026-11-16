/* GWEVAL - Expression Evaluation */
/* Based on GWEVAL.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include "dskcom.h"

/* Case-insensitive string comparison - portable implementation */
static int strcasecmp_ascii(const char *a, const char *b) {
    while (*a && *b) {
        int c = tolower((unsigned char)*a);
        int d = tolower((unsigned char)*b);
        if (c != d) return c - d;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Case-insensitive string comparison (first n chars) - portable implementation */
static int strncasecmp_ascii(const char *a, const char *b, size_t n) {
    while (n > 0 && *a && *b) {
        int c = tolower((unsigned char)*a);
        int d = tolower((unsigned char)*b);
        if (c != d) return c - d;
        a++; b++; n--;
    }
    if (n == 0) return 0;
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Parser state */
typedef struct {
    const char *input;
    size_t pos;
    size_t len;
} parser_t;

static void parser_init(parser_t *p, const char *input) {
    p->input = input;
    p->pos = 0;
    p->len = strlen(input);
}

static char parser_peek(parser_t *p) {
    while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) {
        p->pos++;
    }
    if (p->pos < p->len) return p->input[p->pos];
    return '\0';
}

static char parser_get(parser_t *p) {
    char c = parser_peek(p);
    if (c) p->pos++;
    return c;
}

static bool parser_match(parser_t *p, char c) {
    if (parser_peek(p) == c) {
        p->pos++;
        return true;
    }
    return false;
}

/* Forward declarations for recursive functions */
static double parse_expression(parser_t *p);
static char *parse_string_expression(parser_t *p);

/* v169 HIGH-1: VALTYP (ASM GWEVAL/GWDATA VALTYP) — type of the LAST
 * evaluated numeric expression: 4 = single, 8 = double, 2 = integer.
 * PRINT uses it to choose 7 vs 16 digit output; assign uses it to
 * store single vs double. */
static int g_valtyp = 4;
int gweval_last_valtyp(void) { return g_valtyp; }

static char *parse_string_expression(parser_t *p) {
    char *result = NULL;
    
    /* Try to parse as string literal first */
    char c = parser_peek(p);
    if (c == '\"') {
        parser_get(p);  /* consume opening quote */
        char buf[256];
        size_t i = 0;
        while (p->pos < p->len && p->input[p->pos] != '\"' && i < sizeof(buf) - 1) {
            buf[i++] = p->input[p->pos++];
        }
        if (p->pos < p->len && p->input[p->pos] == '\"') {
            p->pos++;  /* consume closing quote */
        }
        buf[i] = '\0';
        /* v165 HIGH-1: literals are interned — a loop body re-parsing
         * the same "" literal every iteration must not drain the heap */
        result = gwdata_intern_literal(buf, strlen(buf));
        if (!result) {
            result = gwdata_alloc_string(strlen(buf) + 1);
            if (result) strcpy(result, buf);
            /* OOM → gwdata_alloc_string sets OM(7) itself (v165) */
        }
    }
    
    /* Otherwise parse as general string expression (variable, array, function, concatenation) */
    if (!result && isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_' || p->input[p->pos] == '$') && i < sizeof(name) - 1) {
            name[i++] = p->input[p->pos++];
        }
        name[i] = '\0';
        
        /* Check for string function call: LEFT$(, RIGHT$(, MID$(, UCASE$(, LCASE$(, STR$(, CHR$(, SPACE$(, STRING$( */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            if (strcasecmp_ascii(name, "LEFT") == 0 || strcasecmp_ascii(name, "LEFT$") == 0) {
                /* LEFT$(str, n) — ASM PREAM/GETBYT: n 0..255 else FCERR */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ')')) {
                            int n = (int)arg2;
                            if (n < 0 || n > 255) {
                                if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                                return NULL;
                            }
                            return bistrs_left(str_val, n);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "RIGHT") == 0 || strcasecmp_ascii(name, "RIGHT$") == 0) {
                /* RIGHT$(str, n) — ASM PREAM/GETBYT: n 0..255 else FCERR */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ')')) {
                            int n = (int)arg2;
                            if (n < 0 || n > 255) {
                                if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                                return NULL;
                            }
                            return bistrs_right(str_val, n);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "MID") == 0 || strcasecmp_ascii(name, "MID$") == 0) {
                /* MID$(str, start[, len]) — ASM MID$ L763-796:
                 * start==0 → FCERR ("IT MUST NOT BE 0"); start/len are
                 * GETBYT bytes (0..255, but start 0 → FCERR); 2-arg form
                 * returns rest of string (len clamped to end). */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ')')) {
                            /* 2-arg: rest of string */
                            int start = (int)arg2;
                            if (start < 1 || start > 255) {
                                if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                                return NULL;
                            }
                            return bistrs_mid(str_val, start, 0x7FFF);
                        }
                        if (parser_match(p, ',')) {
                            float arg3 = parse_expression(p);
                            if (parser_match(p, ')')) {
                                int start = (int)arg2;
                                int len = (int)arg3;
                                if (start < 1 || start > 255 || len < 0 || len > 255) {
                                    if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                                    return NULL;
                                }
                                return bistrs_mid(str_val, start, len);
                            }
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "UCASE") == 0 || strcasecmp_ascii(name, "UCASE$") == 0) {
                /* UCASE$(str) */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ')')) {
                        return bistrs_ucase(str_val);
                    }
                }
            }
            else if (strcasecmp_ascii(name, "LCASE") == 0 || strcasecmp_ascii(name, "LCASE$") == 0) {
                /* LCASE$(str) */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ')')) {
                        return bistrs_lcase(str_val);
                    }
                }
            }
            else if (strcasecmp_ascii(name, "STR") == 0 || strcasecmp_ascii(name, "STR$") == 0) {
                /* STR$(num) — v170 HIGH-5: double operand prints 15
                 * digits (GW-BASIC VALTYP-aware) */
                double arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    if (g_valtyp == 8) {
                        char *result2 = (char *)gwram_alloc(48);
                        if (!result2) return NULL;
                        sprintf(result2, "%s%.15g", (arg >= 0) ? " " : "", arg);
                        return result2;
                    }
                    return bistrs_str(arg);
                }
            }
            else if (strcasecmp_ascii(name, "HEX") == 0 || strcasecmp_ascii(name, "HEX$") == 0) {
                /* v158: HEX$(num) — 16-bit two's-complement pattern */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_hex(arg);
                }
            }
            else if (strcasecmp_ascii(name, "OCT") == 0 || strcasecmp_ascii(name, "OCT$") == 0) {
                /* v158: OCT$(num) — 16-bit two's-complement pattern */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_oct(arg);
                }
            }
            else if (strcasecmp_ascii(name, "CHR") == 0 || strcasecmp_ascii(name, "CHR$") == 0) {
                /* CHR$(num) — ASM CHR$: CONINT → 0..255 else FCERR */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    int code = (int)arg;
                    if (code < 0 || code > 255) {
                        if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                        return NULL;
                    }
                    return bistrs_chr(code);
                }
            }
            else if (strcasecmp_ascii(name, "SPACE") == 0 || strcasecmp_ascii(name, "SPACE$") == 0) {
                /* SPACE$(num) — ASM SPACE$: CONINT → 0..255 else FCERR */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    int n = (int)arg;
                    if (n < 0 || n > 255) {
                        if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                        return NULL;
                    }
                    return bistrs_space(n);
                }
            }
            else if (strcasecmp_ascii(name, "STRING") == 0 || strcasecmp_ascii(name, "STRING$") == 0) {
                /* STRING$(num, char|str$) — ASM STRNG$ L658: n byte 0..255
                 * else FCERR; arg2 via GETYPR: string → ASC2 (first char),
                 * numeric → CONINT 0..255. Type probe like INSTR: quote
                 * start = unambiguous string; else numeric. */
                float arg1 = parse_expression(p);
                if (parser_match(p, ',')) {
                    int n = (int)arg1;
                    if (n < 0 || n > 255) {
                        if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                        return NULL;
                    }
                    size_t save_pos2 = p->pos;
                    while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) p->pos++;
                    char first2 = (p->pos < p->len) ? p->input[p->pos] : '\0';
                    p->pos = save_pos2;
                    if (first2 == '"' || isalpha((unsigned char)first2) || first2 == '_') {
                        /* STRING$(n, "str") or STRING$(n, str$) or
                         * STRING$(n, LEFT$(...)) — repeat first char of the
                         * string (ASM STRNG$: GETYPR → string → ASC2).
                         * v137 MEDIUM-1: letter start is AMBIGUOUS in theory
                         * (string var S$ vs numeric func LEN) — fall back to
                         * a FULL string-parse probe first (v134 INSTR three-
                         * tier pattern); success → string form, failure →
                         * rewind and numeric form. Quote start is
                         * unambiguous string (probe cannot rewind past a
                         * consumed string literal cleanly, and a quote can
                         * never start a numeric expression anyway). */
                        char *s2 = parse_string_expression(p);
                        if (s2) {
                            if (parser_match(p, ')')) {
                                char ch = s2[0];
                                return bistrs_string(n, ch);
                            }
                        }
                        if (first2 == '"') {
                            /* string literal form malformed → SNERR */
                            if (g_state) gwdata_set_error(2, g_state->current_line, 0);
                            return NULL;
                        }
                        /* Ambiguous start: rewind to the ',' and try the
                         * numeric form (e.g. STRING$(3, LEN(A$))) */
                        p->pos = save_pos2;
                    }
                    float arg2 = parse_expression(p);
                    if (parser_match(p, ')')) {
                        int code = (int)arg2;
                        if (code < 0 || code > 255) {
                            if (g_state) gwdata_set_error(5, g_state->current_line, 0);
                            return NULL;
                        }
                        return bistrs_string(n, (char)code);
                    }
                }
            }
            /* MKI$(num) - convert integer to 2-byte string */
            else if (strcasecmp_ascii(name, "MKI") == 0 || strcasecmp_ascii(name, "MKI$") == 0) {
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_mki((int)arg);
                }
            }
            /* MKS$(num) - convert float to 4-byte string */
            else if (strcasecmp_ascii(name, "MKS") == 0 || strcasecmp_ascii(name, "MKS$") == 0) {
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_mks(arg);
                }
            }
            
            /* If we get here, it wasn't a recognized string function - 
               rewind to handle as array/variable */
            p->pos -= 1;  /* Put back '(' */
        }
        
        /* Check for array reference: A$(5) or A$(1,2) */
        /* v164: a name followed by '(' that IS a known function
         * (rewound after a failed string-function match, e.g. the
         * numeric LEN/INSTR/EOF/FRE family) is NOT an array read —
         * treating it as one made the string-probe in callers
         * spuriously succeed on a freshly created empty element. */
        bool is_known_func_name =
            strcasecmp_ascii(name, "LEN") == 0 ||
            strcasecmp_ascii(name, "INSTR") == 0 ||
            strcasecmp_ascii(name, "EOF") == 0 ||
            strcasecmp_ascii(name, "FRE") == 0 ||
            strcasecmp_ascii(name, "VAL") == 0 ||
            strcasecmp_ascii(name, "ASC") == 0 ||
            strcasecmp_ascii(name, "TAB") == 0 ||
            strcasecmp_ascii(name, "SPC") == 0 ||
            strcasecmp_ascii(name, "POINT") == 0 ||
            strcasecmp_ascii(name, "RND") == 0 ||
            strcasecmp_ascii(name, "LPOS") == 0 ||
            strcasecmp_ascii(name, "CSRLIN") == 0 ||
            strcasecmp_ascii(name, "PEEK") == 0 ||
            strcasecmp_ascii(name, "USR") == 0;
        if (parser_peek(p) == '(' && !is_known_func_name) {
            parser_get(p);  /* consume '(' */
            
            /* v161 HIGH-2: multi-dimensional string arrays */
            int snd_indices[8];
            int snd_n = 0;
            bool snd_ok = true;
            for (;;) {
                float idx_val = parse_expression(p);
                if (gwdata_get_error() != 0) { snd_ok = false; break; }
                if (snd_n < 8) {
                    int iv = (int)idx_val;
                    snd_indices[snd_n++] = (iv < 0) ? 0 : iv;
                }
                if (parser_match(p, ',')) continue;
                if (parser_match(p, ')')) break;
                snd_ok = false;
                break;
            }
            if (snd_ok && snd_n > 0) {
                if (snd_n > 1) {
                    /* v162/v164 HIGH-2: auto-DIM + rank validation */
                    extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                    biptrg_ensure_array_nd(name, snd_n);
                    if (gwdata_get_error() != 0) return NULL;
                    result = biptrg_get_array_string_multi(name, snd_indices);
                }
                else
                    result = biptrg_get_array_string(name, snd_indices[0]);

                /* fall through to concat loop (v138: S$ = S$ + "X" idiom —
                 * early return silently dropped the "+ ..." tail) */
                goto after_name;
            }
        }
        
        /* Scalar string variable.
         * v164 HIGH-1: an UNDEFINED string variable is the empty
         * string (GW-BASIC default) — not NULL (which callers
         * interpreted as "not a string" → TMERR 13 on assignment,
         * and printed as 0). */
        struct variable *v = gwdata_find_variable(name);
        if ((v && v->type == VT_STRING && !v->value.s.ptr) ||
            (!v && name[0] != '\0' && name[strlen(name) - 1] == '$')) {
            /* v165 HIGH-1: defined-but-unassigned or undefined string
             * variable reads as the empty string. Static sentinel —
             * do NOT allocate (a 4000-iteration `IF C$=""` loop
             * drained the 16 KB string heap at 4 bytes/eval). */
            static char g_empty_str[] = "";
            result = g_empty_str;
            goto after_name;
        }
        if (v && v->type == VT_STRING && v->value.s.ptr) {
            if (v->is_field) {
                /* Field variables may not be null-terminated - create a copy */
                char *fcopy = gwdata_alloc_string(v->value.s.len + 1);
                if (fcopy) {
                    memcpy(fcopy, v->value.s.ptr, v->value.s.len);
                    fcopy[v->value.s.len] = '\0';
                }
                result = fcopy;
            } else {
                result = v->value.s.ptr;
            }
            /* fall through to concat loop (v138: S$ = S$ + "X" idiom — the
             * early return silently dropped the "+ ..." tail) */
            goto after_name;
        }
    }
    
after_name:
    /* Check for string concatenation with + */
    /* Handle cases like "Line " + STR$(I) */
    while (p->pos < p->len) {
        /* Skip whitespace */
        while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) p->pos++;
        if (p->pos < p->len && p->input[p->pos] == '+') {
            p->pos++;  /* consume '+' */
            /* Skip whitespace after + */
            while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) p->pos++;
            
            /* Parse right side of concatenation */
            char *right = parse_string_expression(p);
            if (right && result) {
                /* Concatenate result + right */
                size_t len1 = strlen(result);
                size_t len2 = strlen(right);
                char *concat = gwdata_alloc_string(len1 + len2 + 1);
                if (concat) {
                    strcpy(concat, result);
                    strcat(concat, right);
                    result = concat;
                }
            } else if (right) {
                result = right;
            }
            continue;  /* Check for more + operators */
        }
        break;  /* No more + operators */
    }
    
    /* Default: not a recognized string expression — return NULL so caller can try numeric eval */
    return result;
}

static double parse_factor(parser_t *p) {
    char c = parser_peek(p);
    
    if (c == '(') {
        parser_get(p); /* consume '(' */
        double result = parse_expression(p);
        parser_match(p, ')'); /* consume ')' */
        return result;
    }
    
    if (c == '-') {
        parser_get(p);
        return -parse_factor(p);
    }
    
    if (c == '+') {
        parser_get(p);
        return parse_factor(p);
    }
    
    /* Handle NOT operator */
    /* Check if next chars spell "NOT" (case insensitive) */
    if (p->pos + 2 < p->len) {
        char c1 = p->input[p->pos];
        char c2 = p->input[p->pos + 1];
        char c3 = p->input[p->pos + 2];
        if ((c1 == 'N' || c1 == 'n') && (c2 == 'O' || c2 == 'o') && (c3 == 'T' || c3 == 't')) {
            /* Check that it's a whole word (next char is not alphanumeric/underscore) */
            if (p->pos + 3 >= p->len || (!isalnum((unsigned char)p->input[p->pos + 3]) && p->input[p->pos + 3] != '_')) {
                p->pos += 3;  /* consume "NOT" */
                double val = parse_factor(p);
                return (val == 0.0f) ? -1.0f : 0.0f;  /* GW-BASIC: NOT 0 = -1, NOT non-zero = 0 */
            }
        }
    }
    
    /* Number — v159: scientific notation (1E+10, 2.5D-3; GW-BASIC
     * accepts both E and D exponents) */
    if (isdigit((unsigned char)c) || (c == '.' && p->pos + 1 < p->len &&
        isdigit((unsigned char)p->input[p->pos + 1]))) {
        char buf[64];
        size_t i = 0;
        bool d_exp = false;   /* v171 MEDIUM-3: D exponent = double */
        while (p->pos < p->len && (isdigit((unsigned char)p->input[p->pos]) || p->input[p->pos] == '.') && i < sizeof(buf) - 1) {
            buf[i++] = p->input[p->pos++];
        }
        /* exponent part: E or D followed by optional sign + digits */
        if (p->pos < p->len && (p->input[p->pos] == 'E' || p->input[p->pos] == 'e' ||
                                p->input[p->pos] == 'D' || p->input[p->pos] == 'd')) {
            /* only treat as exponent if a digit follows (optionally
             * after a sign) — otherwise "E" is a variable */
            size_t save = p->pos;
            size_t q = p->pos + 1;
            if (q < p->len && (p->input[q] == '+' || p->input[q] == '-')) q++;
            if (q < p->len && isdigit((unsigned char)p->input[q])) {
                /* v171 MEDIUM-3: 'D' exponent = double literal
                 * (GW-BASIC BASCOM: 1.234D0 ≡ 1.234#) */
                if (p->input[p->pos] == 'D' || p->input[p->pos] == 'd') {
                    d_exp = true;
                }
                buf[i++] = 'E';  /* normalize D → E for atof */
                p->pos++;
                if (p->pos < p->len && (p->input[p->pos] == '+' || p->input[p->pos] == '-')) {
                    buf[i++] = p->input[p->pos++];
                }
                while (p->pos < p->len && isdigit((unsigned char)p->input[p->pos]) && i < sizeof(buf) - 1) {
                    buf[i++] = p->input[p->pos++];
                }
            } else {
                (void)save;  /* not an exponent: E belongs to a name */
            }
        }
        buf[i] = '\0';
        /* v160 HIGH-2: constant out of single-precision range →
         * Overflow (6); isfinite guards against atof overflow to inf */
        double dval = atof(buf);
        /* v169 HIGH-1: '#' literal suffix → DOUBLE precision */
        bool lit_double = d_exp;   /* v171: D exponent implies double */
        if (p->pos < p->len && p->input[p->pos] == '#') {
            p->pos++;
            lit_double = true;
        }
        g_valtyp = lit_double ? 8 : 4;
        /* v169: single literals round to float precision (GW-BASIC
         * MBF single) — `A! = 3.14` must compare equal to `3.14` */
        if (!lit_double) dval = (double)(float)dval;
        if (dval > 3.402823e38 || (dval != 0.0 && dval < -3.402823e38)) {
            gwdata_set_error(6, g_state ? (uint16_t)g_state->current_line : 0, 0);
            return 0.0f;
        }
        if (dval != 0.0 && !isfinite(dval)) {
            gwdata_set_error(6, g_state ? (uint16_t)g_state->current_line : 0, 0);
            return 0.0f;
        }
        return dval;
    }
    
    /* String literal in numeric context = 0 */
    if (c == '\"') {
        parser_get(p);  /* consume opening quote */
        while (p->pos < p->len && p->input[p->pos] != '\"') p->pos++;
        if (p->pos < p->len && p->input[p->pos] == '\"') p->pos++;
        return 0.0f;
    }
    
    if (isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        /* v138: include GW-BASIC type suffixes %, !, # — typed numeric
         * variables (A%, A!, A#) were unreadable in expressions (parse cut
         * at the suffix, find_variable("A") missed "A%") */
        while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_' || p->input[p->pos] == '$' || p->input[p->pos] == '%' || p->input[p->pos] == '!' || p->input[p->pos] == '#') && i < sizeof(name) - 1) {
            name[i++] = p->input[p->pos++];
        }
        name[i] = '\0';
        
        /* Check for FN keyword - if we just read "FN", continue to read the function name */
        if (strcasecmp_ascii(name, "FN") == 0) {
            /* Skip whitespace */
            while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) p->pos++;
            
            /* Read the actual function name */
            char fn_name[41];
            size_t j = 0;
            fn_name[0] = 'F'; fn_name[1] = 'N';  /* Start with "FN" prefix */
            j = 2;
            while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_') && j < sizeof(fn_name) - 1) {
                fn_name[j++] = p->input[p->pos++];
            }
            fn_name[j] = '\0';
            
            /* Copy back to name for rest of processing */
            strncpy(name, fn_name, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        }
        
        /* Check for special ERR variable (GW-BASIC error code) */
        if (strcasecmp_ascii(name, "ERR") == 0) {
            struct variable *err_var = gwdata_find_variable("ERR");
            if (err_var && err_var->type == VT_INTEGER) {
                return (float)err_var->value.i;
            }
            return 0.0f;
        }
        
        /* v169: ERL variable — the line number of the offending
         * statement (ASM GWMAIN L472: ERRLIN=CURLIN on error;
         * GWEVAL L673: ERL reads ERRLIN). Unassigned (no error) → 0,
         * matching GW-BASIC. */
        if (strcasecmp_ascii(name, "ERL") == 0) {
            return (float)gwdata_get_error_line();
        }
        
        /* Check for function call - BEFORE array reference check */
        /* Only consume '(' if it's a known function name */
        if (parser_peek(p) == '(') {
            /* Check if name matches a known math function BEFORE consuming '(' */
            bool is_math_func = (strcasecmp_ascii(name, "SIN") == 0 ||
                                 strcasecmp_ascii(name, "COS") == 0 ||
                                 strcasecmp_ascii(name, "TAN") == 0 ||
                                 strcasecmp_ascii(name, "ATN") == 0 ||
                                 strcasecmp_ascii(name, "EXP") == 0 ||
                                 strcasecmp_ascii(name, "LOG") == 0 ||
                                 strcasecmp_ascii(name, "SQR") == 0 ||
                                 strcasecmp_ascii(name, "ABS") == 0 ||
                                 strcasecmp_ascii(name, "SGN") == 0 ||
                                 strcasecmp_ascii(name, "RND") == 0 ||
                                 strcasecmp_ascii(name, "INT") == 0 ||
                                 strcasecmp_ascii(name, "FIX") == 0);
            
            /* Also check for TAB( and SPC( - they're PRINT functions but return numeric */
            bool is_tab_spc = (strcasecmp_ascii(name, "TAB") == 0 ||
                               strcasecmp_ascii(name, "SPC") == 0);
            
            /* Check for string functions that return numeric: LEN, ASC, VAL, EOF, FRE */
            bool is_str_numeric = (strcasecmp_ascii(name, "LEN") == 0 ||
                                   strcasecmp_ascii(name, "ASC") == 0 ||
                                   strcasecmp_ascii(name, "VAL") == 0 ||
                                   strcasecmp_ascii(name, "EOF") == 0 ||
                                   strcasecmp_ascii(name, "FRE") == 0 ||
                                   strcasecmp_ascii(name, "INSTR") == 0);
            
            /* POINT(x,y) — graphics pixel read (ASM POINT GENGRP L172):
             * returns attribute or -1 if out of bounds */
            if (strcasecmp_ascii(name, "POINT") == 0) {
                parser_get(p);  /* consume '(' */
                float px = parse_expression(p);
                if (parser_match(p, ',')) {
                    float py = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (float)gengrp_point((int)px, (int)py);
                    }
                }
                /* malformed → SNERR */
                if (g_state) gwdata_set_error(2, g_state->current_line, 0);
                return 0.0f;
            }
            /* Check for string functions that return string: LEFT$, RIGHT$, MID$, UCASE$, LCASE$, STR$, CHR$, SPACE$, STRING$, MKI$, MKS$ */
            bool is_str_func = (strcasecmp_ascii(name, "LEFT") == 0 || strcasecmp_ascii(name, "LEFT$") == 0 ||
                                strcasecmp_ascii(name, "RIGHT") == 0 || strcasecmp_ascii(name, "RIGHT$") == 0 ||
                                strcasecmp_ascii(name, "MID") == 0 || strcasecmp_ascii(name, "MID$") == 0 ||
                                strcasecmp_ascii(name, "UCASE") == 0 || strcasecmp_ascii(name, "UCASE$") == 0 ||
                                strcasecmp_ascii(name, "LCASE") == 0 || strcasecmp_ascii(name, "LCASE$") == 0 ||
                                strcasecmp_ascii(name, "STR") == 0 || strcasecmp_ascii(name, "STR$") == 0 ||
                                strcasecmp_ascii(name, "CHR") == 0 || strcasecmp_ascii(name, "CHR$") == 0 ||
                                strcasecmp_ascii(name, "SPACE") == 0 || strcasecmp_ascii(name, "SPACE$") == 0 ||
                                strcasecmp_ascii(name, "STRING") == 0 || strcasecmp_ascii(name, "STRING$") == 0 ||
                                strcasecmp_ascii(name, "MKI") == 0 || strcasecmp_ascii(name, "MKI$") == 0 ||
                                strcasecmp_ascii(name, "MKS") == 0 || strcasecmp_ascii(name, "MKS$") == 0);
            
            /* Check for numeric functions that convert from string: CVI, CVS */
            bool is_cv_func = (strcasecmp_ascii(name, "CVI") == 0 || strcasecmp_ascii(name, "CVS") == 0);
            
            if (is_math_func || is_tab_spc || is_str_numeric || is_str_func || is_cv_func) {
                /* It's a known function - consume '(' and call it */
                parser_get(p);  /* consume '(' */
                
                /* Math functions: SIN, COS, TAN, ATN, EXP, LOG, SQR, ABS, SGN, RND, INT, FIX */
                if (strcasecmp_ascii(name, "SIN") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        /* v170 HIGH-4: double input → double math */
                        return (g_valtyp == 8) ? sin(arg) : (double)sinf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "COS") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? cos(arg) : (double)cosf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "TAN") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? tan(arg) : (double)tanf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "ATN") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? atan(arg) : (double)atanf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "EXP") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? exp(arg) : (double)expf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "LOG") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? log(arg) : (double)logf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "SQR") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (g_valtyp == 8) ? sqrt(arg) : (double)sqrtf((float)arg);
                    }
                }
                else if (strcasecmp_ascii(name, "ABS") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return fabs(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "SGN") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        if (arg > 0) return 1.0f;
                        else if (arg < 0) return -1.0f;
                        else return 0.0f;
                    }
                }
                else if (strcasecmp_ascii(name, "RND") == 0) {
                    parse_expression(p);  /* consume optional argument, but ignore */
                    if (parser_match(p, ')')) {
                        return (float)rand() / (float)RAND_MAX;
                    }
                }
                else if (strcasecmp_ascii(name, "INT") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return floorf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "FIX") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return truncf(arg);
                    }
                }
                /* TAB/SPC - PRINT functions that return numeric */
                else if (strcasecmp_ascii(name, "TAB") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return arg;
                    }
                }
                else if (strcasecmp_ascii(name, "SPC") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return arg;
                    }
                }
                /* String functions returning numeric: LEN, ASC, VAL, EOF */
                else if (strcasecmp_ascii(name, "LEN") == 0) {
                    char *str_val = parse_string_expression(p);
                    if (str_val) {
                        if (parser_match(p, ')')) {
                            return (float)bistrs_len(str_val);
                        }
                    }
                }
                else if (strcasecmp_ascii(name, "ASC") == 0) {
                    char *str_val = parse_string_expression(p);
                    if (str_val) {
                        if (parser_match(p, ')')) {
                            return (float)bistrs_asc(str_val);
                        }
                    }
                }
                else if (strcasecmp_ascii(name, "VAL") == 0) {
                    char *str_val = parse_string_expression(p);
                    if (str_val) {
                        if (parser_match(p, ')')) {
                            return bistrs_val(str_val);
                        }
                    }
                }
                else if (strcasecmp_ascii(name, "EOF") == 0) {
                    /* EOF(filenum) - check if file at EOF */
                    float fn = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (float)dskcom_eof((int)fn);
                    }
                }
                /* INSTR([start,] S1$, S2$) — numeric result (ASM INSTRC
                 * BISTRS.ASM L841: FRMPRN + GETYPR distinguishes string vs
                 * numeric first arg at runtime).
                 * v133 LOW-1: illegal forms (1-arg / S2 parse failure) set
                 * SNERR instead of silently returning 0.
                 * v134 MEDIUM-2: letter-start is AMBIGUOUS (string var A$ /
                 * string func LEFT$() vs numeric var N / numeric func LEN()) —
                 * fall back to a FULL parse_string_expression probe (v132
                 * behavior); probe success → 2-arg, failure → rewind + 3-arg.
                 * Quote/digit starts stay cheap (unambiguous).
                 * v134 MEDIUM-1: bistrs_instr FCERR (-1) propagates as error
                 * code 5 (Illegal function call) — same as SNERR handling. */
                else if (strcasecmp_ascii(name, "INSTR") == 0) {
                    /* Type probe: skip spaces, peek first char. Only
                     * unambiguous starts skip the full-parse probe: '"' is
                     * always a string literal; digit/'(' always numeric. */
                    size_t save_pos = p->pos;
                    while (p->pos < p->len && isspace((unsigned char)p->input[p->pos])) p->pos++;
                    char first = (p->pos < p->len) ? p->input[p->pos] : '\0';
                    p->pos = save_pos;

                    if (first == '"') {
                        /* Unambiguous 2-arg: INSTR(S1$, S2$) */
                        char *str_val = parse_string_expression(p);
                        if (str_val) {
                            if (parser_match(p, ',')) {
                                char *s2 = parse_string_expression(p);
                                if (s2) {
                                    if (parser_match(p, ')')) {
                                        return (float)bistrs_instr(str_val, s2, 1);
                                    }
                                }
                            }
                        }
                        /* 1-arg INSTR(S1$) or S2 parse failure → SNERR */
                        if (g_state) {
                            gwdata_set_error(2, g_state->current_line, 0);
                        }
                        return 0.0f;
                    } else if (isdigit((unsigned char)first) || first == '(' ||
                               first == '-' || first == '+') {
                        /* Unambiguous 3-arg: INSTR(start, S1$, S2$) */
                        float start = parse_expression(p);
                        if (parser_match(p, ',')) {
                            char *str_val = parse_string_expression(p);
                            if (str_val) {
                                if (parser_match(p, ',')) {
                                    char *s2 = parse_string_expression(p);
                                    if (s2) {
                                        if (parser_match(p, ')')) {
                                            int pos = bistrs_instr(str_val, s2, (int)start);
                                            if (pos < 0) {
                                                /* FCERR propagation (v134
                                                 * MEDIUM-1): Illegal function
                                                 * call */
                                                if (g_state) {
                                                    gwdata_set_error(5, g_state->current_line, 0);
                                                }
                                                return 0.0f;
                                            }
                                            return (float)pos;
                                        }
                                    }
                                }
                            }
                        }
                        /* malformed 3-arg → SNERR */
                        if (g_state) {
                            gwdata_set_error(2, g_state->current_line, 0);
                        }
                        return 0.0f;
                    } else {
                        /* Ambiguous start (letter/underscore/$): full parse
                         * probe — string parse success → 2-arg; failure →
                         * rewind and 3-arg. */
                        char *probe_s1 = parse_string_expression(p);
                        if (probe_s1) {
                            if (parser_match(p, ',')) {
                                char *s2 = parse_string_expression(p);
                                if (s2) {
                                    if (parser_match(p, ')')) {
                                        return (float)bistrs_instr(probe_s1, s2, 1);
                                    }
                                }
                            }
                            /* 1-arg INSTR(S1$) → SNERR */
                            if (g_state) {
                                gwdata_set_error(2, g_state->current_line, 0);
                            }
                            return 0.0f;
                        } else {
                            /* Not a string expr → 3-arg with numeric start */
                            p->pos = save_pos;
                            float start = parse_expression(p);
                            if (parser_match(p, ',')) {
                                char *str_val = parse_string_expression(p);
                                if (str_val) {
                                    if (parser_match(p, ',')) {
                                        char *s2 = parse_string_expression(p);
                                        if (s2) {
                                            if (parser_match(p, ')')) {
                                                int pos = bistrs_instr(str_val, s2, (int)start);
                                                if (pos < 0) {
                                                    /* FCERR (v134 MEDIUM-1) */
                                                    if (g_state) {
                                                        gwdata_set_error(5, g_state->current_line, 0);
                                                    }
                                                    return 0.0f;
                                                }
                                                return (float)pos;
                                            }
                                        }
                                    }
                                }
                            }
                            if (g_state) {
                                gwdata_set_error(2, g_state->current_line, 0);
                            }
                            return 0.0f;
                        }
                    }
                }
                /* FRE(arg) - return free memory (arg: 0 for numeric, non-zero for string) */
                else if (strcasecmp_ascii(name, "FRE") == 0) {
                    double arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return (float)bistrs_fre((int)arg);
                    }
                }
                /* CVI(string) - convert 2-byte string to integer */
                else if (strcasecmp_ascii(name, "CVI") == 0) {
                    char *str_val = parse_string_expression(p);
                    if (str_val) {
                        if (parser_match(p, ')')) {
                            return (float)bistrs_cvi(str_val);
                        }
                    }
                }
                /* CVS(string) - convert 4-byte string to single precision float */
                else if (strcasecmp_ascii(name, "CVS") == 0) {
                    char *str_val = parse_string_expression(p);
                    if (str_val) {
                        if (parser_match(p, ')')) {
                            return bistrs_cvs(str_val);
                        }
                    }
                }
                /* String functions returning string - not valid in numeric
                 * context. GW-BASIC: type mismatch → SNERR (v137 MEDIUM-2).
                 * Consume args up to the matching ')' first so the parser
                 * position is sane after the error is raised. */
                else if (is_str_func) {
                    int paren_depth = 1;
                    while (paren_depth > 0 && p->pos < p->len) {
                        char ch = p->input[p->pos++];
                        if (ch == '(') paren_depth++;
                        else if (ch == ')') paren_depth--;
                    }
                    if (g_state) gwdata_set_error(2, g_state->current_line, 0);
                    return 0.0f;
                }
            }
        }
        
        /* Check for user-defined function: FNname() - BEFORE array reference check */
        if (strncasecmp_ascii(name, "FN", 2) == 0) {
            struct variable *fn_var = gwdata_find_fn_def(name);
            if (fn_var && fn_var->is_fn_def && fn_var->fn_text_ptr) {
                /* Parse argument if present */
                double arg = 0.0f;
                if (parser_peek(p) == '(') {
                    parser_get(p);  /* consume '(' */
                    arg = parse_expression(p);
                    parser_match(p, ')');  /* consume ')' */
                }
                return gweval_call_fn(name, arg);
            }
        }
        
        /* Check for array reference: A(5) or A(5, 3) */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            /* Parse indices - support multi-dimensional */
            float indices[8];
            int num_indices = 0;
            
            while (parser_peek(p) != ')' && parser_peek(p) != '\0' && num_indices < 8) {
                /* Skip whitespace */
                while (parser_peek(p) == ' ' || parser_peek(p) == '\t') parser_get(p);
                if (parser_peek(p) == ')') break;
                
                /* Parse index expression */
                float index_val = parse_expression(p);
                indices[num_indices++] = index_val;
                
                /* Skip whitespace */
                while (parser_peek(p) == ' ' || parser_peek(p) == '\t') parser_get(p);
                
                if (parser_match(p, ',')) {
                    continue;  /* Parse next index */
                } else if (parser_peek(p) == ')') {
                    break;  /* End of indices */
                } else {
                    /* Unexpected character, break */
                    break;
                }
            }
            
            if (parser_match(p, ')')) {
                if (num_indices == 1) {
                    /* v170 HIGH-5: typed read — VT_DOUBLE arrays
                     * return full precision (was float-truncated) */
                    biptrg_typed_val tv;
                    int tv_idx[1] = { (int)indices[0] };
                    if (biptrg_get_array_element_typed(name, tv_idx, 1, &tv) == 0) {
                        if (tv.type == VT_DOUBLE) { g_valtyp = 8; return tv.d; }
                        if (tv.type == VT_INTEGER) { g_valtyp = 2; return (double)tv.i; }
                        g_valtyp = 4;
                        return tv.d;
                    }
                    if (gwdata_get_error() != 0) return 0.0f;
                    return biptrg_get_array_element(name, (int)indices[0]);
                } else if (num_indices > 1) {
                    int int_indices[8];
                    for (int i = 0; i < num_indices; i++) {
                        int_indices[i] = (int)indices[i];
                        if (int_indices[i] < 0) int_indices[i] = 0;
                    }
                    /* v163/v164 HIGH-2: auto-DIM undeclared arrays
                     * per dimension AND validate the rank of existing
                     * arrays (ERRBS 9 on mismatch) */
                    extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                    biptrg_ensure_array_nd(name, num_indices);
                    if (gwdata_get_error() != 0) return 0.0f;
                    /* v170 HIGH-5: typed multi read (double precision) */
                    biptrg_typed_val tmv;
                    if (biptrg_get_array_element_typed(name, int_indices, num_indices, &tmv) == 0) {
                        if (tmv.type == VT_DOUBLE) { g_valtyp = 8; return tmv.d; }
                        if (tmv.type == VT_INTEGER) { g_valtyp = 2; return (double)tmv.i; }
                        g_valtyp = 4;
                        return tmv.d;
                    }
                    if (gwdata_get_error() != 0) return 0.0f;
                    return biptrg_get_array_element_multi(name, int_indices);
                }
            }
        }
        
        /* Scalar variable — v169: honor type suffixes (% integer,
         * # double, ! or none single); VALTYP tracks the widest type */
        struct variable *v = gwdata_find_variable(name);
        if (v) {
            if (v->type == VT_SINGLE) { g_valtyp = 4; return v->value.f; }
            if (v->type == VT_INTEGER) { g_valtyp = 2; return (double)v->value.i; }
            if (v->type == VT_DOUBLE) { g_valtyp = 8; return v->value.d; }
        }
        
        return 0.0f;
    }
    
    /* Unknown - return 0 */
    return 0.0f;
}

static double parse_term(parser_t *p) {
    double left = parse_factor(p);
    int lt = g_valtyp;              /* v170: type of the FIRST factor */
    
    while (true) {
        char c = parser_peek(p);
        if (c == '*') {
            parser_get(p);
            double right = parse_factor(p);
            if (g_valtyp < lt) g_valtyp = lt;   /* widen */
            left *= right;
        } else if (c == '/') {
            parser_get(p);
            double right = parse_factor(p);
            if (g_valtyp < lt) g_valtyp = lt;
            if (right != 0.0f) {
                left /= right;
            } else {
                /* Division by zero - set error and return 0 */
                if (g_state) {
                    gwdata_set_error(11, g_state->current_line, 0);
                }
            }
        } else if (c == '\\') {
            parser_get(p);
            double right = parse_factor(p);
            g_valtyp = 2;   /* integer division */
            if (g_valtyp < lt) g_valtyp = lt;
            if (right != 0.0f) left = (double)((int)left / (int)right);
        } else if (c == '^') {
            parser_get(p);
            double right = parse_factor(p);
            if (g_valtyp < lt) g_valtyp = lt;
            left = pow(left, right);
        } else {
            break;
        }
    }
    
    return left;
}

static double parse_expression(parser_t *p) {
    double left = parse_term(p);
    int lt = g_valtyp;              /* v170: type after first term */
    
    while (true) {
        char c = parser_peek(p);
        if (c == '+') {
            parser_get(p);
            left += parse_term(p);
            if (g_valtyp < lt) g_valtyp = lt;   /* widen */
        } else if (c == '-') {
            parser_get(p);
            left -= parse_term(p);
            if (g_valtyp < lt) g_valtyp = lt;
        } else {
            break;
        }
    }
    
    /* Comparison operators: <, >, <=, >=, <>, = (return -1 for true, 0 for false) */
    char op[3] = {0, 0, 0};
    char c = parser_peek(p);
    if (c == '<' || c == '>' || c == '=') {
        op[0] = parser_get(p);
        c = parser_peek(p);
        if ((op[0] == '<' && c == '>') || (op[0] == '<' && c == '=') || (op[0] == '>' && c == '=')) {
            op[1] = parser_get(p);
        }
        
        int lt_cmp = g_valtyp;   /* v170: operand type before rhs */
        double right = parse_expression(p);
        if (g_valtyp < lt_cmp) g_valtyp = lt_cmp;
        /* v170 HIGH-3: single comparison rounds BOTH operands
         * (GW-BASIC single compare: 0.1+0.2 = 0.3 is EQ) */
        if (g_valtyp != 8) {
            left = (double)(float)left;
            right = (double)(float)right;
        }
        
        if (strcmp(op, "<>") == 0) left = (left != right) ? -1.0f : 0.0f;
        else if (strcmp(op, "<") == 0) left = (left < right) ? -1.0f : 0.0f;
        else if (strcmp(op, "<=") == 0) left = (left <= right) ? -1.0f : 0.0f;
        else if (strcmp(op, ">") == 0) left = (left > right) ? -1.0f : 0.0f;
        else if (strcmp(op, ">=") == 0) left = (left >= right) ? -1.0f : 0.0f;
        else if (strcmp(op, "=") == 0) left = (left == right) ? -1.0f : 0.0f;
    }
    
    /* v161 HIGH-3: +/- overflow → Overflow (6), not inf.
     * v169 HIGH-1: VALTYP-aware range — single-precision results
     * (no # operand) must stay within float range (GW-BASIC MBF
     * single max ≈ 3.4e38); double results use the double domain. */
    if (!isfinite(left) || (g_valtyp != 8 &&
        (left > 3.402823e38 || left < -3.402823e38))) {
        if (g_state) gwdata_set_error(6, g_state->current_line, 0);
        return 0.0f;
    }
    /* v170 HIGH-3: single-precision results round to float (GW-BASIC
     * computes single ops in MBF single; widening to double without
     * rounding breaks `0.1+0.2=0.3` and `A#=1/3` single semantics) */
    if (g_valtyp != 8) left = (double)(float)left;
    return left;
}

void gweval_init(void) {
    /* No initialization needed */
}

double gweval_eval_numeric(const char *expr) {
    if (!expr) return 0.0f;
    parser_t parser;
    parser_init(&parser, expr);
    return parse_expression(&parser);
}

char *gweval_eval_string(const char *expr) {
    if (!expr) return NULL;
    parser_t parser;
    parser_init(&parser, expr);
    
    /* Try parsing as string expression with concatenation support */
    char *result = parse_string_expression(&parser);
    if (result) {
        return result;
    }
    
    /* No fallback - return NULL for pure numeric expressions so caller can format properly */
    return NULL;
}

void gweval_scan_var(const char **text, int *var_ptr) {
    /* Placeholder - not fully implemented yet */
    (void)text;
    (void)var_ptr;
}

int gweval_frqint(void) {
    return 0;  /* Placeholder */
}

void gweval_chrgt2(const char **text) {
    if (!text || !*text) return;
    /* ASM CHRGT2: load char at pointer, check if < ':' */
    char c = **text;
    if (c == '\0') return;
    /* If char < ':', advance to CHRCON behavior (skip spaces) */
    if (c < ':') {
        /* Skip spaces (CHRCON behavior) */
        while (**text && **text == ' ') {
            (*text)++;
        }
    }
    /* If char >= ':', return without advancing (caller will handle) */
}

void gweval_synchr(const char **text, char expected) {
    if (!text || !*text) return;
    /* ASM SYNCHR: expect exact character match, NO whitespace skipping */
    if (**text != expected) {
        /* ASM: JNZ SYNERR -> SNERR (Syntax Error) */
        /* In C context, we set error and leave pointer at mismatch */
        gwdata_set_error(2, 0, 0);  /* Error 2 = Syntax Error */
        return;
    }
    /* Match: advance pointer */
    (*text)++;
    /* Check if NEXT character (after match) < ':' to decide whether to continue to CHRCON
     * ASM BIMISC.ASM:322: after match, MOV AL,[BX] then CMP AL,':' / JB SYNCON
     * i.e., check the character AFTER the match */
    if (**text && **text < ':') {
        /* Skip spaces (CHRCON behavior) */
        while (**text && **text == ' ') {
            (*text)++;
        }
    }
}

/* DEF FN function call support */
static int g_fn_call_depth = 0;

float gweval_call_fn(const char *fn_name, float arg) {
    if (g_fn_call_depth > 10) {
        return 0.0f;
    }
    
    g_fn_call_depth++;
    if (!g_state) {
        g_fn_call_depth--;
        return 0.0f;
    }
    
    struct variable *fn_var = gwdata_find_fn_def(fn_name);
    if (!fn_var || !fn_var->is_fn_def || !fn_var->fn_text_ptr) {
        g_fn_call_depth--;
        return 0.0f;
    }
    
    struct variable *param_var = gwdata_find_variable(fn_var->fn_param_name);
    float saved_param = 0.0f;
    bool param_exists = false;
    if (param_var && param_var->type == VT_SINGLE) {
        saved_param = param_var->value.f;
        param_exists = true;
    }
    
    if (!param_var) {
        param_var = gwdata_alloc_variable(fn_var->fn_param_name, VT_SINGLE);
    }
    if (param_var) {
        param_var->value.f = arg;
    }
    
    char *fn_body = (char *)(intptr_t)fn_var->fn_text_ptr;
    float result = gweval_eval_numeric(fn_body);
    
    if (param_exists && param_var) {
        param_var->value.f = saved_param;
    }
    
    g_fn_call_depth--;
    return result;
}