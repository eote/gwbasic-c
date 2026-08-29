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

/* String function helper: parse string expression until comma or closing paren
 * Supports string concatenation with + operator */
static char *parse_string_expression(parser_t *p) {
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
        char *result = gwdata_alloc_string(strlen(buf) + 1);
        if (result) strcpy(result, buf);
        return result;
    }
    
    /* Otherwise parse as general string expression (variable, array, function, concatenation) */
    if (isalpha((unsigned char)c)) {
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
            return v->value.s.ptr;
        }
    }
    
    /* Check for string concatenation with + */
    /* We need to handle cases like "Line " + STR$(I) */
    /* The caller will handle this at a higher level */
    
    /* Default: not a recognized string expression — return NULL so caller can try numeric eval */
    return NULL;
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
            
            /* Check for string functions that return numeric: LEN, ASC, VAL, EOF */
            bool is_str_numeric = (strcasecmp_ascii(name, "LEN") == 0 ||
                                   strcasecmp_ascii(name, "ASC") == 0 ||
                                   strcasecmp_ascii(name, "VAL") == 0 ||
                                   strcasecmp_ascii(name, "EOF") == 0);
            
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
    
    return 0.0f;
}

static float parse_power(parser_t *p) {
    float left = parse_factor(p);
    
    if (parser_match(p, '^')) {
        float right = parse_power(p);  /* Right-associative: parse right side first */
        left = powf(left, right);
    }
    
    return left;
}

