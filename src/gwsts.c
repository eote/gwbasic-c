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
extern int dskcom_close_all(void);
extern int dskcom_get(int filenum, long record, void *buffer, int buflen);
extern int dskcom_put(int filenum, long record, const void *buffer, int buflen);
extern int dskcom_print(int filenum, const char *data);
extern int dskcom_line_input(int filenum, char *buffer, int maxlen);

/* Include FM_* constants for OPEN statement */
#include "dskcom.h"

/* Forward declarations */
static char *strdup_trim(const char *s);
static bool starts_with(const char *s, const char *prefix);
static bool parse_var_ref(const char *str, char **var_name, int *index, int *indices, int *num_indices);
int assign_value(const char *var_ref, const char *expr);

void gwsts_init(void) {
    /* No initialization needed */
}

/* FIELD statement: FIELD #filenum, width AS var$, width AS var$, ... */
int gwsts_handle_field(const char *trimmed) {
    /* Make a mutable copy since we need to modify the string */
    char buf[256];
    strncpy(buf, trimmed, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *rest = buf + 6;  /* Skip "FIELD " */
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    if (*rest == '#') {
        rest++;
        /* Find comma first, then parse filenum */
        char *comma = strchr(rest, ',');
        if (!comma) {
            return ERROR;
        }
        
        /* Temporarily null-terminate at comma to parse filenum */
        char saved = *comma;
        *comma = '\0';
        int filenum = (int)gweval_eval_numeric(rest);
        *comma = saved;
        
        char *field_spec = strdup_trim(comma + 1);
        if (field_spec) {
            int rc = dskcom_field(filenum, field_spec);
            gwram_free(field_spec);
            return rc;
        }
    }
    return ERROR;
}

/* LSET statement: LSET var$ = value */
int gwsts_handle_lset(const char *trimmed) {
    /* Make a mutable copy since we need to modify the string */
    char buf[256];
    strncpy(buf, trimmed, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *rest = buf + 5;  /* Skip "LSET " */
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    char *eq = strchr(rest, '=');
    if (eq) {
        *eq = '\0';
        char *varname = strdup_trim(rest);
        char *value = strdup_trim(eq + 1);
        if (varname && value) {
            char *eval_value = gweval_eval_string(value);
            if (eval_value) {
                /* Find which file has this field variable and do LSET */
                struct file_desc *fd = dskcom_get_file_list();
                while (fd) {
                    if (fd->mode == FM_RANDOM && fd->field_buffer) {
                        struct field_def *field = fd->fields;
                        while (field) {
                            if (strcasecmp(field->name, varname) == 0) {
                                int rc = dskcom_lset(fd->filenum, varname, eval_value);
                                gwram_free(varname);
                                gwram_free(value);
                                return rc;
                            }
                            field = field->next;
                        }
                    }
                    fd = fd->next;
                }
                gwram_free(varname);
                gwram_free(value);
                return ERROR;  /* Variable not found in any FIELD */
            }
            gwram_free(varname);
            gwram_free(value);
        }
    }
    return ERROR;
}

/* RSET statement: RSET var$ = value */
int gwsts_handle_rset(const char *trimmed) {
    /* Make a mutable copy since we need to modify the string */
    char buf[256];
    strncpy(buf, trimmed, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *rest = buf + 5;  /* Skip "RSET " */
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    char *eq = strchr(rest, '=');
    if (eq) {
        *eq = '\0';
        char *varname = strdup_trim(rest);
        char *value = strdup_trim(eq + 1);
        if (varname && value) {
            char *eval_value = gweval_eval_string(value);
            if (eval_value) {
                /* Find which file has this field variable and do RSET */
                struct file_desc *fd = dskcom_get_file_list();
                while (fd) {
                    if (fd->mode == FM_RANDOM && fd->field_buffer) {
                        struct field_def *field = fd->fields;
                        while (field) {
                            if (strcasecmp(field->name, varname) == 0) {
                                int rc = dskcom_rset(fd->filenum, varname, eval_value);
                                gwram_free(varname);
                                gwram_free(value);
                                return rc;
                            }
                            field = field->next;
                        }
                    }
                    fd = fd->next;
                }
                gwram_free(varname);
                gwram_free(value);
                return ERROR;  /* Variable not found in any FIELD */
            }
            gwram_free(varname);
            gwram_free(value);
        }
    }
    return ERROR;
}

/* PUT statement: PUT #filenum, [record] */
int gwsts_handle_put(const char *trimmed) {
    /* Make a mutable copy since we need to modify the string */
    char buf[256];
    strncpy(buf, trimmed, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *rest = buf + 4;  /* Skip "PUT " */
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    if (*rest == '#') {
        rest++;
        /* Parse filenum manually */
        int filenum = 0;
        while (*rest && isdigit((unsigned char)*rest)) {
            filenum = filenum * 10 + (*rest - '0');
            rest++;
        }
        
        long record = 0;
        char *comma = strchr(rest, ',');
        
        if (comma) {
            *comma = '\0';
            rest = comma + 1;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            
            /* Check if next is a record number or expression (not empty) */
            if (*rest != '\0') {
                record = (long)gweval_eval_numeric(rest);
            }
        }
        
        /* PUT #filenum, [record] - writes the entire field buffer */
        struct file_desc *fd = dskcom_find_file(filenum);
        if (fd && fd->mode == FM_RANDOM && fd->field_buffer) {
            return dskcom_put(filenum, record, fd->field_buffer, fd->record_length);
        }
    }
    return ERROR;
}

/* GET statement: GET #filenum, [record] */
int gwsts_handle_get(const char *trimmed) {
    /* Make a mutable copy since we need to modify the string */
    char buf[256];
    strncpy(buf, trimmed, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *rest = buf + 4;  /* Skip "GET " */
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    if (*rest == '#') {
        rest++;
        /* Parse filenum manually */
        int filenum = 0;
        while (*rest && isdigit((unsigned char)*rest)) {
            filenum = filenum * 10 + (*rest - '0');
            rest++;
        }
        
        long record = 0;
        char *comma = strchr(rest, ',');
        
        if (comma) {
            *comma = '\0';
            rest = comma + 1;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            
            /* Check if next is a record number or expression (not empty) */
            if (*rest != '\0') {
                record = (long)gweval_eval_numeric(rest);
            }
        }
        
        /* GET #filenum, [record] - reads into the entire field buffer */
        struct file_desc *fd = dskcom_find_file(filenum);
        if (fd && fd->mode == FM_RANDOM && fd->field_buffer) {
            return dskcom_get(filenum, record, fd->field_buffer, fd->record_length);
        }
    }
    return ERROR;
}

int gwsts_exec_statement(const char *text) {
    if (!text) return ERROR;
    
    /* Trim leading/trailing whitespace */
    char *trimmed = strdup_trim(text);
    if (!trimmed) return ERROR;
    
    int result = ERROR;
    
    /* Skip empty lines */
    if (strlen(trimmed) == 0) {
        gwram_free(trimmed);
        return SUCCESS;
    }
    
    /* REM statement - skip */
    if (starts_with(trimmed, "REM")) {
        result = SUCCESS;
    }
    /* DEF FN statement: DEF FN name(param) = expression */
    else if (starts_with(trimmed, "DEF FN")) {
        char *rest = trimmed + 6;  /* Skip "DEF FN" */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Parse function name (e.g., SQUARE) */
        char *fn_name_start = rest;
        char *fn_name_end = rest;
        while (*fn_name_end && (isalnum((unsigned char)*fn_name_end) || *fn_name_end == '_')) fn_name_end++;
        
        if (fn_name_end > fn_name_start) {
            size_t fn_name_len = fn_name_end - fn_name_start;
            char *fn_name = (char *)gwram_alloc(fn_name_len + 1);
            if (fn_name) {
                memcpy(fn_name, fn_name_start, fn_name_len);
                fn_name[fn_name_len] = '\0';
                
                rest = fn_name_end;
                while (*rest && isspace((unsigned char)*rest)) rest++;
                
                /* Expect '(' */
                if (*rest == '(') {
                    rest++;
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    
                    /* Parse parameter name */
                    char *param_start = rest;
                    char *param_end = rest;
                    while (*param_end && (isalnum((unsigned char)*param_end) || *param_end == '_')) param_end++;
                    
                    if (param_end > param_start) {
                        size_t param_len = param_end - param_start;
                        char *param_name = (char *)gwram_alloc(param_len + 1);
                        if (param_name) {
                            memcpy(param_name, param_start, param_len);
                            param_name[param_len] = '\0';
                            
                            rest = param_end;
                            while (*rest && isspace((unsigned char)*rest)) rest++;
                            
                            /* Expect ')' */
                            if (*rest == ')') {
                                rest++;
                                while (*rest && isspace((unsigned char)*rest)) rest++;
                                
                                /* Expect '=' */
                                if (*rest == '=') {
                                    rest++;
                                    while (*rest && isspace((unsigned char)*rest)) rest++;
                                    
                                    /* The rest is the function body expression */
                                    char *fn_body = strdup_trim(rest);
                                    if (fn_body) {
                                        /* Find or create the function definition variable */
                                        char full_fn_name[MAX_VAR_NAME + 1];
                                        snprintf(full_fn_name, sizeof(full_fn_name), "FN%s", fn_name);
                                        
                                        struct variable *fn_var = gwdata_alloc_variable(full_fn_name, VT_SINGLE);
                                        if (fn_var) {
                                            fn_var->is_fn_def = true;
                                            fn_var->fn_text_ptr = (intptr_t)fn_body;  /* Store text pointer to formula */
                                            strncpy(fn_var->fn_param_name, param_name, MAX_VAR_NAME);
                                            fn_var->fn_param_name[MAX_VAR_NAME] = '\0';
                                            fn_var->fn_param_type = VT_SINGLE;
                                            result = SUCCESS;
                                        }
                                        /* Do NOT free fn_body - we store the pointer in fn_var->fn_text_ptr */
                                    }
                                }
                            }
                            gwram_free(param_name);
                        }
                    }
                }
                gwram_free(fn_name);
            }
        }
    }
    /* DIM statement */
    else if (starts_with(trimmed, "DIM ")) {
        char *rest = trimmed + 4;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Parse: DIM A(10) or DIM A(10, 5) or DIM A(10), B(5) */
        char *token = rest;
        while (*token) {
            /* Skip leading whitespace */
            while (*token && isspace((unsigned char)*token)) token++;
            if (!*token) break;
            
            /* Find variable name */
            char *var_name = NULL;
            int dimensions = 0;
            int dim_sizes[8];  /* Max 8 dimensions */
            var_type_t type = VT_SINGLE;
            
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
              
                    /* Parse dimensions inside parentheses */
                    char *dim_start = paren + 1;
                    char *dim_ptr = dim_start;
                    dimensions = 0;
                    while (dim_ptr && *dim_ptr && *dim_ptr != ')') {
                        /* Skip whitespace */
                        while (*dim_ptr && isspace((unsigned char)*dim_ptr)) dim_ptr++;
                        if (*dim_ptr == ')') break;
                        
                        char *dim_end = dim_ptr;
                        while (*dim_end && *dim_end != ',' && *dim_end != ')') dim_end++;
                        
                        size_t dim_len = dim_end - dim_ptr;
                        char *dim_str = (char *)gwram_alloc(dim_len + 1);
                        if (dim_str) {
                            memcpy(dim_str, dim_ptr, dim_len);
                            dim_str[dim_len] = '\0';
                            int size = (int)gweval_eval_numeric(dim_str);
                            gwram_free(dim_str);
                            
                            if (dimensions < 8) {
                                dim_sizes[dimensions] = size;
                                dimensions++;
                            }
                        }
                        
                        if (*dim_end == ',') {
                            dim_ptr = dim_end + 1;
                        } else if (*dim_end == ')') {
                            dim_ptr = dim_end;
                            break;
                        } else {
                            dim_ptr = dim_end;
                        }
                    }
                }
            }
            
            if (var_name && dimensions > 0) {
                if (dimensions == 1) {
                    biptrg_dim_array(var_name, dim_sizes[0], type);
                } else {
                    biptrg_dim_array_multi(var_name, dim_sizes, dimensions, type);
                }
                result = SUCCESS;
                gwram_free(var_name);
            }
            
            /* Move to next token after comma (separating multiple DIM declarations) */
            token = strchr(token, ')');
            if (token) token = strchr(token + 1, ',');
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
    /* Direct assignment (without LET): var = expr (only if not a known statement) */
    else if (strchr(trimmed, '=') && 
             !starts_with(trimmed, "PRINT ") &&
             !starts_with(trimmed, "IF ") &&
             !starts_with(trimmed, "INPUT ") &&
             !starts_with(trimmed, "LINE INPUT ") &&
             !starts_with(trimmed, "FOR ") &&
             !starts_with(trimmed, "WHILE ") &&
             !starts_with(trimmed, "OPEN ") &&
             !starts_with(trimmed, "CLOSE") &&
             !starts_with(trimmed, "FIELD ") &&
             !starts_with(trimmed, "LSET ") &&
             !starts_with(trimmed, "RSET ") &&
             !starts_with(trimmed, "PUT ") &&
             !starts_with(trimmed, "GET ") &&
             !starts_with(trimmed, "ON ERROR GOTO ")) {
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
    /* FOR statement: FOR var = start TO limit [STEP step] */
    else if (starts_with(trimmed, "FOR ")) {
        extern int next86_for(const char **text);
        const char *text_ptr = trimmed + 4;  /* Skip "FOR " */
        result = next86_for(&text_ptr);
    }
    /* NEXT statement: NEXT [var] */
    else if (starts_with(trimmed, "NEXT")) {
        extern int next86_next(const char **text);
        const char *text_ptr = trimmed + 4;  /* Skip "NEXT" */
        if (*text_ptr == ' ') text_ptr++;  /* Skip optional space */
        result = next86_next(&text_ptr);
    }
    /* GOTO statement */
    else if (starts_with(trimmed, "GOTO ")) {
        extern int next86_goto(const char **text);
        const char *text_ptr = trimmed + 5;  /* Skip "GOTO " */
        result = next86_goto(&text_ptr);
    }
    /* GOSUB statement */
    else if (starts_with(trimmed, "GOSUB ")) {
        extern int next86_gosub(const char **text);
        const char *text_ptr = trimmed + 6;  /* Skip "GOSUB " */
        result = next86_gosub(&text_ptr);
    }
    /* RETURN statement */
    else if (starts_with(trimmed, "RETURN")) {
        extern int next86_return(const char **text);
        const char *text_ptr = trimmed + 6;  /* Skip "RETURN" */
        result = next86_return(&text_ptr);
    }
    /* END statement */
    else if (starts_with(trimmed, "END")) {
        /* END - stop execution with special return code */
        result = -2;  /* Distinct from ERROR (-1) */
    }
    /* DATA statement - no-op at runtime (data is read by READ) */
    else if (starts_with(trimmed, "DATA")) {
        result = SUCCESS;
    }
    /* CLEAR statement: CLEAR [,[max_mem][,stack_size]] */
    else if (starts_with(trimmed, "CLEAR")) {
        char *rest = trimmed + 5;  /* Skip "CLEAR" */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == ',') {
            rest++;  /* Skip first comma */
            while (*rest && isspace((unsigned char)*rest)) rest++;
            
            int max_mem = 0;
            int stack_size = 0;
            
            if (*rest) {
                max_mem = (int)gweval_eval_numeric(rest);
                char *comma = strchr(rest, ',');
                if (comma) {
                    rest = comma + 1;
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    if (*rest) {
                        stack_size = (int)gweval_eval_numeric(rest);
                    }
                }
            }
            bimisc_clear_with_args(stack_size, max_mem);
        } else {
            bimisc_clear();
        }
        result = SUCCESS;
    }
    /* RANDOMIZE statement: RANDOMIZE [n] (ASM GWMAIN.ASM RANDOM L3383-3413)
     * No argument → ask from terminal (INPRG: RANMES prompt);
     * With argument → FRMEVL + FRCINT → STRNDS: reseed + RNDMN2. */
    else if (starts_with(trimmed, "RANDOMIZE")) {
        char *rest = trimmed + 9;  /* Skip "RANDOMIZE" */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '\0') {
            /* No argument — ASM asks for input via RANMES prompt */
            int seed = 0;
            printf("Random number seed (-32768 to 32767)");
            fflush(stdout);
            char input[64];
            if (fgets(input, sizeof(input), stdin)) {
                seed = (int)gweval_eval_numeric(input);
            }
            math2_randomize(seed);
        } else {
            int seed = (int)gweval_eval_numeric(rest);
            math2_randomize(seed);
        }
        result = SUCCESS;
    }
    /* RENUM statement: RENUM [NN[,MM[,INC]]] (ASM GWMAIN.ASM RESEQ L3102-3222)
     * NN = first destination line; MM = threshold (lines < MM not resequenced);
     * INC = increment. Defaults: 10 / all / 10. */
    else if (starts_with(trimmed, "RENUM")) {
        char *rest = trimmed + 5;  /* Skip "RENUM" */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        int first = 0, mm = 0;
        int inc = -1;  /* -1 = absent (default 10); explicit 0 → FCERR */
        
        if (*rest) {
            first = (int)gweval_eval_numeric(rest);
            char *comma1 = strchr(rest, ',');
            if (comma1) {
                char *rest2 = comma1 + 1;
                while (*rest2 && isspace((unsigned char)*rest2)) rest2++;
                if (*rest2) {
                    mm = (int)gweval_eval_numeric(rest2);
                    char *comma2 = strchr(rest2, ',');
                    if (comma2) {
                        char *rest3 = comma2 + 1;
                        while (*rest3 && isspace((unsigned char)*rest3)) rest3++;
                        if (*rest3) {
                            inc = (int)gweval_eval_numeric(rest3);
                        } else {
                            inc = -1;  /* Trailing comma, INC absent */
                        }
                    }
                }
            }
        }
        
        result = gwlist_reseq(first, mm, inc);
    }
    /* DELETE statement: DELETE [start][-end] (ASM IBMRES.ASM R2 DELETE →
     * GWMAIN SCCLIN/SCCPTR deletion path). Range syntax: "DELETE 40" (single),
     * "DELETE 40-60" (range). start>end → FCERR (gwlist_delete_range checks). */
    else if (starts_with(trimmed, "DELETE")) {
        char *rest = trimmed + 6;  /* Skip "DELETE" */
        while (*rest && isspace((unsigned char)*rest)) rest++;

        int rc = ERROR;
        if (*rest) {
            /* Parse line numbers TEXTUALLY — the '-' is a RANGE separator, not
             * arithmetic. gweval_eval_numeric would evaluate "20-30" as -10. */
            char *dash = strchr(rest, '-');
            char start_buf[16];
            int start, end;
            if (dash) {
                size_t n = (size_t)(dash - rest);
                if (n >= sizeof(start_buf)) n = sizeof(start_buf) - 1;
                memcpy(start_buf, rest, n);
                start_buf[n] = '\0';
                start = atoi(start_buf);
                end = atoi(dash + 1);
            } else {
                start = atoi(rest);
                end = start;  /* Single line form */
            }
            rc = gwlist_delete_range(start, end);
        }
        result = rc;
    }
    /* RESTORE statement: RESTORE [line] */
    else if (starts_with(trimmed, "RESTORE")) {
        char *rest = trimmed + 7;  /* Skip "RESTORE" */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '\0') {
            /* RESTORE without line number - reset to beginning */
            bimisc_restore(0);
        } else {
            /* RESTORE with line number */
            int line_num = (int)gweval_eval_numeric(rest);
            bimisc_restore(line_num);
        }
        result = SUCCESS;
    }
    /* READ statement: READ var1, var2, ... */
    else if (starts_with(trimmed, "READ ")) {
        char *rest = trimmed + 5;  /* Skip "READ " */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '\0') {
            result = ERROR;  /* READ without variables */
        } else {
            /* Parse variable names */
            char *var_names[64];
            int var_count = 0;
            char *token = rest;
            
            while (*token && var_count < 64) {
                /* Skip leading whitespace */
                while (*token && isspace((unsigned char)*token)) token++;
                if (!*token) break;
                
                /* Find end of variable name (comma or end) */
                char *var_end = token;
                while (*var_end && *var_end != ',' && !isspace((unsigned char)*var_end)) var_end++;
                
                size_t name_len = var_end - token;
                if (name_len > 0 && name_len <= MAX_VAR_NAME) {
                    char *var_name = (char *)gwram_alloc(name_len + 1);
                    if (var_name) {
                        memcpy(var_name, token, name_len);
                        var_name[name_len] = '\0';
                        var_names[var_count++] = var_name;
                    }
                }
                
                /* Move to next token */
                token = var_end;
                if (*token == ',') token++;
            }
            
            if (var_count > 0) {
                const char *text_ptr = rest;
                result = bimisc_read(&text_ptr, var_count, var_names);
            } else {
                result = ERROR;
            }
            
            /* Free allocated variable names */
            for (int i = 0; i < var_count; i++) {
                if (var_names[i]) gwram_free(var_names[i]);
            }
        }
    }
    /* PRINT statement */
    else if (starts_with(trimmed, "PRINT")) {
        /* Check for bare PRINT (no arguments) - outputs newline */
        if (trimmed[5] == '\0') {
            printf("\n");
            result = SUCCESS;
        }
        /* Check for PRINT #filenum, (file output) - handle optional space after PRINT */
        else {
            char *p = trimmed + 5;  /* After "PRINT" */
            while (*p == ' ') p++;  /* Skip optional spaces */
            if (*p == '#') {
                /* File output: PRINT #filenum, expr */
                char *rest = p + 1;  /* After '#' */
                
                /* Find comma FIRST - make a copy to avoid modifying original */
                char *rest_copy = strdup_trim(rest);
                if (rest_copy) {
                    char *comma = strchr(rest_copy, ',');
                    int filenum = 0;
                    if (comma) {
                        *comma = '\0';  /* Truncate at comma to parse filenum */
                        filenum = (int)gweval_eval_numeric(rest_copy);
                    } else {
                        /* No comma - entire rest is filenum */
                        filenum = (int)gweval_eval_numeric(rest_copy);
                    }
                    
                    if (comma && filenum > 0) {
                        char *expr = strdup_trim(comma + 1);
                        if (expr) {
                            /* Handle multiple expressions separated by commas/semicolons (like console PRINT) */
                            char *text = expr;
                            char combined_output[256] = {0};
                            size_t combined_len = 0;
                            
                            while (*text) {
                                /* Find end of expression (next comma, semicolon, or end) - skip commas/semicolons inside quotes or parentheses */
                                char *expr_start = text;
                                char *expr_end = text;
                                bool in_quotes = false;
                                int paren_depth = 0;
                                while (*expr_end && !(!in_quotes && paren_depth == 0 && (*expr_end == ',' || *expr_end == ';'))) {
                                    if (*expr_end == '\"') {
                                        in_quotes = !in_quotes;
                                    } else if (!in_quotes) {
                                        if (*expr_end == '(') paren_depth++;
                                        else if (*expr_end == ')') paren_depth--;
                                    }
                                    expr_end++;
                                }
                                
                                /* Extract expression */
                                size_t expr_len = expr_end - expr_start;
                                char *single_expr = (char *)gwram_alloc(expr_len + 1);
                                if (!single_expr) {
                                    gwram_free(expr);
                                    return ERROR;
                                }
                                memcpy(single_expr, expr_start, expr_len);
                                single_expr[expr_len] = '\0';
                                
                                /* Trim whitespace */
                                char *trim_start = single_expr;
                                char *trim_end = single_expr + strlen(single_expr) - 1;
                                while (trim_end >= trim_start && isspace((unsigned char)*trim_end)) {
                                    *trim_end = '\0';
                                    trim_end--;
                                }
                                while (*trim_start && isspace((unsigned char)*trim_start)) trim_start++;
                                
                                if (*trim_start) {
                                    /* Try string evaluation first, then numeric */
                                    char *str_val = gweval_eval_string(trim_start);
                                    if (str_val) {
                                        size_t str_len = strlen(str_val);
                                        if (combined_len + str_len < sizeof(combined_output) - 1) {
                                            strcpy(combined_output + combined_len, str_val);
                                            combined_len += str_len;
                                        }
                                    } else {
                                        float val = gweval_eval_numeric(trim_start);
                                        char buf[64];
                                        /* GW-BASIC: positive numbers have leading space */
                                        snprintf(buf, sizeof(buf), "%s%g", (val >= 0) ? " " : "", val);
                                        size_t str_len = strlen(buf);
                                        if (combined_len + str_len < sizeof(combined_output) - 1) {
                                            strcpy(combined_output + combined_len, buf);
                                            combined_len += str_len;
                                        }
                                    }
                                }
                                
                                gwram_free(single_expr);
                                
                                /* Handle separator */
                                if (*expr_end == ',') {
                                    /* Zone separator - add tab */
                                    if (combined_len + 1 < sizeof(combined_output) - 1) {
                                        combined_output[combined_len++] = '\t';
                                        combined_output[combined_len] = '\0';
                                    }
                                } else if (*expr_end == ';') {
                                    /* No separator - just continue */
                                }
                                
                                text = expr_end;
                                if (*text) text++;
                            }
                            
                            /* Write combined output to file */
                            result = dskcom_print(filenum, combined_output);
                            
                            gwram_free(expr);
                        }
                    }
                    gwram_free(rest_copy);
                }
            } else {
                /* Regular PRINT to console */
                char *expr = strdup_trim(trimmed + 5);  /* After "PRINT" */
                if (expr) {
                    /* Handle multiple expressions separated by commas/semicolons */
                    char *text = expr;
                    bool trailing_comma = false;
                    bool trailing_semicolon = false;
                    
                    while (*text) {
                        /* Find end of expression (next comma, semicolon, or end) - skip commas/semicolons inside quotes or parentheses */
                        char *expr_start = text;
                        char *expr_end = text;
                        bool in_quotes = false;
                        int paren_depth = 0;
                        while (*expr_end && !(!in_quotes && paren_depth == 0 && (*expr_end == ',' || *expr_end == ';'))) {
                            if (*expr_end == '\"') {
                                in_quotes = !in_quotes;
                            } else if (!in_quotes) {
                                if (*expr_end == '(') paren_depth++;
                                else if (*expr_end == ')') paren_depth--;
                            }
                            expr_end++;
                        }
                        
                        /* Extract expression */
                        size_t expr_len = expr_end - expr_start;
                        char *single_expr = (char *)gwram_alloc(expr_len + 1);
                        if (!single_expr) {
                            gwram_free(expr);
                            return ERROR;
                        }
                        memcpy(single_expr, expr_start, expr_len);
                        single_expr[expr_len] = '\0';
                        
                        /* Trim whitespace - use a separate pointer to avoid corrupting the allocated pointer */
                        char *trim_start = single_expr;
                        char *trim_end = single_expr + strlen(single_expr) - 1;
                        while (trim_end >= trim_start && isspace((unsigned char)*trim_end)) {
                            *trim_end = '\0';
                            trim_end--;
                        }
                        while (*trim_start && isspace((unsigned char)*trim_start)) trim_start++;
                        
                        if (*trim_start) {
                            /* Check for USING keyword - PRINT USING format; value */
                            char *using_pos = strstr(trim_start, "USING");
                            if (using_pos && (using_pos == trim_start || *(using_pos - 1) == ' ' || *(using_pos - 1) == '\t')) {
                                /* Found USING keyword - extract format string and value */
                                *using_pos = '\0';
                                char *format_str = strdup_trim(trim_start);
                                if (format_str) {
                                    char *value_expr = strdup_trim(using_pos + 5);  /* Skip "USING" */
                                    if (value_expr) {
                                        /* Evaluate format string (should be string literal) */
                                        char *fmt_val = gweval_eval_string(format_str);
                                        if (fmt_val) {
                                            /* Evaluate value expression */
                                            float val = gweval_eval_numeric(value_expr);
                                            biprtu_print_using(fmt_val, val);
                                        } else {
                                            /* Fallback: try numeric format string */
                                            float val = gweval_eval_numeric(value_expr);
                                            biprtu_print_using(format_str, val);
                                        }
                                        gwram_free(value_expr);
                                    }
                                    gwram_free(format_str);
                                }
                            } else {
                                /* Try string evaluation first, then numeric */
                                char *str_val = gweval_eval_string(trim_start);
                                if (str_val) {
                                    printf("%s", str_val);
                                } else {
                                    float val = gweval_eval_numeric(trim_start);
                                    /* GW-BASIC: positive numbers have leading space */
                                    printf("%s%g", (val >= 0) ? " " : "", val);
                                }
                            }
                        }
                        
                        gwram_free(single_expr);
                        
                        /* Handle separator */
                        if (*expr_end == ',') {
                            trailing_comma = true;
                            trailing_semicolon = false;
                            /* Zone separator - add tab */
                            printf("\t");
                        } else if (*expr_end == ';') {
                            trailing_semicolon = true;
                            trailing_comma = false;
                            /* No separator */
                        } else {
                            trailing_comma = false;
                            trailing_semicolon = false;
                        }
                        
                        text = expr_end;
                        if (*text) text++;
                    }
                    
                    gwram_free(expr);
                    
                    /* Print newline unless last expression ended with comma or semicolon */
                    if (!trailing_comma && !trailing_semicolon) {
                        printf("\n");
                    }
                    
                    result = SUCCESS;
                }
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
    /* WHILE statement: WHILE <condition> */
    else if (starts_with(trimmed, "WHILE ")) {
        char *rest = trimmed + 6;  /* Skip "WHILE " */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        
        /* Find matching WEND in the program */
        extern int fiveo_push_while(int while_text_ptr, int wend_text_ptr, int while_line);
        extern int fiveo_find_while(int wend_text_ptr);
        extern int fiveo_pop_while(struct while_entry *entry);
        extern int fiveo_while_depth(void);
        extern int fiveo_peek_while(struct while_entry *entry);
        extern WORD g_curlin;
        
        /* We need to find the matching WEND. Search through program lines. */
        struct program_line *curr = gwlist_get_first_line();
        int while_line = g_curlin;
        int wend_line = 0;
        int while_depth = 0;
        
        while (curr) {
            if (curr->number > while_line) {
                char *trimmed_line = strdup_trim(curr->text);
                if (trimmed_line) {
                    if (starts_with(trimmed_line, "WHILE ")) {
                        while_depth++;
                    } else if (starts_with(trimmed_line, "WEND")) {
                        if (while_depth == 0) {
                            wend_line = curr->number;
                            gwram_free(trimmed_line);
                            break;
                        }
                        while_depth--;
                    }
                    gwram_free(trimmed_line);
                }
            }
            curr = curr->next;
        }
        
        
        /* Check if this WHILE is already on stack (top entry) */
                    struct while_entry entry;
                    int rc = fiveo_peek_while(&entry);
                    bool already_on_stack = (rc == 0 && entry.while_line == g_curlin);
            
                    if (!already_on_stack) {
                        /* First execution - push WHILE entry */
                        int rc = fiveo_push_while(g_curlin, wend_line, g_curlin);  /* while_text_ptr=line, wend_text_ptr=wend_line, while_line=line */
                        if (rc != 0) {
                            result = ERROR;  /* Stack overflow */
                        } else {
                            /* Verify it was pushed */
                            struct while_entry entry;
                            fiveo_peek_while(&entry);
                            result = SUCCESS;
                        }
                    }
                    /* Whether first time or jump back, evaluate condition */
                    float cond_val = gweval_eval_numeric(rest);
            
                    if (cond_val != 0.0f) {
                        /* Condition true - continue to loop body */
                        result = SUCCESS;
                    } else {
                        /* Condition false - jump to line after WEND */
                        struct program_line *after_wend = gwlist_find_line(wend_line);
                        if (after_wend && after_wend->next) {
                            result = after_wend->next->number;  /* Jump to line after WEND */
                        } else {
                            result = 0;  /* No next line, continue normally */
                        }
                    }
                }
    /* WEND statement */
    else if (starts_with(trimmed, "WEND")) {
        extern int fiveo_pop_while(struct while_entry *entry);
        extern int fiveo_peek_while(struct while_entry *entry);
        extern int fiveo_while_depth(void);
        
        /* WEND matches the most recent WHILE (by text position) */
        /* Search from top of stack for matching entry */
        int rc = fiveo_while_depth();
        
        if (rc > 0) {
            /* Peek at top entry */
            struct while_entry entry;
            extern int fiveo_peek_while(struct while_entry *entry);
            int rc = fiveo_peek_while(&entry);
            
            if (rc == 0) {
                /* Jump back to WHILE to re-evaluate condition */
                result = entry.while_line;  /* Jump to WHILE line */
            } else {
                result = ERROR;  /* Stack issue */
            }
        } else {
            result = ERROR;  /* WEND without WHILE */
        }
    }
    /* ON ERROR statement */
    else if (starts_with(trimmed, "ON ERROR GOTO ")) {
        char *rest = trimmed + 14;  /* Skip "ON ERROR GOTO " */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        int line = (int)gweval_eval_numeric(rest);
        if (line == 0) {
            /* ON ERROR GOTO 0 - disable error trapping */
            gwdata_clear_on_error();
        } else {
            /* Find the target line to get text pointer */
            extern struct program_line *gwlist_find_line(int number);
            struct program_line *target = gwlist_find_line(line);
            int text_ptr = target ? (int)(intptr_t)target->text : 0;
            gwdata_set_on_error_goto(line, text_ptr);
        }
        result = SUCCESS;
    }
    /* RESUME statement */
    else if (starts_with(trimmed, "RESUME")) {
        if (!gwdata_is_on_error_active() || !gwdata_is_in_error_handler()) {
            /* RESUME without error - error */
            result = ERROR;
        } else {
            char *rest = trimmed + 6;  /* Skip "RESUME" */
            while (*rest && isspace((unsigned char)*rest)) rest++;
            
            if (starts_with(rest, "NEXT")) {
                /* RESUME NEXT - continue after error line */
                int err_line = gwdata_get_error_line();
                (void)gwdata_get_error_text_ptr();
                
                /* Find the line after the error line */
                extern struct program_line *gwlist_get_first_line(void);
                extern struct program_line *gwlist_find_line(int number);
                struct program_line *curr = gwlist_find_line(err_line);
                if (curr && curr->next) {
                    gwdata_clear_error();
                    gwdata_exit_error_handler();
                    result = curr->next->number;  /* Jump to next line */
                } else {
                    /* No next line - end program */
                    gwdata_clear_error();
                    gwdata_exit_error_handler();
                    result = SUCCESS;
                }
            } else if (*rest == '\0') {
                /* RESUME - retry the line that caused error */
                int err_line = gwdata_get_error_line();
                (void)gwdata_get_error_text_ptr();
                gwdata_clear_error();
                gwdata_exit_error_handler();
                result = err_line;  /* Jump to error line */
            } else {
                /* RESUME <line> - resume at specific line */
                int line = (int)gweval_eval_numeric(rest);
                gwdata_clear_error();
                gwdata_exit_error_handler();
                result = line;
            }
        }
    }
    /* ERROR statement */
    else if (starts_with(trimmed, "ERROR ")) {
        char *rest = trimmed + 6;  /* Skip "ERROR " */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        int code = (int)gweval_eval_numeric(rest);
        if (code == 0) {
            result = ERROR;  /* FCERR - error code 0 is illegal */
        } else {
            /* Force an error with given code */
            gwdata_set_error(code, g_state->current_line, 0);
            /* Jump to error handler if ON ERROR active */
            if (gwdata_is_on_error_active()) {
                gwdata_enter_error_handler();
                result = gwdata_get_on_error_line();
            } else {
                result = ERROR;  /* No handler - propagate error */
            }
        }
    }
    /* KILL statement: KILL <string_expr> */
    else if (starts_with(trimmed, "KILL ")) {
        char *rest = trimmed + 5;  /* Skip "KILL " */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        
        /* Evaluate string expression (supports quoted string, variable, or function) */
        char *filename = gweval_eval_string(rest);
        
        if (filename) {
            result = gwram_delete_file(filename);
            /* Don't free filename - it comes from string heap (bump allocator) */
        } else {
            result = ERROR;
        }
        
    }
    /* OPEN statement */
    else if (starts_with(trimmed, "OPEN ")) {
        /* Parse: OPEN "filename" FOR INPUT/OUTPUT/APPEND/RANDOM AS #filenum [LEN=reclen] */
        char *rest = trimmed + 5;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Extract filename (quoted string) */
        if (*rest == '\"') {
            rest++;
            char *filename_end = strchr(rest, '\"');
            if (filename_end) {
                size_t fn_len = filename_end - rest;
                char *filename = (char *)gwram_alloc(fn_len + 1);
                if (filename) {
                    memcpy(filename, rest, fn_len);
                    filename[fn_len] = '\0';
                    
                    rest = filename_end + 1;
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    
                    /* Parse mode */
                    int mode = 0;  /* 0=INPUT, 1=OUTPUT, 2=APPEND, 3=RANDOM -> maps to FM_* */
                    int filenum = 1;
                    int reclen = 128;
                    bool has_for = false;
                    
                    if (starts_with(rest, "FOR ")) {
                        rest += 4;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        if (starts_with(rest, "INPUT")) {
                            mode = FM_INPUT;
                            rest += 5;
                            has_for = true;
                        } else if (starts_with(rest, "OUTPUT")) {
                            mode = FM_OUTPUT;
                            rest += 6;
                            has_for = true;
                        } else if (starts_with(rest, "APPEND")) {
                            mode = FM_APPEND;
                            rest += 6;
                            has_for = true;
                        } else if (starts_with(rest, "RANDOM")) {
                            mode = FM_RANDOM;
                            rest += 6;
                            has_for = true;
                        }
                    }
                    
                    if (has_for) {
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                    } else {
                        /* No FOR clause - default to RANDOM for "AS #" syntax */
                        mode = FM_RANDOM;
                    }
                    
                    /* Parse "AS #filenum" - handle both "AS #" and "AS#" */
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    if (starts_with(rest, "AS")) {
                        rest += 2;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        if (*rest == '#') rest++;
                        /* Parse filenum manually (avoid gweval_eval_numeric parsing rest of line) */
                        filenum = 0;
                        while (*rest && isdigit((unsigned char)*rest)) {
                            filenum = filenum * 10 + (*rest - '0');
                            rest++;
                        }
                        /* Skip trailing whitespace */
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                    }
                    
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    if (starts_with(rest, "LEN=")) {
                        rest += 4;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        /* Parse record length directly (not via expression evaluator to avoid LEN function conflict) */
                        reclen = 0;
                        while (*rest && isdigit((unsigned char)*rest)) {
                            reclen = reclen * 10 + (*rest - '0');
                            rest++;
                        }
                        /* LEN clause implies RANDOM mode */
                        mode = FM_RANDOM;
                    } else if (starts_with(rest, "LEN =")) {
                        rest += 5;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        /* Parse record length directly (not via expression evaluator to avoid LEN function conflict) */
                        reclen = 0;
                        while (*rest && isdigit((unsigned char)*rest)) {
                            reclen = reclen * 10 + (*rest - '0');
                            rest++;
                        }
                        /* LEN clause implies RANDOM mode */
                        mode = FM_RANDOM;
                    }
                    
                    result = dskcom_open(filename, filenum, mode, reclen);
                    gwram_free(filename);
                }
            }
        }
    }
    /* CLOSE statement */
    else if (starts_with(trimmed, "CLOSE")) {
        char *rest = trimmed + 5;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '\0') {
            result = dskcom_close_all();
        } else {
            if (*rest == '#') rest++;
            int filenum = (int)gweval_eval_numeric(rest);
            result = dskcom_close(filenum);
        }
    }
    /* FIELD statement: FIELD #filenum, width AS var$, width AS var$, ... */
    else if (starts_with(trimmed, "FIELD ")) {
        result = gwsts_handle_field(trimmed);
    }
    /* LSET statement: LSET var$ = value */
    else if (starts_with(trimmed, "LSET ")) {
        result = gwsts_handle_lset(trimmed);
    }
    /* RSET statement: RSET var$ = value */
    else if (starts_with(trimmed, "RSET ")) {
        result = gwsts_handle_rset(trimmed);
    }
    /* PUT statement: PUT #filenum, [record], var */
    else if (starts_with(trimmed, "PUT ")) {
        result = gwsts_handle_put(trimmed);
    }
    /* GET statement: GET #filenum, [record], var */
    else if (starts_with(trimmed, "GET ")) {
        result = gwsts_handle_get(trimmed);
    }
    /* INPUT statement */
    else if (starts_with(trimmed, "INPUT ")) {
        char *rest = trimmed + 6;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        char *prompt = NULL;
        if (*rest == '\"') {
            /* Extract prompt string */
            char *prompt_end = strchr(rest + 1, '\"');
            if (prompt_end) {
                size_t prompt_len = prompt_end - rest - 1;
                prompt = (char *)gwram_alloc(prompt_len + 1);
                if (prompt) {
                    memcpy(prompt, rest + 1, prompt_len);
                    prompt[prompt_len] = '\0';
                    rest = prompt_end + 1;
                }
            }
        }
        
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* Variable list */
        char *var_name = rest;
        if (*var_name) {
            char *comma = strchr(var_name, ',');
            if (comma) *comma = '\0';
            
            char *var_ref = strdup_trim(var_name);
            if (var_ref) {
                printf("%s", prompt ? prompt : "? ");
                char input_buf[256];
                if (fgets(input_buf, sizeof(input_buf), stdin)) {
                    input_buf[strcspn(input_buf, "\n")] = '\0';
                    assign_value(var_ref, input_buf);
                    result = SUCCESS;
                }
                gwram_free(var_ref);
            }
        }
    }
    /* LINE INPUT statement */
    else if (starts_with(trimmed, "LINE INPUT ")) {
        char *rest = trimmed + 11;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        char *prompt = NULL;
        if (*rest == '\"') {
            char *prompt_end = strchr(rest + 1, '\"');
            if (prompt_end) {
                size_t prompt_len = prompt_end - rest - 1;
                prompt = (char *)gwram_alloc(prompt_len + 1);
                if (prompt) {
                    memcpy(prompt, rest + 1, prompt_len);
                    prompt[prompt_len] = '\0';
                    rest = prompt_end + 1;
                }
            }
        }
        
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        if (*rest == '#') {
            /* LINE INPUT #filenum, var */
            rest++;
            int filenum = (int)gweval_eval_numeric(rest);
            char *comma = strchr(rest, ',');
            if (comma) {
                *comma = '\0';
                char *var_ref = strdup_trim(comma + 1);
                if (var_ref) {
                    char input_buf[256];
                    result = dskcom_line_input(filenum, input_buf, sizeof(input_buf));
                    if (result == SUCCESS) {
                        assign_value(var_ref, input_buf);
                    }
                    gwram_free(var_ref);
                }
            }
        } else {
            /* LINE INPUT var */
            char *var_ref = strdup_trim(rest);
            if (var_ref) {
                printf("%s", prompt ? prompt : "");
                char input_buf[256];
                if (fgets(input_buf, sizeof(input_buf), stdin)) {
                    input_buf[strcspn(input_buf, "\n")] = '\0';
                    assign_value(var_ref, input_buf);
                    result = SUCCESS;
                }
                gwram_free(var_ref);
            }
        }
    }
    
    gwram_free(trimmed);
    return result;
}

/* Trim leading/trailing whitespace - returns newly allocated string */
static char *strdup_trim(const char *s) {
    if (!s) return NULL;
    
    /* Find start (skip leading whitespace) */
    const char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    
    if (!*start) {
        char *empty = (char *)gwram_alloc(1);
        if (empty) *empty = '\0';
        return empty;
    }
    
    /* Find end (skip trailing whitespace) */
    const char *end = start + strlen(start) - 1;
    while (end >= start && isspace((unsigned char)*end)) end--;
    
    size_t len = end - start + 1;
    char *result = (char *)gwram_alloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }
    return result;
}

/* Case-insensitive prefix match */
static bool starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return false;
    size_t len = strlen(prefix);
    return strncasecmp(s, prefix, len) == 0;
}

/* Parse variable reference: varname or varname(index) or varname(index, ...) */
static bool parse_var_ref(const char *str, char **var_name, int *index, int *indices, int *num_indices) {
    if (!str || !var_name || !index) return false;
    
    *var_name = NULL;
    *index = -1;
    if (indices) *num_indices = 0;
    
    const char *paren = strchr(str, '(');
    if (paren) {
        const char *close_paren = strchr(paren, ')');
        if (close_paren) {
            size_t name_len = paren - str;
            *var_name = (char *)gwram_alloc(name_len + 1);
            if (*var_name) {
                memcpy(*var_name, str, name_len);
                (*var_name)[name_len] = '\0';
                
                /* Parse indices */
                if (indices) {
                    const char *idx_ptr = paren + 1;
                    *num_indices = 0;
                    while (idx_ptr < close_paren && *num_indices < 8) {
                        /* Skip whitespace */
                        while (idx_ptr < close_paren && isspace((unsigned char)*idx_ptr)) idx_ptr++;
                        if (idx_ptr >= close_paren) break;
                        
                        const char *idx_end = idx_ptr;
                        while (idx_end < close_paren && *idx_end != ',' && *idx_end != ')') idx_end++;
                        
                        size_t idx_len = idx_end - idx_ptr;
                        char *idx_str = (char *)gwram_alloc(idx_len + 1);
                        if (idx_str) {
                            memcpy(idx_str, idx_ptr, idx_len);
                            idx_str[idx_len] = '\0';
                            indices[*num_indices] = (int)gweval_eval_numeric(idx_str);
                            gwram_free(idx_str);
                            (*num_indices)++;
                        }
                        
                        if (idx_end < close_paren && *idx_end == ',') {
                            idx_ptr = idx_end + 1;
                        } else {
                            idx_ptr = idx_end;
                        }
                    }
                }
                
                /* For backward compatibility, also set single index if only one dimension */
                if (indices && *num_indices > 0) {
                    *index = indices[0];
                }
            }
            return true;
        }
    } else {
        *var_name = strdup_trim(str);
        return *var_name != NULL;
    }
    
    return false;
}

/* Common assignment function for both LET and direct assignment */
int assign_value(const char *var_ref, const char *expr) {
    if (!var_ref || !expr) return ERROR;
    
    
    char *base_name = NULL;
    int array_index = -1;
    int indices[8];
    int num_indices = 0;
    parse_var_ref(var_ref, &base_name, &array_index, indices, &num_indices);
    
    DEBUG_PRINT("assign_value: var_ref='%s', base_name='%s', array_index=%d, num_indices=%d, expr='%s'", 
                var_ref, base_name ? base_name : "NULL", array_index, num_indices, expr);
    
    int result = ERROR;
    if (base_name && expr) {
        if (num_indices > 0) {
            /* Array element assignment (multi-dimensional) */
            /* Check if this is a string array (name ends with $) */
            size_t name_len = strlen(base_name);
            bool is_string_array = (name_len > 0 && base_name[name_len - 1] == '$');
            
            if (is_string_array) {
                /* String array: evaluate string expression and use biptrg_set_array_string */
                char *str_val = gweval_eval_string(expr);
                if (str_val) {
                    result = biptrg_set_array_string(base_name, indices[0], str_val);
                } else {
                    /* Fallback: treat raw expr as literal string (for DATA values without quotes) */
                    size_t expr_len = strlen(expr);
                    char *heap_str = gwdata_alloc_string(expr_len + 1);
                    if (heap_str) {
                        strcpy(heap_str, expr);
                        result = biptrg_set_array_string(base_name, indices[0], heap_str);
                    }
                }
            } else {
                /* Numeric array: use multi or single interface */
                float val = gweval_eval_numeric(expr);
                DEBUG_PRINT("assign_value: calling biptrg_set_array_element_multi('%s', indices, %f)", base_name, val);
                if (num_indices == 1) {
                    result = biptrg_set_array_element(base_name, indices[0], val);
                } else {
                    result = biptrg_set_array_element_multi(base_name, indices, val);
                }
                DEBUG_PRINT("assign_value: biptrg_set_array_element_multi returned %d", result);
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
                        v->value.s.ptr = str_val;
                        v->value.s.len = strlen(str_val);
                        result = SUCCESS;
                    } else {
                        /* Not a recognized string expression — treat raw expr as literal string
                           (for LINE INPUT etc. which provide unquoted raw text) */
                        size_t expr_len = strlen(expr);
                        char *heap_str = gwdata_alloc_string(expr_len + 1);
                        if (heap_str) {
                            strcpy(heap_str, expr);
                            v->value.s.ptr = heap_str;
                            v->value.s.len = expr_len;
                            result = SUCCESS;
                        }
                    }
                }
            } else {
                /* Numeric variable (integer or float) */
                /* Evaluate expression FIRST before potentially clearing the variable */
                float val = gweval_eval_numeric(expr);
                struct variable *v = gwdata_alloc_variable(base_name, VT_SINGLE);
                if (v) {
                    v->value.f = val;
                    result = SUCCESS;
                }
            }
        }
    }
    
    if (base_name) gwram_free(base_name);
    return result;
}