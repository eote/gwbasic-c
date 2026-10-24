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


/* ===== v141 MEDIUM-2: shared coordinate-pair scanner =====
 * Depth/quote-aware: finds the matching ')' for the '(' at **pp (which must
 * point at '('), splits the inside at the first top-level ',' and evaluates
 * both halves as expressions. Advances *pp past ')'. Returns false on
 * malformed input (no comma / buffer overflow). Nested parens and quoted
 * strings inside either half are skipped, so PSET(A(1),A(2)) works. */
static bool gwsts_parse_coord_pair(const char **pp, int *x, int *y) {
    const char *p = *pp;          /* at '(' */
    const char *q = p + 1;        /* inside */
    int depth = 0;
    bool inq = false;
    const char *closep = NULL;
    while (*q) {
        char c = *q;
        if (inq) {
            if (c == '"') inq = false;
        } else if (c == '"') {
            inq = true;
        } else if (c == '(') {
            depth++;
        } else if (c == ')') {
            if (depth == 0) { closep = q; break; }
            depth--;
        }
        q++;
    }
    if (!closep) return false;

    /* top-level comma inside (p+1 .. closep) */
    const char *seg = p + 1;
    depth = 0; inq = false;
    const char *cm = NULL;
    for (const char *e = seg; e < closep; e++) {
        char c = *e;
        if (inq) {
            if (c == '"') inq = false;
            continue;
        }
        if (c == '"') { inq = true; continue; }
        if (c == '(') { depth++; continue; }
        if (c == ')') { depth--; continue; }
        if (c == ',' && depth == 0) { cm = e; break; }
    }
    if (!cm) return false;

    char xbuf[128], ybuf[128];
    size_t xl = (size_t)(cm - seg);
    size_t yl = (size_t)(closep - (cm + 1));
    if (xl >= sizeof(xbuf) || yl >= sizeof(ybuf)) return false;
    memcpy(xbuf, seg, xl); xbuf[xl] = '\0';
    memcpy(ybuf, cm + 1, yl); ybuf[yl] = '\0';
    *x = (int)gweval_eval_numeric(xbuf);
    *y = (int)gweval_eval_numeric(ybuf);
    *pp = closep + 1;
    return true;
}

/* ===== v143 M2: graphics GET/PUT statement handlers (ASM ADVGRP GPUTG) ===== */

/* GET (x1,y1)-(x2,y2),array — save screen block into numeric array */
static int gwsts_handle_gfx_get(const char *trimmed) {
    const char *p = trimmed + 3;
    while (*p && isspace((unsigned char)*p)) p++;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (*p != '(') return ERROR;
    if (!gwsts_parse_coord_pair(&p, &x1, &y1)) return ERROR;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '-') p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '(') return ERROR;
    if (!gwsts_parse_coord_pair(&p, &x2, &y2)) return ERROR;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != ',') return ERROR;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    /* array name (letters/digits, optional type suffix) */
    char arrname[MAX_VAR_NAME + 1];
    size_t n = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '%' || *p == '!' ||
                  *p == '#' || *p == '$') && n < MAX_VAR_NAME) {
        arrname[n++] = *p++;
    }
    arrname[n] = '\0';
    if (n == 0) return ERROR;
    return advgrp_get(x1, y1, x2, y2, arrname);
}

/* PUT (x,y),array[,action] — action: PSET/PRESET/AND/OR/XOR (default XOR) */
static int gwsts_handle_gfx_put(const char *trimmed) {
    const char *p = trimmed + 3;
    while (*p && isspace((unsigned char)*p)) p++;
    int px = 0, py = 0;
    if (*p != '(') return ERROR;
    if (!gwsts_parse_coord_pair(&p, &px, &py)) return ERROR;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != ',') return ERROR;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    char arrname[MAX_VAR_NAME + 1];
    size_t n = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '%' || *p == '!' ||
                  *p == '#' || *p == '$') && n < MAX_VAR_NAME) {
        arrname[n++] = *p++;
    }
    arrname[n] = '\0';
    if (n == 0) return ERROR;
    /* optional ,action */
    int action = -1;  /* -1 = default XOR (ASM GFUNTB default) */
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ',') {
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (starts_with(p, "PSET")) action = 1;
        else if (starts_with(p, "PRESET")) action = 2;
        else if (starts_with(p, "AND")) action = 3;
        else if (starts_with(p, "OR")) action = 4;
        else if (starts_with(p, "XOR")) action = 5;
        else return ERROR;
    }
    return advgrp_put(px, py, arrname, action);
}

/* ===== v148 MEDIUM-1: RUN environment reset =====
 * GW-BASIC RUN semantics: clear all variables, close files, reset FOR/
 * GOSUB stacks and ON ERROR state; only the program text survives
 * (gwlist untouched). Screen/graphics state is not cleared. */