static float parse_term(parser_t *p) {
    float left = parse_power(p);
    
    while (true) {
        char c = parser_peek(p);
        if (c == '*') {
            parser_get(p);
            left *= parse_power(p);
        } else if (c == '/') {
            parser_get(p);
            float right = parse_power(p);
            if (right == 0.0f) {
                /* Division by zero - set error flag and return 0 */
                return 0.0f / 0.0f;  /* NaN to signal error */
            }
            left /= right;
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
    
    /* Handle comparison operators: <>, ==, <, >, <=, >= */
    /* Note: we check multi-char operators first */
    char c = parser_peek(p);
    if (c == '<' || c == '>' || c == '=') {
        char op[3] = {0, 0, 0};
        op[0] = parser_get(p);  /* consume first char */
        c = parser_peek(p);
        if ((c == '=' || c == '>') && (op[0] == '<' || op[0] == '>' || op[0] == '=')) {
            op[1] = parser_get(p);  /* consume second char */
        }
        
        float right = parse_expression(p);
        
        if (strcmp(op, "<>") == 0) {
            left = (left != right) ? -1.0f : 0.0f;  /* GW-BASIC true = -1, false = 0 */
        } else if (strcmp(op, "==") == 0 || strcmp(op, "=") == 0) {
            left = (left == right) ? -1.0f : 0.0f;
        } else if (strcmp(op, "<") == 0) {
            left = (left < right) ? -1.0f : 0.0f;
        } else if (strcmp(op, ">") == 0) {
            left = (left > right) ? -1.0f : 0.0f;
        } else if (strcmp(op, "<=") == 0) {
            left = (left <= right) ? -1.0f : 0.0f;
        } else if (strcmp(op, ">=") == 0) {
            left = (left >= right) ? -1.0f : 0.0f;
        }
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

/* String expression evaluation for PRINT etc. */
char *gweval_eval_string(const char *expr) {
    if (!expr) return NULL;
    
    parser_t parser;
    parser_init(&parser, expr);
    
    /* Skip leading whitespace */
    char c = parser_peek(&parser);
    
    /* String literal in quotes */
    if (c == '\"') {
        parser_get(&parser);  /* consume opening quote */
        char buf[256];
        size_t i = 0;
        while (parser.pos < parser.len && parser.input[parser.pos] != '\"' && i < sizeof(buf) - 1) {
            buf[i++] = parser.input[parser.pos++];
        }
        if (parser.pos < parser.len && parser.input[parser.pos] == '\"') {
            parser.pos++;  /* consume closing quote */
        }
        buf[i] = '\0';
        char *result = gwdata_alloc_string(strlen(buf) + 1);
        if (result) strcpy(result, buf);
        
        /* Check for concatenation after string literal */
        /* Skip whitespace */
        while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
        /* Check for + operator for concatenation */
        while (parser.pos < parser.len && parser.input[parser.pos] == '+') {
            parser.pos++;  /* consume '+' */
            /* Skip whitespace */
            while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
            /* Parse next string expression */
            char *next_str = parse_string_expression(&parser);
            if (next_str) {
                /* Concatenate result + next_str */
                size_t len1 = strlen(result);
                size_t len2 = strlen(next_str);
                char *concat = gwdata_alloc_string(len1 + len2 + 1);
                if (concat) {
                    strcpy(concat, result);
                    strcat(concat, next_str);
                    result = concat;
                }
            } else {
                /* Try numeric expression then convert to string */
                float num_val = parse_expression(&parser);
                char num_buf[32];
                sprintf(num_buf, "%g", num_val);
                size_t len1 = strlen(result);
                size_t len2 = strlen(num_buf);
                char *concat = gwdata_alloc_string(len1 + len2 + 1);
                if (concat) {
                    strcpy(concat, result);
                    strcat(concat, num_buf);
                    result = concat;
                }
            }
            /* Skip whitespace */
            while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
        }
        return result;
    }
    
    /* Variable or array reference or function call or concatenation */
    if (isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        while (parser.pos < parser.len && (isalnum((unsigned char)parser.input[parser.pos]) || parser.input[parser.pos] == '_' || parser.input[parser.pos] == '$') && i < sizeof(name) - 1) {
            name[i++] = parser.input[parser.pos++];
        }
        name[i] = '\0';
        
        /* Check for string function call: LEFT$(, RIGHT$(, MID$(, UCASE$(, LCASE$(, STR$(, CHR$(, SPACE$(, STRING$( */
        if (parser_peek(&parser) == '(') {
            parser_get(&parser);  /* consume '(' */
            
            if (strcasecmp_ascii(name, "LEFT") == 0 || strcasecmp_ascii(name, "LEFT$") == 0) {
                /* LEFT$(str, n) */
                char *str_val = parse_string_expression(&parser);
                if (str_val) {
                    if (parser_match(&parser, ',')) {
                        float arg2 = parse_expression(&parser);
                        if (parser_match(&parser, ')')) {
                            return bistrs_left(str_val, (int)arg2);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "RIGHT") == 0 || strcasecmp_ascii(name, "RIGHT$") == 0) {
                /* RIGHT$(str, n) */
                char *str_val = parse_string_expression(&parser);
                if (str_val) {
                    if (parser_match(&parser, ',')) {
                        float arg2 = parse_expression(&parser);
                        if (parser_match(&parser, ')')) {
                            return bistrs_right(str_val, (int)arg2);
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "MID") == 0 || strcasecmp_ascii(name, "MID$") == 0) {
                /* MID$(str, start, len) */
                char *str_val = parse_string_expression(&parser);
                if (str_val) {
                    if (parser_match(&parser, ',')) {
                        float arg2 = parse_expression(&parser);
                        if (parser_match(&parser, ',')) {
                            float arg3 = parse_expression(&parser);
                            if (parser_match(&parser, ')')) {
                                return bistrs_mid(str_val, (int)arg2, (int)arg3);
                            }
                        }
                    }
                }
            }
            else if (strcasecmp_ascii(name, "UCASE") == 0 || strcasecmp_ascii(name, "UCASE$") == 0) {
                /* UCASE$(str) */
                char *str_val = parse_string_expression(&parser);
                if (str_val) {
                    if (parser_match(&parser, ')')) {
                        return bistrs_ucase(str_val);
                    }
                }
            }
            else if (strcasecmp_ascii(name, "LCASE") == 0 || strcasecmp_ascii(name, "LCASE$") == 0) {
                /* LCASE$(str) */
                char *str_val = parse_string_expression(&parser);
                if (str_val) {
                    if (parser_match(&parser, ')')) {
                        return bistrs_lcase(str_val);
                    }
                }
            }
            else if (strcasecmp_ascii(name, "STR") == 0 || strcasecmp_ascii(name, "STR$") == 0) {
                /* STR$(num) */
                float arg = parse_expression(&parser);
                if (parser_match(&parser, ')')) {
                    return bistrs_str(arg);
                }
            }
            else if (strcasecmp_ascii(name, "CHR") == 0 || strcasecmp_ascii(name, "CHR$") == 0) {
                /* CHR$(num) */
                float arg = parse_expression(&parser);
                if (parser_match(&parser, ')')) {
                    return bistrs_chr((int)arg);
                }
            }
            else if (strcasecmp_ascii(name, "SPACE") == 0 || strcasecmp_ascii(name, "SPACE$") == 0) {
                /* SPACE$(num) */
                float arg = parse_expression(&parser);
                if (parser_match(&parser, ')')) {
                    return bistrs_space((int)arg);
                }
            }
            else if (strcasecmp_ascii(name, "STRING") == 0 || strcasecmp_ascii(name, "STRING$") == 0) {
                /* STRING$(num, char) */
                float arg1 = parse_expression(&parser);
                if (parser_match(&parser, ',')) {
                    float arg2 = parse_expression(&parser);
                    if (parser_match(&parser, ')')) {
                        return bistrs_string((int)arg1, (char)arg2);
                    }
                }
            }
            /* MKI$(num) - convert integer to 2-byte string */
            else if (strcasecmp_ascii(name, "MKI") == 0 || strcasecmp_ascii(name, "MKI$") == 0) {
                float arg = parse_expression(&parser);
                if (parser_match(&parser, ')')) {
                    return bistrs_mki((int)arg);
                }
            }
            /* MKS$(num) - convert float to 4-byte string */
            else if (strcasecmp_ascii(name, "MKS") == 0 || strcasecmp_ascii(name, "MKS$") == 0) {
                float arg = parse_expression(&parser);
                if (parser_match(&parser, ')')) {
                    return bistrs_mks(arg);
                }
            }
            
            /* Not a recognized string function - put back '(' to check for array reference */
            parser.pos--;  /* Put back '(' */
        }
        
        /* Check for array reference: A$(5) */
        if (parser_peek(&parser) == '(') {
            parser_get(&parser);  /* consume '(' */
            
            float index_val = parse_expression(&parser);
            int index = (int)index_val;
            if (index < 0) index = 0;
            
            if (parser_match(&parser, ')')) {
                return biptrg_get_array_string(name, index);
            }
        }
        
        /* Scalar string variable */
        struct variable *v = gwdata_find_variable(name);
        if (v && v->type == VT_STRING && v->value.s.ptr && !v->is_array) {
            /* For FIELD variables, the string may not be NUL-terminated.
             * Create a proper NUL-terminated copy using the stored length. */
            char *result = gwdata_alloc_string(v->value.s.len + 1);
            if (result) {
                memcpy(result, v->value.s.ptr, v->value.s.len);
                result[v->value.s.len] = '\0';
            }
            /* Check for concatenation after variable */
            /* Skip whitespace */
            while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
            /* Check for + operator for concatenation */
            while (parser.pos < parser.len && parser.input[parser.pos] == '+') {
                parser.pos++;  /* consume '+' */
                /* Skip whitespace */
                while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
                /* Parse next string expression */
                char *next_str = parse_string_expression(&parser);
                if (next_str) {
                    /* Concatenate result + next_str */
                    size_t len1 = strlen(result);
                    size_t len2 = strlen(next_str);
                    char *concat = gwdata_alloc_string(len1 + len2 + 1);
                    if (concat) {
                        strcpy(concat, result);
                        strcat(concat, next_str);
                        result = concat;
                    }
                } else {
                    /* Try numeric expression then convert to string */
                    float num_val = parse_expression(&parser);
                    char num_buf[32];
                    sprintf(num_buf, "%g", num_val);
                    size_t len1 = strlen(result);
                    size_t len2 = strlen(num_buf);
                    char *concat = gwdata_alloc_string(len1 + len2 + 1);
                    if (concat) {
                        strcpy(concat, result);
                        strcat(concat, num_buf);
                        result = concat;
                    }
                }
                /* Skip whitespace */
                while (parser.pos < parser.len && isspace((unsigned char)parser.input[parser.pos])) parser.pos++;
            }
            return result;
        }
    }
    
    /* Default: not a recognized string expression — return NULL so caller can try numeric eval */
    return NULL;
}