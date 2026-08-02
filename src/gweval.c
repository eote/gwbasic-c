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

static float parse_expression(parser_t *p);

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
    
    /* Number or variable or function */
    if (isdigit((unsigned char)c) || c == '.') {
        char buf[64];
        size_t i = 0;
        while (p->pos < p->len && (isdigit((unsigned char)p->input[p->pos]) || p->input[p->pos] == '.') && i < sizeof(buf) - 1) {
            buf[i++] = p->input[p->pos++];
        }
        buf[i] = '\0';
        return (float)atof(buf);
    }
    
    if (isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        while (p->pos < p->len && (isalnum((unsigned char)p->input[p->pos]) || p->input[p->pos] == '_') && i < sizeof(name) - 1) {
            name[i++] = p->input[p->pos++];
        }
        name[i] = '\0';
        
        /* Check for function call: SIN(, COS(, etc. */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            /* Parse argument expression */
            float arg = parse_expression(p);
            
            if (parser_match(p, ')')) {
                        /* Math functions */
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
                    }
        }
        
        /* Check for array reference: A(5) or A$(5) */
        if (parser_peek(p) == '(') {
            parser_get(p);  /* consume '(' */
            
            /* Parse index expression */
            float index_val = parse_expression(p);
            int index = (int)index_val;
            
            /* GW-BASIC: negative index treated as 0 */
            if (index < 0) index = 0;
            
            if (parser_match(p, ')')) {
                /* Check if it's a string array (name ends with $) */
                size_t name_len = strlen(name);
                if (name_len > 0 && name[name_len - 1] == '$') {
                    /* String array - return 0 for numeric evaluation */
                    return 0.0f;
                }
                return biptrg_get_array_element(name, index);
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
    if (c == '"') {
        parser_get(&parser);  /* consume opening quote */
        char buf[256];
        size_t i = 0;
        while (parser.pos < parser.len && parser.input[parser.pos] != '"' && i < sizeof(buf) - 1) {
            buf[i++] = parser.input[parser.pos++];
        }
        if (parser.pos < parser.len && parser.input[parser.pos] == '"') {
            parser.pos++;  /* consume closing quote */
        }
        buf[i] = '\0';
        char *result = gwdata_alloc_string(strlen(buf) + 1);
        if (result) strcpy(result, buf);
        return result;
    }
    
    /* Variable or array reference */
    if (isalpha((unsigned char)c)) {
        char name[41];
        size_t i = 0;
        while (parser.pos < parser.len && (isalnum((unsigned char)parser.input[parser.pos]) || parser.input[parser.pos] == '_' || parser.input[parser.pos] == '$') && i < sizeof(name) - 1) {
            name[i++] = parser.input[parser.pos++];
        }
        name[i] = '\0';
        
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
    
    return NULL;
}