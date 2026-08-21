/* GWEVAL - Expression Evaluation */
/* Based on GWEVAL.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

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
        parser_get(p);
        return true;
    }
    return false;
}

/* Forward declarations */
static float parse_expression(parser_t *p);
static char *parse_string_expression(parser_t *p);

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
            
            /* Check for string functions that return numeric: LEN, ASC, VAL */
            bool is_str_numeric = (strcasecmp_ascii(name, "LEN") == 0 ||
                                   strcasecmp_ascii(name, "ASC") == 0 ||
                                   strcasecmp_ascii(name, "VAL") == 0);
            
            /* Check for string functions that return string: LEFT$, RIGHT$, MID$, UCASE$, LCASE$, STR$, CHR$, SPACE$, STRING$ */
            bool is_str_func = (strcasecmp_ascii(name, "LEFT") == 0 || strcasecmp_ascii(name, "LEFT$") == 0 ||
                                strcasecmp_ascii(name, "RIGHT") == 0 || strcasecmp_ascii(name, "RIGHT$") == 0 ||
                                strcasecmp_ascii(name, "MID") == 0 || strcasecmp_ascii(name, "MID$") == 0 ||
                                strcasecmp_ascii(name, "UCASE") == 0 || strcasecmp_ascii(name, "UCASE$") == 0 ||
                                strcasecmp_ascii(name, "LCASE") == 0 || strcasecmp_ascii(name, "LCASE$") == 0 ||
                                strcasecmp_ascii(name, "STR") == 0 || strcasecmp_ascii(name, "STR$") == 0 ||
                                strcasecmp_ascii(name, "CHR") == 0 || strcasecmp_ascii(name, "CHR$") == 0 ||
                                strcasecmp_ascii(name, "SPACE") == 0 || strcasecmp_ascii(name, "SPACE$") == 0 ||
                                strcasecmp_ascii(name, "STRING") == 0 || strcasecmp_ascii(name, "STRING$") == 0);
            
            if (is_math_func || is_tab_spc || is_str_numeric || is_str_func) {
                parser_get(p);  /* consume '(' */
                
                if (is_str_numeric) {
                    /* String functions returning numeric: LEN, ASC, VAL - parse string expression */
                    char *str_val = parse_string_expression(p);
                    
                    if (parser_match(p, ')')) {
                        if (strcasecmp_ascii(name, "LEN") == 0) {
                            return (float)bistrs_len(str_val);
                        }
                        if (strcasecmp_ascii(name, "ASC") == 0) {
                            return (float)bistrs_asc(str_val);
                        }
                        if (strcasecmp_ascii(name, "VAL") == 0) {
                            return (float)bistrs_val(str_val);
                        }
                    }
                    return 0.0f;
                }
                
                if (is_str_func) {
                    /* String functions returning string: handle via gweval_eval_string */
                    p->pos -= 1;  /* Put back '(' so gweval_eval_string can parse it */
                    return 0.0f;  /* String functions in numeric context = 0 */
                }
                
                /* Math functions / TAB / SPC */
                float arg = parse_expression(p);
                
                if (parser_match(p, ')')) {
                    if (strcasecmp_ascii(name, "SIN") == 0) return math1_sin(arg);
                    if (strcasecmp_ascii(name, "COS") == 0) return math1_cos(arg);
                    if (strcasecmp_ascii(name, "TAN") == 0) return math1_tan(arg);
                    if (strcasecmp_ascii(name, "ATN") == 0) return math1_atn(arg);
                    if (strcasecmp_ascii(name, "EXP") == 0) return math1_exp(arg);
                    if (strcasecmp_ascii(name, "LOG") == 0) return math1_log(arg);
                    if (strcasecmp_ascii(name, "SQR") == 0) return math2_sqr(arg);
                    if (strcasecmp_ascii(name, "ABS") == 0) return math2_abs(arg);
                    if (strcasecmp_ascii(name, "SGN") == 0) return math2_sgn(arg);
                    if (strcasecmp_ascii(name, "RND") == 0) return math2_rnd(arg);
                    if (strcasecmp_ascii(name, "INT") == 0) return math2_int(arg);
                    if (strcasecmp_ascii(name, "FIX") == 0) return math2_fix(arg);
                    /* TAB and SPC return the column/space count as numeric (for print positioning) */
                    if (strcasecmp_ascii(name, "TAB") == 0) return arg;
                    if (strcasecmp_ascii(name, "SPC") == 0) return arg;
                }
            }
        }
        
        /* Check for array reference: A(5) or A$(5) or A(5, 3) */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            /* Parse indices - support multi-dimensional */
            int indices[8];
            int num_indices = 0;
            
            while (parser_peek(p) != ')' && parser_peek(p) != '\0' && num_indices < 8) {
                /* Skip whitespace */
                while (parser_peek(p) == ' ' || parser_peek(p) == '\t') parser_get(p);
                if (parser_peek(p) == ')') break;
                
                /* Parse index expression */
                float index_val = parse_expression(p);
                int index = (int)index_val;
                
                /* GW-BASIC: negative index treated as 0 */
                if (index < 0) index = 0;
                
                indices[num_indices] = index;
                num_indices++;
                
                /* Skip whitespace */
                while (parser_peek(p) == ' ' || parser_peek(p) == '\t') parser_get(p);
                
                /* Check for comma or closing paren */
                if (parser_peek(p) == ',') {
                    parser_get(p);  /* consume ',' */
                    continue;
                } else if (parser_peek(p) == ')') {
                    break;
                }
            }
            
            if (parser_match(p, ')')) {
                /* Check if it's a string array (name ends with $) */
                size_t name_len = strlen(name);
                if (name_len > 0 && name[name_len - 1] == '$') {
                    /* String array - return 0 for numeric evaluation */
                    return 0.0f;
                }
                if (num_indices == 1) {
                    return biptrg_get_array_element(name, indices[0]);
                } else {
                    return biptrg_get_array_element_multi(name, indices);
                }
            }
        }
        
        struct variable *v = gwdata_find_variable(name);
        if (v && v->type == VT_SINGLE) {
            return v->value.f;
        }
        return 0.0f;
    }
    
    return 0.0f;
}

