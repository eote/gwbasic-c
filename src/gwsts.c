/* GWSTS - Statement Execution Engine */
/* Based on GWSTS.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Forward declaration */
extern int biptrg_dim_array(const char *name, int size, var_type_t type);
extern int biptrg_set_array_element(const char *name, int index, float val);

void gwsts_init(void) {
    /* No initialization needed */
}

/* Simple statement parser and executor */
static char *strdup_trim(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = (char *)s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    size_t len = end - s + 1;
    char *result = (char *)gwram_alloc(len + 1);
    if (result) {
        memcpy(result, s, len);
        result[len] = '\0';
    }
    return result;
}

static bool starts_with(const char *s, const char *prefix) {
    return strncasecmp(s, prefix, strlen(prefix)) == 0;
}

/* Parse variable name with optional array index: A or A(5) */
static bool parse_var_ref(const char *str, char **var_name, int *index) {
    *var_name = NULL;
    *index = -1;
    
    /* Find opening parenthesis */
    const char *paren = strchr(str, '(');
    if (!paren) {
        /* Simple variable */
        *var_name = (char *)gwram_alloc(strlen(str) + 1);
        if (*var_name) strcpy(*var_name, str);
        return true;
    }
    
    /* Array reference: A(5) */
    size_t name_len = paren - str;
    *var_name = (char *)gwram_alloc(name_len + 1);
    if (!*var_name) return false;
    memcpy(*var_name, str, name_len);
    (*var_name)[name_len] = '\0';
    
    /* Parse index */
    const char *index_str = paren + 1;
    const char *close_paren = strchr(index_str, ')');
    if (!close_paren) return false;
    
    char *index_buf = (char *)gwram_alloc(close_paren - index_str + 1);
    if (!index_buf) return false;
    memcpy(index_buf, index_str, close_paren - index_str);
    index_buf[close_paren - index_str] = '\0';
    
    *index = atoi(index_buf);
    gwram_free(index_buf);
    return true;
}

int gwsts_exec_statement(const char *stmt) {
    if (!stmt || !g_state) return ERROR;
    
    char *trimmed = strdup_trim(stmt);
    if (!trimmed) return ERROR;
    
    int result = ERROR;
    
    /* DIM statement */
    if (starts_with(trimmed, "DIM ")) {
        char *rest = trimmed + 4;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Parse: DIM A(10) or DIM A(10), B(5) */
        char *token = rest;
        while (*token) {
            /* Find variable name */
            char *var_name = NULL;
            int size = 0;
            
            char *paren = strchr(token, '(');
            if (paren) {
                size_t name_len = paren - token;
                var_name = (char *)gwram_alloc(name_len + 1);
                if (var_name) {
                    memcpy(var_name, token, name_len);
                    var_name[name_len] = '\0';
                    
                    char *close_paren = strchr(paren + 1, ')');
                    if (close_paren) {
                        char *size_str = (char *)gwram_alloc(close_paren - paren);
                        if (size_str) {
                            memcpy(size_str, paren + 1, close_paren - paren - 1);
                            size_str[close_paren - paren - 1] = '\0';
                            size = atoi(size_str);
                            gwram_free(size_str);
                        }
                    }
                }
            }
            
            if (var_name && size > 0) {
                biptrg_dim_array(var_name, size, VT_SINGLE);
                result = SUCCESS;
                gwram_free(var_name);
            }
            
            /* Move to next token after comma */
            token = strchr(token, ',');
            if (token) token++;
            else break;
        }
    }
    /* LET statement (optional LET keyword) */
    else if (starts_with(trimmed, "LET ")) {
        char *eq = strchr(trimmed + 4, '=');
        if (eq) {
            *eq = '\0';
            char *var_name = strdup_trim(trimmed + 4);
            char *expr = strdup_trim(eq + 1);
            
            int array_index = -1;
            char *base_name = NULL;
            parse_var_ref(var_name, &base_name, &array_index);
            
            if (base_name && expr) {
                if (array_index >= 0) {
                    /* Array element assignment */
                    float val = gweval_eval_numeric(expr);
                    result = biptrg_set_array_element(base_name, array_index, val);
                } else {
                    /* Simple variable assignment */
                    struct variable *v = gwdata_alloc_variable(base_name, VT_SINGLE);
                    if (v) {
                        v->value.f = gweval_eval_numeric(expr);
                        result = SUCCESS;
                    }
                }
            }
            if (base_name) gwram_free(base_name);
            if (var_name) gwram_free(var_name);
            if (expr) gwram_free(expr);
        }
    }
    /* PRINT statement */
    else if (starts_with(trimmed, "PRINT ")) {
        char *expr = strdup_trim(trimmed + 6);
        if (expr) {
            float val = gweval_eval_numeric(expr);
            printf("%g\n", val);
            gwram_free(expr);
            result = SUCCESS;
        }
    }
    /* IF statement (simplified: IF <expr> THEN <stmt>) */
    else if (starts_with(trimmed, "IF ")) {
        char *then_pos = strstr(trimmed + 3, " THEN ");
        if (then_pos) {
            *then_pos = '\0';
            char *cond_expr = strdup_trim(trimmed + 3);
            char *then_stmt = strdup_trim(then_pos + 6);
            if (cond_expr && then_stmt) {
                float cond_val = gweval_eval_numeric(cond_expr);
                if (cond_val != 0.0f) {
                    result = gwsts_exec_statement(then_stmt);
                } else {
                    result = SUCCESS;
                }
                gwram_free(cond_expr);
                gwram_free(then_stmt);
            }
        }
    }
    /* Direct assignment (A = 5 or A(5) = 42) */
    else {
        char *eq = strchr(trimmed, '=');
        if (eq) {
            *eq = '\0';
            char *var_name = strdup_trim(trimmed);
            char *expr = strdup_trim(eq + 1);
            
            int array_index = -1;
            char *base_name = NULL;
            parse_var_ref(var_name, &base_name, &array_index);
            
            if (base_name && expr) {
                if (array_index >= 0) {
                    /* Array element assignment */
                    float val = gweval_eval_numeric(expr);
                    result = biptrg_set_array_element(base_name, array_index, val);
                } else {
                    /* Simple variable assignment */
                    struct variable *v = gwdata_alloc_variable(base_name, VT_SINGLE);
                    if (v) {
                        v->value.f = gweval_eval_numeric(expr);
                        result = SUCCESS;
                    }
                }
            }
            if (base_name) gwram_free(base_name);
            if (var_name) gwram_free(var_name);
            if (expr) gwram_free(expr);
        }
    }
    
    gwram_free(trimmed);
    return result;
}