/* GWSTS - Statement Execution Engine */
/* Based on GWSTS.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

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

int gwsts_exec_statement(const char *stmt) {
    if (!stmt || !g_state) return ERROR;
    
    char *trimmed = strdup_trim(stmt);
    if (!trimmed) return ERROR;
    
    int result = ERROR;
    
    /* LET statement (optional LET keyword) */
    if (starts_with(trimmed, "LET ")) {
        char *eq = strchr(trimmed + 4, '=');
        if (eq) {
            *eq = '\0';
            char *var_name = strdup_trim(trimmed + 4);
            char *expr = strdup_trim(eq + 1);
            if (var_name && expr) {
                struct variable *v = gwdata_alloc_variable(var_name, VT_SINGLE);
                if (v) {
                    v->value.f = gweval_eval_numeric(expr);
                    result = SUCCESS;
                }
            }
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
    /* Direct assignment (A = 5) */
    else {
        char *eq = strchr(trimmed, '=');
        if (eq) {
            *eq = '\0';
            char *var_name = strdup_trim(trimmed);
            char *expr = strdup_trim(eq + 1);
            if (var_name && expr) {
                struct variable *v = gwdata_alloc_variable(var_name, VT_SINGLE);
                if (v) {
                    v->value.f = gweval_eval_numeric(expr);
                    result = SUCCESS;
                }
            }
            if (var_name) gwram_free(var_name);
            if (expr) gwram_free(expr);
        }
    }
    
    gwram_free(trimmed);
    return result;
}