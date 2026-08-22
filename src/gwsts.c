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

/* Forward declarations for static functions */
static char *strdup_trim(const char *s);
static bool starts_with(const char *s, const char *prefix);
static bool parse_var_ref(const char *str, char **var_name, int *index, int *indices, int *num_indices);
static int assign_value(const char *var_ref, const char *expr);

void gwsts_init(void) {
    /* No initialization needed */
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
             !starts_with(trimmed, "WHILE ")) {
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
                            /* Try string evaluation first, then numeric */
                            char *str_val = gweval_eval_string(trim_start);
                            if (str_val) {
                                printf("%s", str_val);
                            } else {
                                float val = gweval_eval_numeric(trim_start);
                                printf("%g", val);
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
                    
                    if (starts_with(rest, "FOR ")) {
                        rest += 4;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        if (starts_with(rest, "INPUT")) {
                            mode = FM_INPUT;
                            rest += 5;
                        } else if (starts_with(rest, "OUTPUT")) {
                            mode = FM_OUTPUT;
                            rest += 6;
                        } else if (starts_with(rest, "APPEND")) {
                            mode = FM_APPEND;
                            rest += 6;
                        } else if (starts_with(rest, "RANDOM")) {
                            mode = FM_RANDOM;
                            rest += 6;
                        }
                    }
                    
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    if (starts_with(rest, "AS ")) {
                        rest += 3;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        if (*rest == '#') rest++;
                        filenum = (int)gweval_eval_numeric(rest);
                    }
                    
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    if (starts_with(rest, "LEN=")) {
                        rest += 4;
                        reclen = (int)gweval_eval_numeric(rest);
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
static int assign_value(const char *var_ref, const char *expr) {
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
                    result = ERROR;
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
                        v->value.s = str_val;
                        result = SUCCESS;
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