void gwsts_reset_runtime_environment(void) {
    /* variables + string heap (gwdata_init resets the variable table and
     * the string heap wholesale) */
    gwdata_init();
    /* close all open files (RUN closes files) */
    giodsk_close_all();
    /* FOR / GOSUB stacks */
    next86_clear_for_stack();
    next86_clear_gosub_stack();
    /* ON ERROR state + handler flag + pending error */
    gwdata_set_on_error_goto(0, 0);
    gwdata_exit_error_handler();
    gwdata_clear_error();
    /* DATA pointer back to the first DATA statement */
    bimisc_data_init();
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

/* Case-insensitive keyword match (ASCII): s[0..n) vs kw, both length n */
static bool gwsts_ieq_n(const char *s, const char *kw, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char a = s[i], b = kw[i];
        if (a >= 'a' && a <= 'z') a = (char)(a - 32);
        if (b >= 'a' && b <= 'z') b = (char)(b - 32);
        if (a != b) return false;
    }
    return true;
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
             !starts_with(trimmed, "ON ERROR GOTO ") &&
             !starts_with(trimmed, "ON ") &&
             !starts_with(trimmed, "ON")) {
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
    /* SCREEN statement: SCREEN <mode> (v138 M2: 0/1/2 → gengrp mode).
     * v141 LOW-1: compact "SCREEN1" allowed — keyword followed by space
     * or digit (GW-BASIC keywords don't require the space). */
    else if (starts_with(trimmed, "SCREEN") &&
             (trimmed[6] == ' ' || trimmed[6] == '\t' ||
              isdigit((unsigned char)trimmed[6]))) {
        const char *mp = trimmed + 6;
        while (*mp && isspace((unsigned char)*mp)) mp++;
        float mode = gweval_eval_numeric(mp);
        if (gwdata_get_error() == 0) {
            gengrp_set_screen_mode((int)mode);
            result = SUCCESS;
        } else {
            result = ERROR;
        }
    }
    /* PSET/PRESET statement: PSET[(x,y)][,color] — coordinates are
     * expressions (ASM SCAND); default color FORCLR/BAKCLR; out-of-bounds
     * silently skipped (ASM JAE PSTNOT) */
    else if (starts_with(trimmed, "PSET") || starts_with(trimmed, "PRESET")) {
        bool is_preset = starts_with(trimmed, "PRESET");
        const char *p = trimmed + (is_preset ? 6 : 4);
        while (*p && isspace((unsigned char)*p)) p++;
        int x = 0, y = 0, color = -1;
        if (*p == '(') {
            /* v141 MEDIUM-2: depth/quote-aware shared scanner (handles
             * nested parens like PSET(A(1),A(2))) */
            gwsts_parse_coord_pair(&p, &x, &y);
            /* optional ,color */
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') {
                p++;
                char cbuf[128];
                size_t cl = strlen(p);
                if (cl >= sizeof(cbuf)) cl = sizeof(cbuf) - 1;
                memcpy(cbuf, p, cl); cbuf[cl] = '\0';
                color = (int)gweval_eval_numeric(cbuf);
            }
        }
        if (gwdata_get_error() != 0) {
            result = ERROR;
        } else {
            if (is_preset) gengrp_preset(x, y, color);
            else gengrp_pset(x, y, color);
            result = SUCCESS;
        }
    }
    /* LINE statement: LINE [(x1,y1)]-(x2,y2)[,color[,B[F]]] — expression
     * coordinates; omitted first coord = current graphics cursor.
     * NOTE: must not shadow "LINE INPUT #n, var$" (its own branch below) */
    else if (starts_with(trimmed, "LINE") && !starts_with(trimmed, "LINE INPUT") &&
             (trimmed[4] == '\0' || trimmed[4] == ' ' || trimmed[4] == '\t' ||
              trimmed[4] == '(')) {
        /* v141 LOW-1: compact "LINE(" allowed — keyword must be followed by
         * space/tab/'('/end (not an identifier char, avoiding LINEABC hits) */
        const char *p = trimmed + 4;
        while (*p && isspace((unsigned char)*p)) p++;
        int x1 = 0, y1 = 0, x2 = 0, y2 = 0, color = -1;
        bool box = false, box_fill = false;
        bool has_first = false;
        /* v141 MEDIUM-2: depth/quote-aware shared scanner for both coords */
        if (*p == '(') {
            has_first = gwsts_parse_coord_pair(&p, &x1, &y1);
        }
        while (*p && isspace((unsigned char)*p)) p++;
        /* '-' 分隔符: 无论首坐标是否省略都要消费 (GW-BASIC LINE -(x,y)
         * 省略首坐标 = 从当前 graphics cursor 画) */
        if (*p == '-') p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '(') {
            gwsts_parse_coord_pair(&p, &x2, &y2);
        }
        /* [,color[,B[F]]] */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ',') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
            const char *seg = p;
            int depth = 0;
            while (*seg && (*seg != ',' || depth > 0)) {
                if (*seg == '(') depth++;
                else if (*seg == ')') depth--;
                seg++;
            }
            size_t cl = (size_t)(seg - p);
            char colorbuf[128];
            if (cl < sizeof(colorbuf)) {
                memcpy(colorbuf, p, cl); colorbuf[cl] = '\0';
                color = (int)gweval_eval_numeric(colorbuf);
            }
            p = seg;
            if (*p == ',') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == 'B' || *p == 'b') {
                    box = true;
                    p++;
                    if (*p == 'F' || *p == 'f') box_fill = true;
                }
            }
        }
        if (gwdata_get_error() != 0) {
            result = ERROR;
        } else {
            if (!has_first) {
                x1 = gengrp_get_grpacx();
                y1 = gengrp_get_grpacy();
            }
            if (color < 0) color = gengrp_get_forclr();
            gengrp_line_coords(x1, y1, x2, y2, color, box ? 1 : 0, box_fill ? 1 : 0);
            result = SUCCESS;
        }
    }
    /* CIRCLE statement: CIRCLE (x,y),radius[,color[,start[,end[,aspect]]]]
     * (ASM ADVGRP CIRCLE L460: SCAN1 coords, angles in radians, 0=east CCW;
     * negative angle draws radius line to center; aspect<1 scales Y) */
    else if (starts_with(trimmed, "CIRCLE") &&
             (trimmed[6] == ' ' || trimmed[6] == '\t' || trimmed[6] == '(')) {
        const char *p = trimmed + 6;
        while (*p && isspace((unsigned char)*p)) p++;
        int cx = 0, cy = 0;
        bool ok = false;
        if (*p == '(') {
            ok = gwsts_parse_coord_pair(&p, &cx, &cy);
        }
        int radius = 0, color = -1;
        float start_ang = 0.0f, end_ang = 6.283185f, aspect = 1.0f;
        if (ok) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
                /* radius expression up to top-level ',' or end */
                const char *seg = p;
                int depth = 0;
                while (*seg && (*seg != ',' || depth > 0)) {
                    if (*seg == '(') depth++;
                    else if (*seg == ')') depth--;
                    seg++;
                }
                size_t cl = (size_t)(seg - p);
                char rbuf[128];
                if (cl < sizeof(rbuf)) {
                    memcpy(rbuf, p, cl); rbuf[cl] = '\0';
                    radius = (int)gweval_eval_numeric(rbuf);
                }
                p = seg;
                /* [,color[,start[,end[,aspect]]]] */
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == ',') {
                    p++;
                    while (*p && isspace((unsigned char)*p)) p++;
                    seg = p; depth = 0;
                    while (*seg && (*seg != ',' || depth > 0)) {
                        if (*seg == '(') depth++;
                        else if (*seg == ')') depth--;
                        seg++;
                    }
                    cl = (size_t)(seg - p);
                    char cbuf[128];
                    if (cl < sizeof(cbuf)) {
                        memcpy(cbuf, p, cl); cbuf[cl] = '\0';
                        color = (int)gweval_eval_numeric(cbuf);
                    }
                    p = seg;
                    while (*p && isspace((unsigned char)*p)) p++;
                    if (*p == ',') {
                        p++;
                        while (*p && isspace((unsigned char)*p)) p++;
                        seg = p; depth = 0;
                        while (*seg && (*seg != ',' || depth > 0)) {
                            if (*seg == '(') depth++;
                            else if (*seg == ')') depth--;
                            seg++;
                        }
                        cl = (size_t)(seg - p);
                        char sbuf[128];
                        if (cl < sizeof(sbuf)) {
                            memcpy(sbuf, p, cl); sbuf[cl] = '\0';
                            start_ang = (float)gweval_eval_numeric(sbuf);
                        }
                        p = seg;
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (*p == ',') {
                            p++;
                            while (*p && isspace((unsigned char)*p)) p++;
                            seg = p; depth = 0;
                            while (*seg && (*seg != ',' || depth > 0)) {
                                if (*seg == '(') depth++;
                                else if (*seg == ')') depth--;
                                seg++;
                            }
                            cl = (size_t)(seg - p);
                            char ebuf[128];
                            if (cl < sizeof(ebuf)) {
                                memcpy(ebuf, p, cl); ebuf[cl] = '\0';
                                end_ang = (float)gweval_eval_numeric(ebuf);
                            }
                            p = seg;
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (*p == ',') {
                                p++;
                                char abuf[128];
                                size_t al = strlen(p);
                                if (al >= sizeof(abuf)) al = sizeof(abuf) - 1;
                                memcpy(abuf, p, al); abuf[al] = '\0';
                                aspect = (float)gweval_eval_numeric(abuf);
                            }
                        }
                    }
                }
            }
            if (gwdata_get_error() != 0) {
                result = ERROR;
            } else {
                advgrp_circle(cx, cy, radius, color, start_ang, end_ang, aspect);
                result = SUCCESS;
            }
        } else {
            result = ERROR;
        }
    }
    /* PAINT statement: PAINT (x,y)[,fill[,border]] (ASM ADVGRP PAINT L121) */
    else if (starts_with(trimmed, "PAINT") &&
             (trimmed[5] == ' ' || trimmed[5] == '\t' || trimmed[5] == '(')) {
            const char *p = trimmed + 5;
        while (*p && isspace((unsigned char)*p)) p++;
        int px = 0, py = 0;
        bool ok = false;
        if (*p == '(') {
            ok = gwsts_parse_coord_pair(&p, &px, &py);
        }
        int fill = -1, border = -1;
        if (ok) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == ',') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
                const char *seg = p;
                int depth = 0;
                while (*seg && (*seg != ',' || depth > 0)) {
                    if (*seg == '(') depth++;
                    else if (*seg == ')') depth--;
                    seg++;
                }
                size_t cl = (size_t)(seg - p);
                char fbuf[128];
                if (cl < sizeof(fbuf)) {
                    memcpy(fbuf, p, cl); fbuf[cl] = '\0';
                    fill = (int)gweval_eval_numeric(fbuf);
                }
                p = seg;
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == ',') {
                    p++;
                    char bbuf[128];
                    size_t bl = strlen(p);
                    if (bl >= sizeof(bbuf)) bl = sizeof(bbuf) - 1;
                    memcpy(bbuf, p, bl); bbuf[bl] = '\0';
                    border = (int)gweval_eval_numeric(bbuf);
                }
            }
            if (gwdata_get_error() != 0) {
                result = ERROR;
            } else {
                advgrp_paint(px, py, fill, border);
                result = SUCCESS;
            }
        } else {
            result = ERROR;
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
    /* DELETE statement: DELETE [start][-end] (ASM SCNLIN GWMAIN.ASM L842-876 +
     * DELETE GWLIST.ASM L778-812). Semantics per ASM:
     * - "DELETE 40"     → single line (SCNLIN SNGLIN path)
     * - "DELETE 40-60"  → range 40..60
     * - "DELETE 40-"    → open range to end (ALLLST: DX=65530 max)
     * - "DELETE -40"    → from start to 40 (SCNLIN: DX=0 initial)
     * - start > end     → FCERR (GWLIST.ASM FCERRG L805)
     * - non-numeric     → SYNTAX ERROR (LINGET MORLIN → SNERR L426) */
    else if (starts_with(trimmed, "DELETE")) {
        char *rest = trimmed + 6;  /* Skip "DELETE" */
        while (*rest && isspace((unsigned char)*rest)) rest++;

        int rc;
        if (!*rest) {
            /* Bare DELETE — ASM syntax error (no range given) */
            rc = ERROR;
        } else {
            /* Parse line numbers TEXTUALLY — the '-' is a RANGE separator, not
             * arithmetic. gweval_eval_numeric would evaluate "20-30" as -10. */
            char *dash = strchr(rest, '-');
            char start_buf[16];
            int start = 0, end = 0;
            if (dash) {
                size_t n = (size_t)(dash - rest);
                if (n >= sizeof(start_buf)) n = sizeof(start_buf) - 1;
                memcpy(start_buf, rest, n);
                start_buf[n] = '\0';
                /* MEDIUM-2: digits-only validation (ASM LINSPC/LINGET —
                 * non-digit → SNERR). Empty start = "DELETE -40" → 0. */
                int valid = 1;
                for (size_t k = 0; k < n; k++) {
                    if (!isdigit((unsigned char)start_buf[k])) { valid = 0; break; }
                }
                const char *endp = dash + 1;
                if (!*endp) {
                    /* MEDIUM-1: "DELETE 40-" → open range to end (ASM ALLLST 65530) */
                    end = 65530;
                } else {
                    /* MEDIUM-1: ASM LINGET — whitespace is a legal terminator.
                     * Scan digits, then skip trailing whitespace; anything
                     * else (or content after the whitespace) → SNERR. */
                    const char *q = endp;
                    while (*q && isdigit((unsigned char)*q)) q++;
                    while (*q && isspace((unsigned char)*q)) q++;
                    if (*q) {
                        valid = 0;  /* garbage after number/whitespace */
                    } else {
                        end = (q > endp) ? atoi(endp) : 0;
                    }
                }
                if (!valid) {
                    rc = ERROR;  /* SNERR */
                } else {
                    start = (n == 0) ? 0 : atoi(start_buf);
                    /* start > end → FCERR (ASM FCERRG); otherwise success.
                     * Must be INSIDE the valid branch — an unconditional check
                     * here overwrote the SNERR rc (start/end still 0). */
                    rc = (start > end) ? ERROR : SUCCESS;
                    if (rc == SUCCESS) {
                        gwlist_delete_range(start, end);
                    }
                }
            } else {
                /* MEDIUM-2: whole token must be digits; MEDIUM-1: trailing
                 * whitespace is a legal terminator (ASM LINGET). */
                int valid = (*rest != '\0');
                const char *q = rest;
                while (*q && isdigit((unsigned char)*q)) q++;
                while (*q && isspace((unsigned char)*q)) q++;
                if (*q) valid = 0;  /* garbage after number/whitespace */
                if (!valid) {
                    rc = ERROR;  /* SNERR */
                    goto delete_done;
                }
                start = atoi(rest);
                end = start;  /* Single line form */
                /* start > end impossible here (end == start) — success. */
                rc = SUCCESS;
                gwlist_delete_range(start, end);
            }
        }
delete_done:
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
    /* ON <expr> GOTO/GOSUB — computed branch (ASM NTONG1/LOOPON L2446-2460).
     * 1-based index selects the target from a comma-separated list; index out
     * of range (or 0) → list exhausted → next statement (ASM L2453-2456 RET).
     * ON I GOSUB pushes a GOSUB return (ASM ISGOSU/SETGSB). */
    else if (starts_with(trimmed, "ON") &&
             !starts_with(trimmed, "ON ERROR")) {
        char *rest = trimmed + 2;  /* Skip "ON" (v130 LOW-1: 紧凑写法兼容) */
        while (*rest && isspace((unsigned char)*rest)) rest++;

        /* v131 MEDIUM-1: statement boundary — the ON statement ends at the
         * first colon (multi-statement line) or end of line. Keyword search
         * and target parsing must NOT see beyond it (": GOTO 500" or
         * ": REM GOTO later" would pollute the parse).
         * v132 MEDIUM-1: quote-aware scan — colons inside double-quoted
         * string literals (e.g. INSTR(A$, ":")) are NOT statement separators.
         * GW-BASIC has no escape sequences; "" inside a literal is itself
         * literal content (still in-string), so a simple in_string toggle on
         * '"' is exactly the ASM semantics. */
        {
            int in_string = 0;
            char *p;
            for (p = rest; *p; p++) {
                if (*p == '"') {
                    in_string = !in_string;
                } else if (*p == ':' && !in_string) {
                    *p = '\0';
                    break;
                }
            }
        }

        /* Find the GOTO/GOSUB keyword — v130 MEDIUM-1: search BACKWARD and
         * require a token boundary (preceding char not a letter/digit part of
         * a variable name). Variable names may contain GOTO/GOSUB substrings
         * (XGOTO, MYGOSUB are legal); a naive strstr truncates the expr. */
        char *kw = NULL;
        int is_gosub = 0;
        for (char *q = rest + strlen(rest); q >= rest; q--) {
            if (strncmp(q, "GOSUB", 5) == 0) {
                kw = q;
                is_gosub = 1;
                break;
            }
            if (strncmp(q, "GOTO", 4) == 0) {
                kw = q;
                is_gosub = 0;
                break;
            }
        }
        /* Token-boundary check: the char before the keyword must NOT be part
         * of a variable name (letter/digit/underscore/$). Backward search finds
         * the LAST occurrence — for "ON XGOTO GOTO 100" the last "GOTO" is the
         * real keyword (preceded by space); the one inside XGOTO is skipped.
         * If the ONLY match is inside a variable name (e.g. "ON XGOTO 100"),
         * the boundary check rejects it → SNERR. */
        if (kw) {
            /* v131 LOW-1: kw == rest also rejected — "ON GOTO 100" (no index
             * expression) and "ON GOTOX ..." (variable name at start) are both
             * illegal → SNERR. */
            if (kw == rest) {
                kw = NULL;
            } else {
                char prev = *(kw - 1);
                /* Letter/underscore/$ before keyword → variable name (XGOTO) —
                 * not the keyword. Digit before keyword → compact index form
                 * "ON 2GOTO 100" (ASM GETBYT consumes digits, then GOTO) — OK. */
                if (isalpha((unsigned char)prev) || prev == '_' || prev == '$') {
                    kw = NULL;  /* Inside a variable name — not the keyword */
                }
            }
        }

        if (!kw) {
            result = ERROR;  /* SNERR - no GOTO/GOSUB */
        } else {
            /* Evaluate the index expression (text before the keyword) */
            char expr_buf[128];
            size_t expr_len = (size_t)(kw - rest);
            if (expr_len >= sizeof(expr_buf)) expr_len = sizeof(expr_buf) - 1;
            memcpy(expr_buf, rest, expr_len);
            expr_buf[expr_len] = '\0';

            int index = (int)gweval_eval_numeric(expr_buf);

            /* Parse comma-separated target line numbers (after keyword) */
            char *p = kw + (is_gosub ? 5 : 4);  /* Skip "GOSUB" / "GOTO" */
            int targets[64];
            int count = 0;
            while (*p && count < 64) {
                while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                if (!isdigit((unsigned char)*p)) break;
                int num = 0;
                while (*p && isdigit((unsigned char)*p)) {
                    num = num * 10 + (*p - '0');
                    p++;
                }
                targets[count++] = num;
            }

            /* ASM NTONG1: 1-based selection; out of range → next statement */
            if (index >= 1 && index <= count) {
                int target = targets[index - 1];
                if (is_gosub) {
                    /* GOSUB semantics: push return, jump to target (ASM SETGSB) */
                    extern int next86_gosub_to_line(int line_num);
                    result = next86_gosub_to_line(target);
                } else {
                    /* GOTO semantics: plain jump (ASM GONE2 → GOTO) */
                    result = target;
                }
            } else {
                result = SUCCESS;  /* Out of range → next statement */
            }
        }
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
                                } else if (gwdata_get_error() != 0) {
                                    /* String eval raised an error (e.g. OM on
                                     * literal alloc with exhausted heap) —
                                     * abort PRINT, let statement loop handle
                                     * the error (v137: error-aware PRINT). */
                                    ;
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
    /* IF statement: IF <cond> THEN <stmt> [ELSE <stmt>]
     * v138 MEDIUM-2: single-line ELSE branch (was silently dropped).
     * v138 MEDIUM-1: compound conditions — split at top-level OR then AND
     * (GW-BASIC precedence), each atom evaluated with string semantics when
     * both comparison sides are strings, numeric otherwise. The old code
     * took only the first comparison and discarded the AND/OR tail, and
     * never executed the ELSE branch. */
    else if (starts_with(trimmed, "IF ")) {
        /* 1. Quote/paren-aware scan for top-level THEN / ELSE keywords */
        char *then_pos = NULL;
        char *else_pos = NULL;
        {
            bool in_quote = false;
            int depth = 0;
            for (char *s = (char *)trimmed + 3; *s; s++) {
                if (*s == '"') { in_quote = !in_quote; continue; }
                if (in_quote) continue;
                if (*s == '(') { depth++; continue; }
                if (*s == ')') { if (depth > 0) depth--; continue; }
                if (depth == 0) {
                    if (!then_pos && gwsts_ieq_n(s, " THEN ", 6)) {
                        then_pos = s;
                        s += 5;
                        continue;
                    }
                    if (then_pos && !else_pos && gwsts_ieq_n(s, " ELSE ", 6)) {
                        else_pos = s;
                        s += 5;
                        continue;
                    }
                }
            }
        }
        if (then_pos) {
            /* 2. Split: NUL at ELSE first, then at THEN */
            char *else_stmt = NULL;
            if (else_pos) {
                *else_pos = '\0';
                else_stmt = strdup_trim(else_pos + 6);
            }
            *then_pos = '\0';
            char *cond_expr = strdup_trim(trimmed + 3);
            char *then_stmt = strdup_trim(then_pos + 6);
            if (cond_expr && then_stmt) {
                /* 3. Evaluate the condition.
                 * eval_atom: -1 true, 0 false, -2 unknown/error. */
                #define GWSIFS_MAX_ATOMS 16
                char atoms[GWSIFS_MAX_ATOMS][256];
                int  atom_op[GWSIFS_MAX_ATOMS];   /* 0=AND (binds tighter), 1=OR */
                int  atom_count = 0;
                bool split_ok = true;
                {
                    int depth = 0;
                    bool in_quote = false;
                    int start = 0;
                    size_t clen = strlen(cond_expr);
                    for (size_t ci = 0; ci <= clen && split_ok; ci++) {
                        char cc = cond_expr[ci];
                        if (ci < clen) {
                            if (cc == '"') { in_quote = !in_quote; continue; }
                            if (in_quote) continue;
                            if (cc == '(') { depth++; continue; }
                            if (cc == ')') { if (depth > 0) depth--; continue; }
                            if (depth != 0) continue;
                        }
                        /* top-level: check AND/OR word boundaries */
                        bool prev_nonword = (ci == 0) ||
                            (!isalnum((unsigned char)cond_expr[ci-1]) && cond_expr[ci-1] != '_');
                        if (prev_nonword && gwsts_ieq_n(&cond_expr[ci], "AND", 3) &&
                            (ci + 3 >= clen || (!isalnum((unsigned char)cond_expr[ci+3]) && cond_expr[ci+3] != '_'))) {
                            if (atom_count >= GWSIFS_MAX_ATOMS) { split_ok = false; break; }
                            size_t e = ci;
                            while (e > (size_t)start && cond_expr[e-1] == ' ') e--;
                            size_t b = start;
                            while (b < e && cond_expr[b] == ' ') b++;
                            size_t copy = e - b;
                            if (copy == 0 || copy >= 256) { split_ok = false; break; }
                            memcpy(atoms[atom_count], &cond_expr[b], copy);
                            atoms[atom_count][copy] = '\0';
                            atom_count++;
                            /* op sits between the just-appended atom and the
                             * NEXT one → index atom_count-1 */
                            if (atom_count >= 1) atom_op[atom_count - 1] = 0;
                            start = (int)(ci + 3);
                            ci += 2;
                        } else if (prev_nonword && gwsts_ieq_n(&cond_expr[ci], "OR", 2) &&
                            (ci + 2 >= clen || (!isalnum((unsigned char)cond_expr[ci+2]) && cond_expr[ci+2] != '_'))) {
                            if (atom_count >= GWSIFS_MAX_ATOMS) { split_ok = false; break; }
                            size_t e = ci;
                            while (e > (size_t)start && cond_expr[e-1] == ' ') e--;
                            size_t b = start;
                            while (b < e && cond_expr[b] == ' ') b++;
                            size_t copy = e - b;
                            if (copy == 0 || copy >= 256) { split_ok = false; break; }
                            memcpy(atoms[atom_count], &cond_expr[b], copy);
                            atoms[atom_count][copy] = '\0';
                            atom_count++;
                            if (atom_count >= 1) atom_op[atom_count - 1] = 1;
                            start = (int)(ci + 2);
                            ci += 1;
                        }
                    }
                    /* tail atom */
                    if (split_ok) {
                        size_t e = clen;
                        while (e > (size_t)start && cond_expr[e-1] == ' ') e--;
                        size_t b = start;
                        while (b < e && cond_expr[b] == ' ') b++;
                        size_t copy = e - b;
                        if (copy > 0) {
                            if (atom_count >= GWSIFS_MAX_ATOMS || copy >= 256) {
                                split_ok = false;
                            } else {
                                memcpy(atoms[atom_count], &cond_expr[b], copy);
                                atoms[atom_count][copy] = '\0';
                                atom_count++;
                            }
                        } else if (atom_count == 0) {
                            split_ok = false;  /* empty condition */
                        }
                    }
                }

                int then_taken = 0;
                bool use_atoms = split_ok && atom_count > 0;
                if (use_atoms) {
                    /* eval each atom */
                    int aval[GWSIFS_MAX_ATOMS];
                    for (int ai = 0; ai < atom_count && use_atoms; ai++) {
                        const char *atom = atoms[ai];
                        int cv = -2;
                        /* find top-level comparison op */
                        const char *ops[] = {"<=", ">=", "<>", "=", "<", ">"};
                        const char *opf = NULL; size_t opl = 0, opa = 0;
                        int depth = 0; bool inq = false;
                        for (size_t ci = 0; atom[ci]; ci++) {
                            char c2 = atom[ci];
                            if (c2 == '"') { inq = !inq; continue; }
                            if (inq) continue;
                            if (c2 == '(') { depth++; continue; }
                            if (c2 == ')') { if (depth > 0) depth--; continue; }
                            if (depth == 0) {
                                for (int oi = 0; oi < 6; oi++) {
                                    size_t l = strlen(ops[oi]);
                                    if (strncmp(&atom[ci], ops[oi], l) == 0) {
                                        if (!opf || l > opl) { opf = ops[oi]; opl = l; opa = ci; }
                                        ci += l - 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (opf) {
                            char lb[256], rb[256];
                            size_t ll = opa, ro = opa + opl;
                            if (ll < sizeof(lb) && strlen(atom) - ro < sizeof(rb)) {
                                memcpy(lb, atom, ll); lb[ll] = '\0';
                                strcpy(rb, atom + ro);
                                char *lt = strdup_trim(lb);
                                char *rt = strdup_trim(rb);
                                char *ls = lt ? gweval_eval_string(lt) : NULL;
                                char *rs = rt ? gweval_eval_string(rt) : NULL;
                                if (ls && rs && gwdata_get_error() == 0) {
                                    int cmp = strcmp(ls, rs);
                                    if (strcmp(opf, "=") == 0) cv = (cmp == 0) ? -1 : 0;
                                    else if (strcmp(opf, "<>") == 0) cv = (cmp != 0) ? -1 : 0;
                                    else if (strcmp(opf, "<") == 0) cv = (cmp < 0) ? -1 : 0;
                                    else if (strcmp(opf, "<=") == 0) cv = (cmp <= 0) ? -1 : 0;
                                    else if (strcmp(opf, ">") == 0) cv = (cmp > 0) ? -1 : 0;
                                    else cv = (cmp >= 0) ? -1 : 0;
                                }
                                if (lt) gwram_free(lt);
                                if (rt) gwram_free(rt);
                            }
                        }
                        if (cv == -2) {
                            /* numeric atom (handles numeric comparisons and
                             * plain values); error → whole-cond fallback */
                            gwdata_clear_error();
                            float nv = gweval_eval_numeric(atom);
                            if (gwdata_get_error() == 0) cv = (nv != 0.0f) ? -1 : 0;
                            else cv = -2;
                        }
                        if (cv == -2) { use_atoms = false; break; }
                        aval[ai] = cv;
                    }
                    if (use_atoms) {
                        /* AND binds tighter than OR: fold AND groups first */
                        int val = aval[0];
                        int or_val = -2;
                        for (int ai = 0; ai < atom_count - 1; ai++) {
                            if (atom_op[ai] == 0) {
                                val = (val != 0 && aval[ai + 1] != 0) ? -1 : 0;
                            } else {
                                if (or_val == -2) or_val = val;
                                else or_val = (or_val != 0 || val != 0) ? -1 : 0;
                                val = aval[ai + 1];
                            }
                        }
                        if (or_val == -2) {
                            then_taken = (val != 0);
                        } else {
                            or_val = (or_val != 0 || val != 0) ? -1 : 0;
                            then_taken = (or_val != 0);
                        }
                    }
                }
                if (!use_atoms) {
                    /* fallback: whole-condition numeric eval */
                    gwdata_clear_error();
                    float cond_val = gweval_eval_numeric(cond_expr);
                    then_taken = (cond_val != 0.0f);
                }
                #undef GWSIFS_MAX_ATOMS

                /* 4. THEN / ELSE dispatch (v138 MEDIUM-2) */
                /* helper: pure-numeric THEN/ELSE target = line number jump
                 * (GW-BASIC: IF..THEN <line> is a GOTO) */
                #define GWSIFS_IS_LINENUM(s_) (s_ != NULL && *s_ != '\0' && \
                    strspn(s_, "0123456789") == strlen(s_))
                if (then_taken) {
                    if (GWSIFS_IS_LINENUM(then_stmt)) {
                        result = atoi(then_stmt);   /* jump target (>0) */
                    } else {
                        result = gwsts_exec_statement(then_stmt);
                    }
                } else if (else_stmt) {
                    if (GWSIFS_IS_LINENUM(else_stmt)) {
                        result = atoi(else_stmt);   /* jump target (>0) */
                    } else {
                        result = gwsts_exec_statement(else_stmt);
                    }
                } else {
                    result = SUCCESS;
                }
                #undef GWSIFS_IS_LINENUM
                if (else_stmt) gwram_free(else_stmt);
                gwram_free(cond_expr);
                gwram_free(then_stmt);
            } else {
                if (cond_expr) gwram_free(cond_expr);
                if (then_stmt) gwram_free(then_stmt);
                if (else_stmt) gwram_free(else_stmt);
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
                char *first_expr = (char *)gwram_alloc(fn_len + 1);
                if (first_expr) {
                    memcpy(first_expr, rest, fn_len);
                    first_expr[fn_len] = '\0';
                    
                    rest = filename_end + 1;
                    while (*rest && isspace((unsigned char)*rest)) rest++;
                    
                    /* v149 MEDIUM-1: ASM GIO86.ASM L59-101 — two forms:
                     *   OPEN filespec FOR mode AS #fnum [LEN=...]   (new-style)
                     *   OPEN mode,fnum,filespec[,reclen]            (classic, mode-first)
                     * The first expression followed by a comma is the MODE
                     * character ("O"/"I"/"A"/"R", case-insensitive per ASM
                     * AND -1-" "); anything else is the FILESPEC. */
                    int mode = 0;  /* FM_* */
                    int filenum = 1;
                    int reclen = 128;
                    bool has_for = false;
                    
                    if (*rest == ',') {
                        /* ---- classic mode-first: OPEN mode,fnum,filespec[,reclen] ---- */
                        char mc = first_expr[0];
                        if (mc >= 'a' && mc <= 'z') mc = (char)(mc - 'a' + 'A');
                        if (strlen(first_expr) != 1 ||
                            (mc != 'O' && mc != 'I' && mc != 'A' && mc != 'R')) {
                            /* ASM ERBFM0: "Bad file mode" */
                            gwram_free(first_expr);
                            return ERROR;
                        }
                        switch (mc) {
                            case 'O': mode = FM_OUTPUT; break;
                            case 'I': mode = FM_INPUT;  break;
                            case 'A': mode = FM_APPEND; break;
                            default:  mode = FM_RANDOM; break;
                        }
                        rest++;  /* skip comma */
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        /* optional '#' before file number (ASM POFNUM) */
                        if (*rest == '#') { rest++; while (*rest && isspace((unsigned char)*rest)) rest++; }
                        filenum = 0;
                        while (*rest && isdigit((unsigned char)*rest)) {
                            filenum = filenum * 10 + (*rest - '0');
                            rest++;
                        }
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        if (*rest != ',') {
                            gwram_free(first_expr);
                            return ERROR;  /* SNERR — syntax error */
                        }
                        rest++;  /* skip comma */
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        /* filespec: quoted string */
                        if (*rest != '\"') {
                            gwram_free(first_expr);
                            return ERROR;  /* SNERR */
                        }
                        rest++;
                        char *fs_end = strchr(rest, '\"');
                        if (!fs_end) {
                            gwram_free(first_expr);
                            return ERROR;  /* SNERR */
                        }
                        size_t fs_len = fs_end - rest;
                        char *filespec = (char *)gwram_alloc(fs_len + 1);
                        if (!filespec) {
                            gwram_free(first_expr);
                            return ERROR;
                        }
                        memcpy(filespec, rest, fs_len);
                        filespec[fs_len] = '\0';
                        rest = fs_end + 1;
                        while (*rest && isspace((unsigned char)*rest)) rest++;
                        /* optional ,reclen (ASM PORLEN) */
                        if (*rest == ',') {
                            rest++;
                            while (*rest && isspace((unsigned char)*rest)) rest++;
                            reclen = 0;
                            while (*rest && isdigit((unsigned char)*rest)) {
                                reclen = reclen * 10 + (*rest - '0');
                                rest++;
                            }
                            mode = FM_RANDOM;
                        }
                        result = dskcom_open(filespec, filenum, mode, reclen);
                        gwram_free(filespec);
                        gwram_free(first_expr);
                        /* fall through to the tail of the OPEN branch */
                        goto gwsts_open_done;
                    }
                    
                    /* ---- new-style: filespec first ---- */
                    {
                    char *filename = first_expr;  /* reuse */
                    
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
                    }  /* end new-style block */
                    gwram_free(first_expr);
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
    /* PUT statement: PUT #filenum,... (file) or PUT (x,y),arr[,action] (gfx) */
    else if (starts_with(trimmed, "PUT")) {
        const char *q = trimmed + 3;
        while (*q && isspace((unsigned char)*q)) q++;
        if (*q == '(') {
            result = gwsts_handle_gfx_put(trimmed);
        } else {
            result = gwsts_handle_put(trimmed);
        }
    }
    /* GET statement: GET #filenum,... (file) or GET (x1,y1)-(x2,y2),arr (gfx) */
    else if (starts_with(trimmed, "GET")) {
        const char *q = trimmed + 3;
        while (*q && isspace((unsigned char)*q)) q++;
        if (*q == '(') {
            result = gwsts_handle_gfx_get(trimmed);
        } else {
            result = gwsts_handle_get(trimmed);
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
    
gwsts_open_done:;
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
                /* v138: evaluate RHS BEFORE alloc_variable — alloc on an
                 * EXISTING string variable clears value.s.ptr (gwdata.c
                 * L82-84), so self-referential assignment S$ = S$ + "X"
                 * read the CLEARED (NULL) value. Numeric path already
                 * evaluates first (same file, numeric branch). */
                char *str_val = gweval_eval_string(expr);
                struct variable *v = gwdata_alloc_variable(base_name, VT_STRING);
                if (v) {
                    if (str_val) {
                        v->value.s.ptr = str_val;
                        v->value.s.len = strlen(str_val);
                        result = SUCCESS;
                    } else if (gwdata_get_error() != 0) {
                        /* String function raised FCERR/SNERR (e.g. CHR$(256),
                         * MID$(s,0)) — do NOT fall back to literal; the error
                         * propagates at statement level (v137 audit). */
                        result = ERROR;
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