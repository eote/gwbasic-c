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
static float parse_expression(parser_t *p);
static char *parse_string_expression(parser_t *p);

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
        result = gwdata_alloc_string(strlen(buf) + 1);
        if (result) strcpy(result, buf);
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
                /* LEFT$(str, n) */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ')')) {
                            return bistrs_left(str_val, (int)arg2);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "RIGHT") == 0 || strcasecmp_ascii(name, "RIGHT$") == 0) {
                /* RIGHT$(str, n) */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ')')) {
                            return bistrs_right(str_val, (int)arg2);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "MID") == 0 || strcasecmp_ascii(name, "MID$") == 0) {
                /* MID$(str, start, len) */
                char *str_val = parse_string_expression(p);
                if (str_val) {
                    if (parser_match(p, ',')) {
                        float arg2 = parse_expression(p);
                        if (parser_match(p, ',')) {
                            float arg3 = parse_expression(p);
                            if (parser_match(p, ')')) {
                                return bistrs_mid(str_val, (int)arg2, (int)arg3);
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
                /* STR$(num) */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_str(arg);
                }
            }
            else if (strcasecmp_ascii(name, "CHR") == 0 || strcasecmp_ascii(name, "CHR$") == 0) {
                /* CHR$(num) */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_chr((int)arg);
                }
            }
            else if (strcasecmp_ascii(name, "SPACE") == 0 || strcasecmp_ascii(name, "SPACE$") == 0) {
                /* SPACE$(num) */
                float arg = parse_expression(p);
                if (parser_match(p, ')')) {
                    return bistrs_space((int)arg);
                }
            }
            else if (strcasecmp_ascii(name, "STRING") == 0 || strcasecmp_ascii(name, "STRING$") == 0) {
                /* STRING$(num, char) */
                float arg1 = parse_expression(p);
                if (parser_match(p, ',')) {
                    float arg2 = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return bistrs_string((int)arg1, (char)arg2);
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
        
        /* Check for array reference: A$(5) */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            float index_val = parse_expression(p);
            int index = (int)index_val;
            if (index < 0) index = 0;
            
            if (parser_match(p, ')')) {
                return biptrg_get_array_string(name, index);
            }
        }
        
        /* Scalar string variable */
        struct variable *v = gwdata_find_variable(name);
        if (v && v->type == VT_STRING && v->value.s.ptr) {
            if (v->is_field) {
                /* Field variables may not be null-terminated - create a copy */
                char *result = gwdata_alloc_string(v->value.s.len + 1);
                if (result) {
                    memcpy(result, v->value.s.ptr, v->value.s.len);
                    result[v->value.s.len] = '\0';
                }
                return result;
            }
            return v->value.s.ptr;
        }
    }
    
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

static float parse_factor(parser_t *p) {
    char c = parser_peek(p);
    
    if (c == '(') {
        parser_get(p); /* consume '(' */
        float result = parse_expression(p);
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
                float val = parse_factor(p);
                return (val == 0.0f) ? -1.0f : 0.0f;  /* GW-BASIC: NOT 0 = -1, NOT non-zero = 0 */
            }
        }
    }
    
    /* Number */
    if (isdigit((unsigned char)c) || c == '.') {
        char buf[64];
        size_t i = 0;
        while (p->pos < p->len && (isdigit((unsigned char)p->input[p->pos]) || p->input[p->pos] == '.') && i < sizeof(buf) - 1) {
            buf[i++] = p->input[p->pos++];
        }
        buf[i] = '\0';
        return (float)atof(buf);
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
        while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_' || p->input[p->pos] == '$') && i < sizeof(name) - 1) {
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
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return sinf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "COS") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return cosf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "TAN") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return tanf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "ATN") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return atanf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "EXP") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return expf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "LOG") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return logf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "SQR") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return sqrtf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "ABS") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return fabsf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "SGN") == 0) {
                    float arg = parse_expression(p);
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
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return floorf(arg);
                    }
                }
                else if (strcasecmp_ascii(name, "FIX") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return truncf(arg);
                    }
                }
                /* TAB/SPC - PRINT functions that return numeric */
                else if (strcasecmp_ascii(name, "TAB") == 0) {
                    float arg = parse_expression(p);
                    if (parser_match(p, ')')) {
                        return arg;
                    }
                }
                else if (strcasecmp_ascii(name, "SPC") == 0) {
                    float arg = parse_expression(p);
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
                    float arg = parse_expression(p);
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
                /* String functions returning string - not valid in numeric context, return 0 */
                else if (is_str_func) {
                    if (parser_match(p, ')')) {
                        return 0.0f;
                    }
                }
            }
        }
        
        /* Check for user-defined function: FNname() - BEFORE array reference check */
        if (strncasecmp_ascii(name, "FN", 2) == 0) {
            struct variable *fn_var = gwdata_find_fn_def(name);
            if (fn_var && fn_var->is_fn_def && fn_var->fn_text_ptr) {
                /* Parse argument if present */
                float arg = 0.0f;
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
                    return biptrg_get_array_element(name, (int)indices[0]);
                } else if (num_indices > 1) {
                    int int_indices[8];
                    for (int i = 0; i < num_indices; i++) {
                        int_indices[i] = (int)indices[i];
                        if (int_indices[i] < 0) int_indices[i] = 0;
                    }
                    return biptrg_get_array_element_multi(name, int_indices);
                }
            }
        }
        
        /* Scalar variable */
        struct variable *v = gwdata_find_variable(name);
        if (v && v->type == VT_SINGLE) {
            return v->value.f;
        }
        
        return 0.0f;
    }
    
    /* Unknown - return 0 */
    return 0.0f;
}

static float parse_term(parser_t *p) {
    float left = parse_factor(p);
    
    while (true) {
        char c = parser_peek(p);
        if (c == '*') {
            parser_get(p);
            float right = parse_factor(p);
            left *= right;
        } else if (c == '/') {
            parser_get(p);
            float right = parse_factor(p);
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
            float right = parse_factor(p);
            if (right != 0.0f) left = (float)((int)left / (int)right);
        } else if (c == '^') {
            parser_get(p);
            float right = parse_factor(p);
            left = powf(left, right);
        } else {
            break;
        }
    }
    
    return left;
}

static float parse_expression(parser_t *p) {
    float left = parse_term(p);
    
    while (true) {
        char c = parser_peek(p);
        if (c == '+') {
            parser_get(p);
            left += parse_term(p);
        } else if (c == '-') {
            parser_get(p);
            left -= parse_term(p);
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
        
        float right = parse_expression(p);
        
        if (strcmp(op, "<>") == 0) left = (left != right) ? -1.0f : 0.0f;
        else if (strcmp(op, "<") == 0) left = (left < right) ? -1.0f : 0.0f;
        else if (strcmp(op, "<=") == 0) left = (left <= right) ? -1.0f : 0.0f;
        else if (strcmp(op, ">") == 0) left = (left > right) ? -1.0f : 0.0f;
        else if (strcmp(op, ">=") == 0) left = (left >= right) ? -1.0f : 0.0f;
        else if (strcmp(op, "=") == 0) left = (left == right) ? -1.0f : 0.0f;
    }
    
    return left;
}

void gweval_init(void) {
    /* No initialization needed */
}

float gweval_eval_numeric(const char *expr) {
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