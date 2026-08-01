/* GWSTS - Statement Execution Engine */
/* Based on GWSTS.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Forward declarations */
extern int biptrg_dim_array(const char *name, int size, var_type_t type);
extern int biptrg_set_array_element(const char *name, int index, float val);
extern int dskcom_open(const char *filename, int filenum, int mode, int reclen);
extern int dskcom_close(int filenum);

/* Forward declarations for static functions */
static char *strdup_trim(const char *s);
static bool starts_with(const char *s, const char *prefix);
static bool parse_var_ref(const char *str, char **var_name, int *index);
static int assign_value(const char *var_ref, const char *expr);

void gwsts_init(void) {
    /* No initialization needed */
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
            /* Skip leading whitespace */
            while (*token && isspace((unsigned char)*token)) token++;
            if (!*token) break;
            
            /* Find variable name */
            char *var_name = NULL;
            int size = 0;
            var_type_t type = VT_SINGLE;  /* Moved outside inner block */
            
            char *paren = strchr(token, '(');
            if (paren) {
                size_t name_len = paren - token;
                var_name = (char *)gwram_alloc(name_len + 1);
                if (var_name) {
                    memcpy(var_name, token, name_len);
                    var_name[name_len] = '\0';
                
                    /* Trim trailing whitespace from var_name */
                    char *end = var_name + strlen(var_name) - 1;
                    while (end > var_name && isspace((unsigned char)*end)) end--;
                    *(end + 1) = '\0';
                
                    /* Determine type from $ suffix */
                    if (strlen(var_name) > 0 && var_name[strlen(var_name) - 1] == '$') {
                        type = VT_STRING;
                    }
                
                    char *close_paren = strchr(paren + 1, ')');
                    if (close_paren) {
                        char *size_str = (char *)gwram_alloc(close_paren - paren);
                        if (size_str) {
                            memcpy(size_str, paren + 1, close_paren - paren - 1);
                            size_str[close_paren - paren - 1] = '\0';
                            /* Use expression evaluation instead of atoi */
                            size = (int)gweval_eval_numeric(size_str);
                            gwram_free(size_str);
                        }
                    }
                }
            }
            
            if (var_name && size > 0) {
                biptrg_dim_array(var_name, size, type);
                result = SUCCESS;
                gwram_free(var_name);
            }
            
            /* Move to next token after comma */
            token = strchr(token, ',');
            if (token) {
                token++;
                continue;  /* Continue loop to parse next array */
            }
            break;
        }
    }
    /* LET statement (optional LET keyword) */
    else if (starts_with(trimmed, "LET ")) {
        char *eq = strchr(trimmed + 4, '=');
        if (eq) {
            *eq = '\0';
            char *var_ref = strdup_trim(trimmed + 4);
            char *expr = strdup_trim(eq + 1);
            
            result = assign_value(var_ref, expr);
            
            if (var_ref) gwram_free(var_ref);
            if (expr) gwram_free(expr);
        }
    }
    /* PRINT statement */
    else if (starts_with(trimmed, "PRINT ")) {
        /* Check for PRINT #filenum, (file output) */
        if (trimmed[6] == '#') {
            char *rest = trimmed + 7;
            int filenum = (int)gweval_eval_numeric(rest);
            
            /* Find comma */
            char *comma = strchr(rest, ',');
            if (comma && filenum > 0) {
                *comma = '\0';
                char *expr = strdup_trim(comma + 1);
                if (expr) {
                    char *str_val = gweval_eval_string(expr);
                    if (str_val) {
                        result = dskcom_print(filenum, str_val);
                    } else {
                        float val = gweval_eval_numeric(expr);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%g", val);
                        result = dskcom_print(filenum, buf);
                    }
                    gwram_free(expr);
                }
            }
        } else {
            /* Regular PRINT to console */
            char *expr = strdup_trim(trimmed + 6);
            if (expr) {
                /* Try string evaluation first, then numeric */
                char *str_val = gweval_eval_string(expr);
                if (str_val) {
                    printf("%s\n", str_val);
                } else {
                    float val = gweval_eval_numeric(expr);
                    printf("%g\n", val);
                }
                gwram_free(expr);
                result = SUCCESS;
            }
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
    /* OPEN statement */
    else if (starts_with(trimmed, "OPEN ")) {
        /* Parse: OPEN "filename" FOR INPUT/OUTPUT/APPEND/RANDOM AS #filenum [LEN=reclen] */
        char *rest = trimmed + 5;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Extract filename (quoted string) */
        if (*rest == '"') {
            rest++;
            char *filename_end = strchr(rest, '"');
            if (filename_end) {
                size_t fn_len = filename_end - rest;
                char *filename = (char *)gwram_alloc(fn_len + 1);
                if (filename) {
                    memcpy(filename, rest, fn_len);
                    filename[fn_len] = '\0';
                    
                    rest = filename_end + 1;
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    
                    /* Parse FOR mode */
                    int mode = FM_INPUT;
                    if (starts_with(rest, "FOR INPUT")) {
                        mode = FM_INPUT;
                        rest += 9;
                    } else if (starts_with(rest, "FOR OUTPUT")) {
                        mode = FM_OUTPUT;
                        rest += 10;
                    } else if (starts_with(rest, "FOR APPEND")) {
                        mode = FM_APPEND;
                        rest += 10;
                    } else if (starts_with(rest, "FOR RANDOM")) {
                        mode = FM_RANDOM;
                        rest += 10;
                    }
                    
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    
                    /* Parse AS #filenum */
                    int filenum = 0;
                    if (starts_with(rest, "AS #")) {
                        rest += 4;
                        filenum = (int)gweval_eval_numeric(rest);
                    }
                    
                    /* Parse optional LEN=reclen */
                    int reclen = 0;
                    char *len_pos = strstr(rest, "LEN=");
                    if (len_pos) {
                        reclen = (int)gweval_eval_numeric(len_pos + 4);
                    }
                    
                    if (filenum > 0) {
                        result = dskcom_open(filename, filenum, mode, reclen);
                    }
                    gwram_free(filename);
                }
            }
        }
    }
    /* CLOSE statement */
    else if (starts_with(trimmed, "CLOSE")) {
        char *rest = trimmed + 5;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '\0' || *rest == '#') {
            if (*rest == '#') rest++;
            int filenum = (int)gweval_eval_numeric(rest);
            if (filenum > 0) {
                result = dskcom_close(filenum);
            } else {
                /* CLOSE without filenum - close all */
                /* For simplicity, just return success */
                result = SUCCESS;
            }
        }
    }
    /* LINE INPUT statement: LINE INPUT #filenum, var$ or LINE INPUT var$ */
    else if (starts_with(trimmed, "LINE INPUT ")) {
        char *rest = trimmed + 11;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        int filenum = 0;
        /* Check for #filenum */
        if (*rest == '#') {
            rest++;
            filenum = (int)gweval_eval_numeric(rest);
            char *comma = strchr(rest, ',');
            if (comma) rest = comma + 1;
        }
        
        char *var_ref = strdup_trim(rest);
        if (var_ref) {
            if (filenum > 0) {
                /* LINE INPUT #filenum, var$ */
                char buf[256];
                int rc = dskcom_line_input(filenum, buf, sizeof(buf));
                if (rc == SUCCESS) {
                    /* Assign to variable */
                    char *expr = (char *)gwram_alloc(strlen(buf) + 3);
                    if (expr) {
                        expr[0] = '"';
                        strcpy(expr + 1, buf);
                        strcat(expr, "\"");
                        result = assign_value(var_ref, expr);
                        gwram_free(expr);
                    }
                }
            } else {
                /* LINE INPUT var$ - from console (not implemented yet) */
                result = SUCCESS;
            }
            gwram_free(var_ref);
        }
    }
    /* Direct assignment (A = 5 or A(5) = 42) */
    else {
        char *eq = strchr(trimmed, '=');
        if (eq) {
            *eq = '\0';
            char *var_ref = strdup_trim(trimmed);
            char *expr = strdup_trim(eq + 1);
            
            result = assign_value(var_ref, expr);
            
            if (var_ref) gwram_free(var_ref);
            if (expr) gwram_free(expr);
        }
    }
    
    gwram_free(trimmed);
    return result;
}