/* String expression parser - handles string literals, variables, and string functions.
   Parses a single string expression and stops at comma or closing paren. */
static char *parse_string_expression(parser_t *p) {
    char c = parser_peek(p);
    
    /* String literal in quotes */
    if (c == '\"') {
        parser_get(p);  /* consume opening quote */
        char buf[256];
        size_t i = 0;
        /* Read until closing quote - do NOT skip whitespace inside string */
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
    
    /* Variable or function call */
    if (isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_' || p->input[p->pos] == '$') && i < sizeof(name) - 1) {
            name[i++] = p->input[p->pos++];
        }
        name[i] = '\0';
        
        /* Check for function call: LEFT$(, RIGHT$(, MID$(, UCASE$(, LCASE$(, STR$(, CHR$(, SPACE$(, STRING$( */
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
        if (v && v->type == VT_STRING && v->value.s) {
            return v->value.s;
        }
    }
    
    /* Default: empty string */
    char *result = gwdata_alloc_string(1);
    if (result) result[0] = '\0';
    return result;
}

static float parse_power(parser_t *p) {
    float left = parse_factor(p);
    
    while (parser_match(p, '^')) {
        float right = parse_factor(p);
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
            if (right != 0.0f) left /= right;
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
        return result;
    }
    
    /* Variable or array reference or function call */
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
            
            /* If we get here, it wasn't a recognized string function - 
               rewind to handle as array/variable */
            parser.pos -= 1;  /* Put back '(' */
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
        if (v && v->type == VT_STRING && v->value.s) {
            return v->value.s;
        }
    }
    
    /* Not a recognized string expression - return NULL to let caller try numeric eval */
    return NULL;
}

/* Scan variable name from text */
void gweval_scan_var(const char **text, int *var_ptr) {
    (void)text;
    if (var_ptr) *var_ptr = 0;
}

/* Convert FAC to integer */
int gweval_frqint(void) {
    return 0;
}

/* Skip to next character */
void gweval_chrgt2(const char **text) {
    if (text && *text && **text) (*text)++;
}

/* Check for expected character */
void gweval_synchr(const char **text, char expected) {
    (void)expected;
    if (text && *text && **text == expected) (*text)++;
}