/* Common assignment function for both LET and direct assignment */
static int assign_value(const char *var_ref, const char *expr) {
    if (!var_ref || !expr) return ERROR;
    
    char *base_name = NULL;
    int array_index = -1;
    parse_var_ref(var_ref, &base_name, &array_index);
    
    int result = ERROR;
    if (base_name && expr) {
        if (array_index >= 0) {
            /* Array element assignment */
            /* Check if this is a string array (name ends with $) */
            size_t name_len = strlen(base_name);
            bool is_string_array = (name_len > 0 && base_name[name_len - 1] == '$');
            
            if (is_string_array) {
                /* String array: evaluate string expression and use biptrg_set_array_string */
                char *str_val = gweval_eval_string(expr);
                if (str_val) {
                    result = biptrg_set_array_string(base_name, array_index, str_val);
                } else {
                    result = ERROR;
                }
            } else {
                /* Numeric array: use biptrg_set_array_element */
                float val = gweval_eval_numeric(expr);
                result = biptrg_set_array_element(base_name, array_index, val);
            }
        } else {
            /* Simple variable assignment */
            /* Check if string variable (name ends with $) */
            size_t name_len = strlen(base_name);
            bool is_string_var = (name_len > 0 && base_name[name_len - 1] == '$');
            
            if (is_string_var) {
                struct variable *v = gwdata_alloc_variable(base_name, VT_STRING);
                if (v) {
                    char *str_val = gweval_eval_string(expr);
                    if (str_val) {
                        v->value.s = str_val;
                        result = SUCCESS;
                    }
                }
            } else {
                struct variable *v = gwdata_alloc_variable(base_name, VT_SINGLE);
                if (v) {
                    v->value.f = gweval_eval_numeric(expr);
                    result = SUCCESS;
                }
            }
        }
    }
    if (base_name) gwram_free(base_name);
    return result;
}

/* Simple statement parser and executor helpers */
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

/* Parse variable name with optional array index: A or A(5)
 * Caller must free *var_name when done
 */
static bool parse_var_ref(const char *str, char **var_name, int *index) {
    *var_name = NULL;
    *index = -1;
    
    if (!str) return false;
    
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
    
    /* Trim trailing whitespace from var_name */
    char *end = *var_name + strlen(*var_name) - 1;
    while (end > *var_name && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    
    /* Parse index */
    const char *index_str = paren + 1;
    const char *close_paren = strchr(index_str, ')');
    if (!close_paren) return false;
    
    char *index_buf = (char *)gwram_alloc(close_paren - index_str + 1);
    if (!index_buf) return false;
    memcpy(index_buf, index_str, close_paren - index_str);
    index_buf[close_paren - index_str] = '\0';
    
    /* Use strtol with validation instead of atoi */
    char *endptr = NULL;
    long idx = strtol(index_buf, &endptr, 10);
    if (endptr == index_buf || *endptr != '\0') {
        /* Invalid number */
        gwram_free(index_buf);
        return false;
    }
    *index = (int)idx;
    /* GW-BASIC: negative index treated as 0 */
    if (*index < 0) *index = 0;
    gwram_free(index_buf);
    return true;
}