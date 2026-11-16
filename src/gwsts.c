/* GWSTS - Statement Execution Engine */
/* Based on GWSTS.ASM */

#include "gwbasic.h"
#include <math.h>
#include <io.h>
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
int assign_value_raw(const char *var_ref, const char *expr);  /* v162 HIGH-1 */

/* v161 MEDIUM-3: line-start tracking for print-zone columns */
static long g_line_start = 0;
long gwsts_last_line_start(void) { return g_line_start; }
void gwsts_note_newline(void) {
    long pos = ftell(stdout);
    if (pos >= 0) g_line_start = pos;
}

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
    /* v156 obs-5: graphics statement in text mode → error (ASM FCERR);
     * v156 MEDIUM-1: set code/line so ON ERROR + RESUME NEXT works */
    if (gengrp_get_screen_mode() == 0) {
        gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
        return ERROR;
    }
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
    /* v156 obs-5: graphics statement in text mode → error (ASM FCERR);
     * v156 MEDIUM-1: set code/line so ON ERROR + RESUME NEXT works */
    if (gengrp_get_screen_mode() == 0) {
        gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
        return ERROR;
    }
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

/* ===== v152: OPEN statement (extracted from gwsts_exec_statement) =====
 * ASM GIO86.ASM L59-101 — two forms, both parsed with FRMEVL
 * expressions (NAMSCN calls FRMEVL for the filespec):
 *   OPEN filespec FOR mode AS #fnum [LEN=...]
 *   OPEN mode,fnum,filespec[,reclen]     (classic, mode-first)
 * reclen is only parsed for RANDOM mode (ASM PRLENC/PNRLEN); a reclen
 * parm on O/I/A is a syntax error (SNERR). reclen <= 0 → FCERR (VARECS). */
int gwsts_exec_open(const char *trimmed) {
    int result = ERROR;
    char *first_expr = NULL;
    char *filespec = NULL;
    char *rest = (char *)trimmed + 5;
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    /* --- Phase 1: first expression (quoted literal OR expression up
     *     to a top-level comma — quote/paren aware) --- */
    {
        const char *sp = rest;
        bool in_q = false;
        int depth = 0;
        const char *e = sp;
        bool quoted = (*sp == '"');
        if (quoted) {
            e = strchr(sp + 1, '"');
            if (!e) return ERROR;  /* unterminated literal */
            e++;  /* past closing quote */
        } else {
            while (*e) {
                char cc = *e;
                if (cc == '"') { in_q = !in_q; e++; continue; }
                if (in_q) { e++; continue; }
                if (cc == '(') { depth++; e++; continue; }
                if (cc == ')') { if (depth > 0) depth--; e++; continue; }
                if (depth == 0 && cc == ',') break;
                /* new-style keywords end the expression (ASM FRMEVL
                 * stops at FOR/AS tokens): top-level " FOR " / " AS" */
                if (depth == 0 && e[0] == ' ' &&
                    (starts_with(e, " FOR ") ||
                     (starts_with(e, " AS") &&
                      (isspace((unsigned char)e[3]) || e[3] == '#')))) {
                    break;
                }
                e++;
            }
        }
        const char *et = e;
        if (quoted) {
            /* literal content between quotes */
            const char *ls = sp + 1, *le = e - 1;
            first_expr = (char *)gwram_alloc((size_t)(le - ls) + 1);
            if (!first_expr) return ERROR;
            memcpy(first_expr, ls, (size_t)(le - ls));
            first_expr[le - ls] = '\0';
        } else {
            while (et > sp && isspace((unsigned char)et[-1])) et--;
            if (et == sp) return ERROR;  /* empty expression */
            first_expr = (char *)gwram_alloc((size_t)(et - sp) + 1);
            if (!first_expr) return ERROR;
            memcpy(first_expr, sp, (size_t)(et - sp));
            first_expr[et - sp] = '\0';
        }
        rest = (char *)e;
    }
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    int mode = 0;      /* FM_* */
    int filenum = 1;
    int reclen = 128;
    bool has_for = false;
    
    if (*rest == ',') {
        /* ---- classic mode-first: OPEN mode,fnum,filespec[,reclen] ---- */
        char mc = first_expr[0];
        if (mc >= 'a' && mc <= 'z') mc = (char)(mc - 'a' + 'A');
        if (strlen(first_expr) != 1 ||
            (mc != 'O' && mc != 'I' && mc != 'A' && mc != 'R')) {
            gwram_free(first_expr);
            return ERROR;  /* ASM ERBFM0 "Bad file mode" */
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
        if (*rest != ',') { gwram_free(first_expr); return ERROR; }  /* SNERR */
        rest++;  /* skip comma */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* filespec: quoted literal OR expression to top-level comma/end
         * (v152 obs-2: ASM NAMSCN uses FRMEVL — variables work) */
        bool fs_owned = false;
        if (*rest == '"') {
            rest++;
            const char *fs_end = strchr(rest, '"');
            if (!fs_end) { gwram_free(first_expr); return ERROR; }
            size_t fs_len = (size_t)(fs_end - rest);
            filespec = (char *)gwram_alloc(fs_len + 1);
            if (!filespec) { gwram_free(first_expr); return ERROR; }
            memcpy(filespec, rest, fs_len);
            filespec[fs_len] = '\0';
            rest = (char *)fs_end + 1;
            fs_owned = true;
        } else {
            const char *sp = rest;
            bool in_q = false;
            const char *e = sp;
            while (*e) {
                if (*e == '"') { in_q = !in_q; e++; continue; }
                if (!in_q && *e == ',') break;
                e++;
            }
            const char *et = e;
            while (et > sp && isspace((unsigned char)et[-1])) et--;
            if (et == sp) { gwram_free(first_expr); return ERROR; }
            size_t fs_len = (size_t)(et - sp);
            char *fs_expr = (char *)gwram_alloc(fs_len + 1);
            if (!fs_expr) { gwram_free(first_expr); return ERROR; }
            memcpy(fs_expr, sp, fs_len);
            fs_expr[fs_len] = '\0';
            gwdata_clear_error();
            filespec = gweval_eval_string(fs_expr);
            gwram_free(fs_expr);
            if (!filespec || gwdata_get_error() != 0) {
                gwram_free(first_expr);
                return ERROR;
            }
            rest = (char *)e;
        }
        while (*rest && isspace((unsigned char)*rest)) rest++;
        
        /* optional ,reclen — RANDOM only (v152 obs-3: ASM PRLENC) */
        if (*rest == ',') {
            if (mode != FM_RANDOM) {
                if (fs_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;  /* SNERR */
            }
            rest++;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            gwdata_clear_error();
            float rv = gweval_eval_numeric(rest);
            if (gwdata_get_error() != 0) {
                if (fs_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;
            }
            reclen = (int)rv;
            /* ASM VARECS/PORLEN: 0 illegal (FCERR); INTID2 rejects
             * values whose high bit is set → >32767 → FCERR */
            if (reclen <= 0 || reclen > 32767) {
                if (fs_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;
            }
        }
        result = dskcom_open(filespec, filenum, mode, reclen);
        if (fs_owned) gwram_free(filespec);
        gwram_free(first_expr);
        return result;
    }
    
    /* ---- new-style: filespec expr + [FOR mode] AS #fnum [LEN=...] ---- */
    {
        /* filespec: first_expr holds a quoted literal (plain text) OR
         * an expression (F$ / A$+".TXT") — for the expression form,
         * re-scan the original text from `rest` backward? No: phase 1
         * already consumed it. For the expression case, first_expr is
         * the RAW EXPRESSION TEXT — evaluate it here. */
        if (!first_expr) return ERROR;
        bool ns_owned = false;
        /* Determine if first_expr was a literal: we can't tell here, so
         * re-parse — if the ORIGINAL text at the captured position was
         * quoted we took the content; otherwise first_expr is raw expr
         * text that must be evaluated. Track via a flag set in phase 1
         * instead: use presence of surrounding context — simplest: keep
         * a module-level decision by re-checking trimmed[5]=='"'. */
        if (trimmed[5] == '"' ) {
            const char *p = trimmed + 5;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p != '"') {
                /* expression form — evaluate */
                gwdata_clear_error();
                filespec = gweval_eval_string(first_expr);
                if (!filespec || gwdata_get_error() != 0) {
                    gwram_free(first_expr);
                    return ERROR;
                }
            } else {
                filespec = first_expr;  /* literal content */
                first_expr = NULL;
                ns_owned = true;
            }
        } else {
            /* trimmed[5] was whitespace — find the first non-space */
            const char *p = trimmed + 5;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p != '"') {
                gwdata_clear_error();
                filespec = gweval_eval_string(first_expr);
                if (!filespec || gwdata_get_error() != 0) {
                    gwram_free(first_expr);
                    return ERROR;
                }
            } else {
                filespec = first_expr;
                first_expr = NULL;
            }
        }
        
        if (starts_with(rest, "FOR ")) {
            rest += 4;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            if (starts_with(rest, "INPUT")) {
                mode = FM_INPUT; rest += 5; has_for = true;
            } else if (starts_with(rest, "OUTPUT")) {
                mode = FM_OUTPUT; rest += 6; has_for = true;
            } else if (starts_with(rest, "APPEND")) {
                mode = FM_APPEND; rest += 6; has_for = true;
            } else if (starts_with(rest, "RANDOM")) {
                mode = FM_RANDOM; rest += 6; has_for = true;
            }
        }
        if (has_for) {
            while (*rest && isspace((unsigned char)*rest)) rest++;
        } else {
            mode = FM_RANDOM;  /* no FOR → default RANDOM (ASM NOTNSO) */
        }
        
        /* AS #filenum */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        if (starts_with(rest, "AS")) {
            rest += 2;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            if (*rest == '#') rest++;
            filenum = 0;
            while (*rest && isdigit((unsigned char)*rest)) {
                filenum = filenum * 10 + (*rest - '0');
                rest++;
            }
            while (*rest && isspace((unsigned char)*rest)) rest++;
        }
        
        /* LEN= reclen — RANDOM only (v152 obs-3: ASM PNRLEN) */
        while (*rest && isspace((unsigned char)*rest)) rest++;
        if (starts_with(rest, "LEN=") || starts_with(rest, "LEN =")) {
            if (mode != FM_RANDOM) {
                if (ns_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;  /* SNERR */
            }
            rest += (rest[3] == '=') ? 4 : 5;
            while (*rest && isspace((unsigned char)*rest)) rest++;
            gwdata_clear_error();
            float rv = gweval_eval_numeric(rest);
            if (gwdata_get_error() != 0) {
                if (ns_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;
            }
            reclen = (int)rv;
            if (reclen <= 0 || reclen > 32767) {  /* ASM INTID2 FCERR */
                if (ns_owned) gwram_free(filespec);
                gwram_free(first_expr);
                return ERROR;  /* FCERR */
            }
        }
        
        result = dskcom_open(filespec, filenum, mode, reclen);
        if (ns_owned) gwram_free(filespec);
        gwram_free(first_expr);
    }
    return result;
}

/* ===== v158: PRINT USING statement handler (ASM BIPRTU PRINUS) =====
 * rest = text after "PRINT USING". Grammar:
 *   USING <fmt-expr> ; <value> [{;|,} <value>...]
 * Returns SUCCESS/ERROR; *trailing_sep_out = 1 if the value list ends
 * with ';' (no newline), 2 if ',' (zone), 0 otherwise. */
static int gwsts_print_using(const char *rest, int *trailing_sep_out) {
    *trailing_sep_out = 0;
    while (*rest == ' ' || *rest == '\t') rest++;
    /* fmt-expr: up to the top-level ';' (quote/paren aware) */
    const char *fp = rest;
    bool in_q = false;
    int depth = 0;
    while (*fp && !(!in_q && depth == 0 && *fp == ';')) {
        if (*fp == '"') in_q = !in_q;
        else if (!in_q && *fp == '(') depth++;
        else if (!in_q && *fp == ')') depth--;
        fp++;
    }
    if (in_q || depth != 0 || *fp != ';') {
        /* v159 MEDIUM-2: missing ';' → SNERR (error 2) */
        gwdata_set_error(2, g_state ? (uint16_t)g_state->current_line : 0, 0);
        return ERROR;
    }
    const char *fend = fp;
    while (fend > rest && isspace((unsigned char)fend[-1])) fend--;
    if (fend == rest) return ERROR;  /* empty fmt expr */
    size_t flen = (size_t)(fend - rest);
    char *fmt_expr = (char *)gwram_alloc(flen + 1);
    if (!fmt_expr) return ERROR;
    memcpy(fmt_expr, rest, flen);
    fmt_expr[flen] = '\0';
    gwdata_clear_error();
    char *fmt_val = gweval_eval_string(fmt_expr);
    gwram_free(fmt_expr);
    if (!fmt_val || gwdata_get_error() != 0) {
        if (gwdata_get_error() == 0) {
            /* fmt evaluated but not a string → SNERR */
            gwdata_set_error(2, g_state ? (uint16_t)g_state->current_line : 0, 0);
        }
        return ERROR;
    }

    /* value list — v159 HIGH-7: dynamically sized (no 64 cap) */
    int vcap = 64;
    struct using_value *vals = (struct using_value *)gwram_alloc(
        sizeof(struct using_value) * vcap);
    if (!vals) return ERROR;
    int vcount = 0;
    const char *vp = fp + 1;
    int trailing = 0;
    while (*vp) {
        if (vcount >= vcap) {
            int ncap = vcap * 2;
            struct using_value *nv = (struct using_value *)gwram_alloc(
                sizeof(struct using_value) * ncap);
            if (!nv) {
                /* v161 LOW-4: expansion failure = Out of memory (7),
                 * not a silent truncation */
                gwdata_set_error(7, g_state ? (uint16_t)g_state->current_line : 0, 0);
                gwram_free(vals);
                *trailing_sep_out = trailing;
                return ERROR;
            }
            memcpy(nv, vals, sizeof(struct using_value) * vcount);
            gwram_free(vals);
            vals = nv;
            vcap = ncap;
        }
        while (*vp == ' ' || *vp == '\t') vp++;
        if (!*vp) break;
        const char *vs = vp;
        bool vin_q = false;
        int vdepth = 0;
        while (*vp && !(!vin_q && vdepth == 0 && (*vp == ',' || *vp == ';'))) {
            if (*vp == '"') vin_q = !vin_q;
            else if (!vin_q && *vp == '(') vdepth++;
            else if (!vin_q && *vp == ')') vdepth--;
            vp++;
        }
        const char *ve = vp;
        while (ve > vs && isspace((unsigned char)ve[-1])) ve--;
        if (*vp == ',') trailing = 2;
        else if (*vp == ';') trailing = 1;
        else trailing = 0;
        if (ve == vs) { if (*vp) vp++; continue; }
        size_t vlen = (size_t)(ve - vs);
        char *vexpr = (char *)gwram_alloc(vlen + 1);
        if (!vexpr) break;
        memcpy(vexpr, vs, vlen);
        vexpr[vlen] = '\0';
        const char *probe = vexpr;
        while (*probe == ' ') probe++;
        bool is_str = (*probe == '"');
        if (!is_str) {
            /* string-parse probe first (three-tier INSTR pattern) */
            gwdata_clear_error();
            char *sprobe = gweval_eval_string(vexpr);
            if (sprobe && gwdata_get_error() == 0) {
                vals[vcount].is_str = true;
                vals[vcount].str = sprobe;  /* borrowed (eval contract) */
                vals[vcount].num = 0;
                vcount++;
            } else {
                gwdata_clear_error();
                vals[vcount].is_str = false;
                vals[vcount].str = NULL;
                vals[vcount].num = gweval_eval_numeric(vexpr);
                if (gwdata_get_error() != 0) {
                    /* v161 MEDIUM-4: value evaluation failed (e.g.
                     * 1/0 → ERR 11) — abort BEFORE the driver runs;
                     * no pseudo field output */
                    gwram_free(vexpr);
                    gwram_free(vals);
                    *trailing_sep_out = trailing;
                    return ERROR;
                }
                vcount++;
            }
        } else {
            gwdata_clear_error();
            vals[vcount].is_str = true;
            vals[vcount].str = gweval_eval_string(vexpr);
            vals[vcount].num = 0;
            if (!vals[vcount].str) vals[vcount].str = "";
            vcount++;
        }
        gwram_free(vexpr);
        if (gwdata_get_error() != 0) break;
        if (*vp) vp++;
    }
    biprtu_print_using_values(fmt_val, vals, vcount);
    gwram_free(vals);
    *trailing_sep_out = trailing;
    /* v159: errors set inside the driver (TMERR 13 / FCERR 5) must
     * propagate — otherwise the statement reports success */
    if (gwdata_get_error() != 0) return ERROR;
    return SUCCESS;
}

/* ===== v169/v170: compound-statement executor (shared by
 * gwmain line execution and gwsts IF THEN/ELSE dispatch) =====
 * GW-BASIC lines hold multiple colon-separated statements
 * ("A=1: B=2: SWAP A,B"). Execute each segment in order with the
 * same rc semantics as a single statement: rc > 0 = jump target,
 * rc == -2 = END, rc < 0 = error. Colons inside string literals
 * are not separators (GW-BASIC has no escapes; "" is literal
 * content). Returns the first non-zero rc, or 0 when all segments
 * succeeded. */
int gwsts_exec_compound(const char *text) {
    const char *p = text;
    const char *e = NULL;   /* trimmed segment end */
    bool if_block = false;   /* v169: inside IF..THEN/ELSE — colons
                              * after THEN/ELSE belong to the branch,
                              * not to the enclosing statement list */
    int seg_index = 0;
    /* v171 HIGH-1: RESUME NEXT re-enters the ERROR line at the
     * statement AFTER the failing one (ASM OLDTXT resume) */
    int skip_segs = g_state ? g_state->resume_seg_skip : 0;
    if (g_state) g_state->resume_seg_skip = 0;
    while (*p) {
        if (skip_segs > 0) {
            /* advance past the skipped segment */
            const char *q = p;
            bool qq = false;
            while (*q) {
                if (*q == '"') qq = !qq;
                else if (!qq && *q == ':') break;
                q++;
            }
            p = (*q == ':') ? q + 1 : q;
            while (*p && isspace((unsigned char)*p)) p++;
            skip_segs--;
            seg_index++;
            continue;
        }
        /* segment bounds: scan for a top-level colon */
        const char *seg_start = p;
        const char *seg_end = p;
        bool in_quote = false;
        while (*seg_end) {
            if (*seg_end == '"') in_quote = !in_quote;
            else if (*seg_end == ':' && !in_quote) break;
            seg_end++;
        }
        /* trim the segment */
        while (seg_start < seg_end && isspace((unsigned char)*seg_start)) seg_start++;
        /* v170 CRITICAL-1: REM or ' at segment start = comment — the
         * rest of the LINE is comment (GW-BASIC REM extends to EOL,
         * colons inside are not separators). Swallow to EOL without
         * executing anything. */
        if (seg_start < seg_end &&
            ((*seg_start == '\'') ||
             ((seg_end - seg_start >= 3) &&
              (seg_start[0]=='R'||seg_start[0]=='r') &&
              (seg_start[1]=='E'||seg_start[1]=='e') &&
              (seg_start[2]=='M'||seg_start[2]=='m') &&
              (seg_end - seg_start == 3 ||
               !isalnum((unsigned char)seg_start[3]))))) {
            break;   /* nothing else on this line executes */
        }
        /* v171 MEDIUM-4: DATA does NOT extend past a colon (ASM
         * DATA: "TERMINATES ON ':'" — quotes matched). The dead
         * v170 branch (which read `e` before assignment — UB) is
         * removed; DATA ends at the colon like any statement. */
        /* v170: trimmed segment end (recomputed every segment) */
        e = seg_end;
        while (e > seg_start && isspace((unsigned char)e[-1])) e--;
        if (e > seg_start) {
            /* v169: a segment containing a top-level THEN or ELSE
             * swallows the rest of the line (GW-BASIC IF semantics:
             * statements after THEN/ELSE colons run only inside the
             * taken branch, not as sibling statements). */
            if (!if_block) {
                const char *q = seg_start;
                bool q_quote = false;
                while (q < e) {
                    if (*q == '"') q_quote = !q_quote;
                    else if (!q_quote) {
                        if ((q[0]=='T'||q[0]=='t') && (q[1]=='H'||q[1]=='h') &&
                            (q[2]=='E'||q[2]=='e') && (q[3]=='N'||q[3]=='n') &&
                            (q+4 >= e || !isalnum((unsigned char)q[4]))) break;
                        if ((q[0]=='E'||q[0]=='e') && (q[1]=='L'||q[1]=='l') &&
                            (q[2]=='S'||q[2]=='s') && (q[3]=='E'||q[3]=='e') &&
                            (q+4 >= e || !isalnum((unsigned char)q[4]))) break;
                    }
                    q++;
                }
                if (q < e) {
                    if_block = true;
                    seg_end = text + strlen(text);   /* swallow to EOL */
                    e = seg_end;
                    while (e > seg_start && isspace((unsigned char)e[-1])) e--;
                }
            }
            /* v171 HIGH-1: single-line FOR — `FOR I=1 TO 3: body:
             * NEXT I` is ONE construct (ASM FOR saves the text
             * pointer into the line). Swallow the rest of the line
             * so the FOR header sees its body and NEXT. */
            if ((strncasecmp(seg_start, "FOR", 3) == 0 &&
                 (e - seg_start == 3 || seg_start[3] == ' ' || seg_start[3] == '\t'))) {
                seg_end = text + strlen(text);   /* swallow to EOL */
                e = seg_end;
                while (e > seg_start && isspace((unsigned char)e[-1])) e--;
            }
            /* v171 HIGH-1: GOSUB on a compound line — the rest of
             * the line runs AFTER RETURN (ASM resumes at the saved
             * text pointer). Stash it on the GOSUB entry. */
            if ((strncasecmp(seg_start, "GOSUB", 5) == 0 &&
                 (e - seg_start == 5 || seg_start[5] == ' ' || seg_start[5] == '\t'))) {
                const char *tp = seg_end;
                if (*tp == ':') tp++;
                while (*tp && isspace((unsigned char)*tp)) tp++;
                char seg[MAX_LINE_LEN + 1];
                size_t len = (size_t)(e - seg_start);
                if (len > MAX_LINE_LEN) len = MAX_LINE_LEN;
                memcpy(seg, seg_start, len);
                seg[len] = '\0';
                int rc = gwsts_exec_statement(seg);
                /* stash AFTER exec — the GOSUB entry now exists
                 * (rc > 0 = jump to the subroutine body; the tail
                 * still belongs to the entry just pushed) */
                if (*tp) {
                    extern void next86_gosub_set_tail(const char *tail);
                    next86_gosub_set_tail(tp);
                }
                if (rc != 0) return rc;
                return SUCCESS;   /* rest of line deferred to RETURN */
            }
            char seg[MAX_LINE_LEN + 1];
            size_t len = (size_t)(e - seg_start);
            if (len > MAX_LINE_LEN) len = MAX_LINE_LEN;
            memcpy(seg, seg_start, len);
            seg[len] = '\0';
            /* v171 HIGH-1 (RESUME NEXT): track the segment about to
             * execute; if an error fires in it on the ERROR line,
             * pin it as the failing segment. Handler lines (their
             * g_state->current_line differs from error_line) never
             * clobber the pin. */
            if (g_state) g_state->current_seg_index = seg_index;
            int rc = gwsts_exec_statement(seg);
            if (g_state && gwdata_get_error() != 0 &&
                (int)g_state->error_line == (int)g_state->current_line &&
                g_state->error_seg_index < 0) {
                g_state->error_seg_index = g_state->current_seg_index;
            }
            /* v171 HIGH-1: same-line FOR — NEXT returned -3, resume
             * at the body text pointer (statements after FOR) */
            if (rc == -3) {
                extern const char *next86_current_body(void);
                const char *body = next86_current_body();
                if (body) {
                    p = body;   /* restart the body segments */
                    if_block = false;
                    continue;
                }
                rc = 0;   /* no body: fall through as success */
            }
            if (rc != 0) return rc;   /* jump / END / error: stop here */
        }
        if (if_block) break;   /* the IF segment consumed the line */
        seg_index++;
        p = (*seg_end == ':') ? seg_end + 1 : seg_end;
        /* ": REST" — skip leading whitespace of the next segment */
        while (*p && isspace((unsigned char)*p)) p++;
    }
    return SUCCESS;
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
              
                    /* v169: full suffix typing ($ string, % integer,
                     * # double — ASM PTRGET) */
                    {
                        size_t vnl = strlen(var_name);
                        if (vnl > 0) {
                            char lastc = var_name[vnl - 1];
                            if (lastc == '$') type = VT_STRING;
                            else if (lastc == '%') type = VT_INTEGER;
                            else if (lastc == '#') type = VT_DOUBLE;
                        }
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
    /* SWAP statement: SWAP var1, var2 — v158 (ASM: exchange two
     * variables; both must be the same type, else Type mismatch). */
    else if (starts_with(trimmed, "SWAP ") || starts_with(trimmed, "SWAP")) {
        const char *sp = trimmed + 4;
        while (*sp == ' ' || *sp == '\t') sp++;
        /* var1: up to top-level ',' (array indices allowed) */
        const char *c1 = sp;
        bool in_q1 = false;
        int d1 = 0;
        while (*c1 && !(!in_q1 && d1 == 0 && *c1 == ',')) {
            if (*c1 == '"') in_q1 = !in_q1;
            else if (!in_q1 && *c1 == '(') d1++;
            else if (!in_q1 && *c1 == ')') d1--;
            c1++;
        }
        if (*c1 != ',') {
            result = ERROR;  /* SNERR */
        } else {
            const char *v1end = c1;
            while (v1end > sp && isspace((unsigned char)v1end[-1])) v1end--;
            size_t n1 = (size_t)(v1end - sp);
            char *var1 = (char *)gwram_alloc(n1 + 1);
            if (!var1) result = ERROR;
            else {
            memcpy(var1, sp, n1);
            var1[n1] = '\0';
            const char *v2s = c1 + 1;
            while (*v2s == ' ' || *v2s == '\t') v2s++;
            const char *v2e = v2s + strlen(v2s);
            while (v2e > v2s && isspace((unsigned char)v2e[-1])) v2e--;
            size_t n2 = (size_t)(v2e - v2s);
            char *var2 = (char *)gwram_alloc(n2 + 1);
            if (!var2) { gwram_free(var1); result = ERROR; }
            else {
            memcpy(var2, v2s, n2);
            var2[n2] = '\0';
            /* v160 CRITICAL-1 rewrite (reviewer probe: mixed-type SWAP
             * crashed EXIT=116; string-heap ptrs handed to free()).
             * ASM BIMISC SWAP semantics:
             *  1. PTRGET resolves both (arrays + auto-create scalars)
             *  2. types must match → else Type mismatch (13), no write
             *  3. exchange values via descriptors — string swap moves
             *     the value.s descriptor (ptr+len); no copies, no
             *     cross-heap free (string heap is a bump allocator).
             *  4. numeric exchange = raw value union copy (same type
             *     both sides → member layout identical). */
            result = SUCCESS;
            char *bn1 = NULL, *bn2 = NULL;
            int ai1 = -1, ai2 = -1;
            int idx1[8], idx2[8];
            int ni1 = 0, ni2 = 0;
            bool ok1 = parse_var_ref(var1, &bn1, &ai1, idx1, &ni1);
            (void)ok1;
            bool ok2 = parse_var_ref(var2, &bn2, &ai2, idx2, &ni2);
            (void)ok2;
            if (!bn1 || !bn2) {
                gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                result = ERROR;
            } else {
                /* resolve both operands to (variable*, or array ref) */
                struct variable *va = NULL, *vb = NULL;
                bool arr1 = (ni1 > 0), arr2 = (ni2 > 0);
                if (!arr1) {
                    va = gwdata_find_variable(bn1);
                    if (!va) {
                        size_t L1 = strlen(bn1);
                        va = gwdata_alloc_variable(bn1,
                            (L1 && bn1[L1-1]=='$') ? VT_STRING : VT_SINGLE);
                    }
                } else {
                    va = biptrg_ensure_array(bn1);  /* for type query */
                }
                if (!arr2) {
                    vb = gwdata_find_variable(bn2);
                    if (!vb) {
                        size_t L2 = strlen(bn2);
                        vb = gwdata_alloc_variable(bn2,
                            (L2 && bn2[L2-1]=='$') ? VT_STRING : VT_SINGLE);
                    }
                } else {
                    vb = biptrg_ensure_array(bn2);
                }
                var_type_t t1 = va ? va->type : VT_SINGLE;
                var_type_t t2 = vb ? vb->type : VT_SINGLE;
                if (!va || !vb) {
                    gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    result = ERROR;
                } else if (t1 != t2) {
                    /* v160 C1: type check BEFORE any write */
                    gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    result = ERROR;
                } else if (t1 == VT_STRING) {
                    /* v161 HIGH-1: scalar ↔ array element is legal
                     * (ASM PTRGET); all four combinations supported.
                     * v161 HIGH-2: multi-dimensional via *_multi. */
                    char *tmp1 = NULL, *tmp2 = NULL;
                    char *tmp1_copy = NULL, *tmp2_copy = NULL;  /* v168 M1 */
                    if (arr1) {
                        if (ni1 > 1) {
                            extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                            if (!gwdata_find_variable(bn1))
                                biptrg_ensure_array_nd(bn1, ni1);
                        }
                        tmp1 = (ni1 > 1)
                            ? biptrg_get_array_string_multi(bn1, idx1)
                            : biptrg_get_array_string(bn1, idx1[0]);
                        if (!tmp1) tmp1 = "";
                        /* v167 CRITICAL-1: copy immediately — the array
                         * slot pointer is freed by the later
                         * set_array_string (gwram bump reuse), turning
                         * tmp1 into a dangling read (UAF).
                         * v168 MEDIUM-1: track the copy — freed after
                         * both writes; if alloc fails we keep the raw
                         * slot pointer (rare OOM edge, no free). */
                        {
                            size_t t1len = strlen(tmp1);
                            char *t1copy = (char *)gwram_alloc(t1len + 1);
                            if (t1copy) { memcpy(t1copy, tmp1, t1len + 1); tmp1 = t1copy; tmp1_copy = t1copy; }
                        }
                    } else {
                        tmp1 = va->value.s.ptr ? va->value.s.ptr : "";
                    }
                    if (arr2) {
                        if (ni2 > 1) {
                            extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                            if (!gwdata_find_variable(bn2))
                                biptrg_ensure_array_nd(bn2, ni2);
                        }
                        tmp2 = (ni2 > 1)
                            ? biptrg_get_array_string_multi(bn2, idx2)
                            : biptrg_get_array_string(bn2, idx2[0]);
                        if (!tmp2) tmp2 = "";
                        /* v167 CRITICAL-1: same defensive copy for tmp2 */
                        {
                            size_t t2len = strlen(tmp2);
                            char *t2copy = (char *)gwram_alloc(t2len + 1);
                            if (t2copy) { memcpy(t2copy, tmp2, t2len + 1); tmp2 = t2copy; tmp2_copy = t2copy; }
                        }
                    } else {
                        tmp2 = vb->value.s.ptr ? vb->value.s.ptr : "";
                    }
                    /* v162 MEDIUM-5: a get that set ERRBS (subscript
                     * out of range) must abort BEFORE any write —
                     * no partial swap data loss */
                    if (gwdata_get_error() != 0) {
                        result = ERROR;
                    } else
                    /* write value2 → slot1, value1 → slot2 */
                    if (arr1) {
                        if (ni1 > 1) result = biptrg_set_array_string_multi(bn1, idx1, tmp2);
                        else result = biptrg_set_array_string(bn1, idx1[0], tmp2);
                    } else {
                        va->value.s.len = (int)strlen(tmp2);
                        va->value.s.ptr = gwdata_alloc_string(va->value.s.len + 1);
                        if (va->value.s.ptr) {
                            memcpy(va->value.s.ptr, tmp2, va->value.s.len + 1);
                        }
                    }
                    if (arr2) {
                        if (ni2 > 1) result = biptrg_set_array_string_multi(bn2, idx2, tmp1);
                        else result = biptrg_set_array_string(bn2, idx2[0], tmp1);
                    } else {
                        vb->value.s.len = (int)strlen(tmp1);
                        vb->value.s.ptr = gwdata_alloc_string(vb->value.s.len + 1);
                        if (vb->value.s.ptr) {
                            memcpy(vb->value.s.ptr, tmp1, vb->value.s.len + 1);
                        }
                    }
                    /* v168 MEDIUM-1: release the defensive copies
                     * (the array slots now own their own copies) */
                    if (tmp1_copy) gwram_free(tmp1_copy);
                    if (tmp2_copy) gwram_free(tmp2_copy);
                    if (result != ERROR) result = SUCCESS;
                } else {
                    /* numeric: v169 CRITICAL-1 — fully typed swap (value.i/f/d by the
 * variable's type; float-only round-trips corrupted VT_DOUBLE and
 * reinterpreted VT_INTEGER bits) */
                    biptrg_typed_val v1t, v2t;
                    memset(&v1t, 0, sizeof(v1t));
                    memset(&v2t, 0, sizeof(v2t));
                    if (arr1) {
                        if (ni1 > 1) {
                            extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                            if (!gwdata_find_variable(bn1))
                                biptrg_ensure_array_nd(bn1, ni1);
                        }
                        if (biptrg_get_array_element_typed(bn1, idx1, ni1, &v1t) != SUCCESS) {
                            result = ERROR;
                        }
                    } else {
                        v1t.type = va->type;
                        if (va->type == VT_INTEGER) { v1t.i = va->value.i; v1t.d = (double)va->value.i; }
                        else if (va->type == VT_DOUBLE) { v1t.d = va->value.d; }
                        else { v1t.d = va->value.f; }
                    }
                    if (result != ERROR && arr2) {
                        if (ni2 > 1) {
                            extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                            if (!gwdata_find_variable(bn2))
                                biptrg_ensure_array_nd(bn2, ni2);
                        }
                        if (biptrg_get_array_element_typed(bn2, idx2, ni2, &v2t) != SUCCESS) {
                            result = ERROR;
                        }
                    } else if (result != ERROR) {
                        v2t.type = vb->type;
                        if (vb->type == VT_INTEGER) { v2t.i = vb->value.i; v2t.d = (double)vb->value.i; }
                        else if (vb->type == VT_DOUBLE) { v2t.d = vb->value.d; }
                        else { v2t.d = vb->value.f; }
                    }
                    /* v162 MEDIUM-5: abort before write on get error */
                    if (result != ERROR && gwdata_get_error() != 0) {
                        result = ERROR;
                    } else if (result != ERROR) {
                    /* write value2 → slot1, value1 → slot2 (typed) */
                    if (arr1) {
                        result = biptrg_set_array_element_typed(bn1, idx1, ni1, &v2t);
                    } else {
                        if (va->type == VT_INTEGER) va->value.i = v2t.i;
                        else if (va->type == VT_DOUBLE) va->value.d = v2t.d;
                        else va->value.f = (float)v2t.d;
                    }
                    if (arr2) {
                        result = biptrg_set_array_element_typed(bn2, idx2, ni2, &v1t);
                    } else {
                        if (vb->type == VT_INTEGER) vb->value.i = v1t.i;
                        else if (vb->type == VT_DOUBLE) vb->value.d = v1t.d;
                        else vb->value.f = (float)v1t.d;
                    }
                    }
                    if (result != ERROR) result = SUCCESS;
                }
            }
            if (bn1) gwram_free(bn1);
            if (bn2) gwram_free(bn2);
            gwram_free(var2);
            }
            gwram_free(var1);
            }
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
        /* v156 obs-5: graphics statement in text mode → error
         * (ASM: SCREEN 0 has no graphics — Illegal function call) */
        if (gengrp_get_screen_mode() != 0) {
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
        }  /* end screen-mode gate */
        if (gengrp_get_screen_mode() == 0) {
            /* v156 MEDIUM-1: set error code/line so ON ERROR +
             * RESUME NEXT works (ASM: Illegal function call = 5) */
            gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
            result = ERROR;
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
        /* v156 obs-5: graphics in text mode → error (wrap body in gate) */
        if (gengrp_get_screen_mode() != 0) {
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
        }  /* end screen-mode gate */
        if (gengrp_get_screen_mode() == 0) {
            /* v156 MEDIUM-1: set error code/line so ON ERROR +
             * RESUME NEXT works (ASM: Illegal function call = 5) */
            gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
            result = ERROR;
        }
    }
    /* CIRCLE statement: CIRCLE (x,y),radius[,color[,start[,end[,aspect]]]]
     * (ASM ADVGRP CIRCLE L460: SCAN1 coords, angles in radians, 0=east CCW;
     * negative angle draws radius line to center; aspect<1 scales Y) */
    else if (starts_with(trimmed, "CIRCLE") &&
             (trimmed[6] == ' ' || trimmed[6] == '\t' || trimmed[6] == '(')) {
        /* v156 obs-5: graphics in text mode → error (wrap body in gate) */
        if (gengrp_get_screen_mode() != 0) {
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
        }  /* end screen-mode gate */
        if (gengrp_get_screen_mode() == 0) {
            /* v156 MEDIUM-1: set error code/line so ON ERROR +
             * RESUME NEXT works (ASM: Illegal function call = 5) */
            gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
            result = ERROR;
        }
    }
    /* PAINT statement: PAINT (x,y)[,fill[,border]] (ASM ADVGRP PAINT L121) */
    else if (starts_with(trimmed, "PAINT") &&
             (trimmed[5] == ' ' || trimmed[5] == '\t' || trimmed[5] == '(')) {
        /* v156 obs-5: graphics in text mode → error (wrap body in gate) */
        if (gengrp_get_screen_mode() != 0) {
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
        }  /* end screen-mode gate */
        if (gengrp_get_screen_mode() == 0) {
            /* v156 MEDIUM-1: set error code/line so ON ERROR +
             * RESUME NEXT works (ASM: Illegal function call = 5) */
            gwdata_set_error(5, (uint16_t)g_state->current_line, 0);
            result = ERROR;
        }
    }
    /* FOR statement: FOR var = start TO limit [STEP step] */
    else if (starts_with(trimmed, "FOR ")) {
        extern int next86_for(const char **text);
        const char *text_ptr = trimmed + 4;  /* Skip "FOR " */
        result = next86_for(&text_ptr);
        /* v171 HIGH-1: single-line FOR — the header consumed its
         * body text; run the body now (its NEXT drives the loop
         * via the -3 resume inside the compound executor) */
        if (result == 0) {
            extern const char *next86_current_body(void);
            if (next86_current_body()) {
                result = gwsts_exec_compound(next86_current_body());
            }
        }
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
        extern const char *next86_gosub_take_tail(void);
        const char *text_ptr = trimmed + 6;  /* Skip "RETURN" */
        const char *tail = next86_gosub_take_tail();   /* BEFORE pop */
        result = next86_return(&text_ptr);
        /* v171 HIGH-1: statements after GOSUB on the same line run
         * after RETURN (ASM resumes at the saved text pointer) */
        if (result > 0 || result == 0) {
            if (tail) {
                int trc = gwsts_exec_compound(tail);
                if (trc != 0) {
                    result = trc;   /* tail jumped/ended/errored */
                }
            }
        }
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
            gwsts_note_newline();
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
                                        if (gwdata_get_error() != 0) {
                                            /* v160 HIGH-2: eval raised
                                             * Overflow (1E+39) — abort
                                             * PRINT, propagate error */
                                            gwram_free(trimmed);
                                            return ERROR;
                                        }
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
                /* v158: PRINT USING <fmt>; <list> — whole-statement
                 * handler (the value list spans multiple expression
                 * fragments, so it cannot live in the per-fragment
                 * loop). */
                const char *up = trimmed + 5;
                while (*up == ' ' || *up == '\t') up++;
                /* v159 HIGH-6: keyword is case-insensitive */
                if ((up[0]=='U'||up[0]=='u') && (up[1]=='S'||up[1]=='s') &&
                    (up[2]=='I'||up[2]=='i') && (up[3]=='N'||up[3]=='n') &&
                    (up[4]=='G'||up[4]=='g') &&
                    (up[5] == ' ' || up[5] == '\t' || up[5] == '\0')) {
                    int trail = 0;
                    result = gwsts_print_using(up + 5, &trail);
                    /* v159 MEDIUM-5 / v160 MEDIUM-2: trailing ';'
                     * suppresses newline; trailing ',' advances to the
                     * next 14-column print zone (spaces, not '\t');
                     * on error no newline is emitted (empty format
                     * must not print a stray blank line) */
                    if (gwdata_get_error() == 0) {
                        if (trail == 0) { printf("\n"); gwsts_note_newline(); }
                        else if (trail == 2) {
                            /* current column = chars printed this line;
                             * approximate with the chunk-writer state is
                             * unavailable here — use the using-out
                             * flush count tracked by biprtu */
                            extern int biprtu_last_output_columns(void);
                            /* v161 MEDIUM-3: real console column =
                             * current stdout position minus the last
                             * line-start position (line starts are
                             * tracked at every PRINT newline). */
                            extern long gwsts_last_line_start(void);
                            int col = -1;
                            if (!isatty(fileno(stdout))) {
                                long pos = ftell(stdout);
                                if (pos >= 0)
                                    col = (int)(pos - gwsts_last_line_start());
                            }
                            if (col < 0) col = biprtu_last_output_columns();
                            int zone = 14;
                            int next = ((col / zone) + 1) * zone;
                            for (int k = col; k < next; k++) putchar(' ');
                        }
                    }
                    gwram_free(trimmed);
                    return result;
                }
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
                            } else if (gwdata_get_error() != 0) {
                                /* v137/v160: error-aware PRINT — keep
                                 * the baseline trailing newline (PRINT
                                 * is line-oriented; test_gwram OM
                                 * handler layout depends on it) */
                                printf("\n");
                                gwram_free(trimmed);
                                return ERROR;
                            } else {
                                double val = gweval_eval_numeric(trim_start);
                                if (gwdata_get_error() != 0) {
                                    /* v160 HIGH-2: eval raised Overflow
                                     * etc. — abort, propagate, no output */
                                    gwram_free(trimmed);
                                    return ERROR;
                                }
                                /* GW-BASIC: positive numbers have leading
                                 * space; single prints 7 digits, double
                                 * (VALTYP 8) prints 16 — v169 HIGH-1 */
                                extern int gweval_last_valtyp(void);
                                if (gweval_last_valtyp() == 8) {
                                    printf("%s%.16g", (val >= 0) ? " " : "", val);
                                } else {
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
                        gwsts_note_newline();
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
                bool condition_hard_error = false;  /* v167 HIGH-1 */
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
                                if ((!ls || !rs) && gwdata_get_error() != 0) {
                                    /* v166 HIGH-2: eval failure with an
                                     * active error (e.g. OM from a literal
                                     * when the heap is full) must propagate —
                                     * silently treating NULL as false changed
                                     * program logic. */
                                    cv = -3;
                                }
                                else if (ls && rs && gwdata_get_error() == 0) {
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
                        if (cv == -3) {
                            /* v166 HIGH-2: hard error (OM) from string eval —
                             * keep the error, fail the statement */
                            use_atoms = false;
                            break;
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
                if (!use_atoms && gwdata_get_error() == 0) {
                    /* fallback: whole-condition numeric eval.
                     * v167 HIGH-1: when use_atoms bailed on a hard
                     * error (cv=-3, e.g. OM while evaluating a literal),
                     * the error is still active and must NOT be cleared
                     * and re-evaluated — that turned OM into a silent
                     * "condition false". */
                    gwdata_clear_error();
                    float cond_val = gweval_eval_numeric(cond_expr);
                    then_taken = (cond_val != 0.0f);
                } else if (!use_atoms) {
                    /* v167 HIGH-1: hard error (OM) — statement fails,
                     * error stays set for ON ERROR / run loop */
                    condition_hard_error = true;
                    result = ERROR;
                }
                #undef GWSIFS_MAX_ATOMS

                /* 4. THEN / ELSE dispatch (v138 MEDIUM-2) */
                /* v167 HIGH-1: on a hard condition error (OM) the
                 * statement has already failed (result=ERROR) — the
                 * dispatch below must NOT overwrite it with SUCCESS */
                if (!condition_hard_error) {
                /* helper: pure-numeric THEN/ELSE target = line number jump
                 * (GW-BASIC: IF..THEN <line> is a GOTO) */
                /* v171 HIGH-2: only the FIRST token decides (ASM
                 * DOCOND CHRGTR: THEN <lincon> = GOTO, the rest of
                 * the line is discarded by the jump). */
                #define GWSIFS_IS_LINENUM(s_) (s_ != NULL && *s_ != '\0' && \
                    strspn(s_, "0123456789") > 0 && \
                    !isdigit((unsigned char)s_[strspn(s_, "0123456789")]))
                if (then_taken) {
                    if (GWSIFS_IS_LINENUM(then_stmt)) {
                        result = atoi(then_stmt);   /* jump target (>0) */
                    } else {
                        /* v170 HIGH-1: THEN branch may hold multiple
                         * colon-separated statements (GW-BASIC: they
                         * belong to the branch) */
                        result = gwsts_exec_compound(then_stmt);
                    }
                } else if (else_stmt) {
                    if (GWSIFS_IS_LINENUM(else_stmt)) {
                        result = atoi(else_stmt);   /* jump target (>0) */
                    } else {
                        result = gwsts_exec_compound(else_stmt);
                    }
                } else {
                    result = SUCCESS;
                }
                }  /* v167: end of "no hard error" guard */
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
                /* v171 HIGH-1: the failing statement may have been a
                 * mid-line segment — resume at the NEXT segment of
                 * the SAME line (ASM OLDTXT resume) */
                if (g_state && g_state->error_seg_index > 0 && curr) {
                    gwdata_clear_error();
                    gwdata_exit_error_handler();
                    g_state->resume_seg_skip = g_state->error_seg_index + 1;
                    g_state->error_seg_index = -1;   /* consumed */
                    result = err_line;   /* re-enter the same line */
                }
                else if (curr && curr->next) {
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
    /* OPEN statement — v152: extracted to gwsts_exec_open (ASM GIO86
     * dual forms; expression filespec; RANDOM-only reclen) */
    else if (starts_with(trimmed, "OPEN ")) {
        result = gwsts_exec_open(trimmed);
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
                    assign_value_raw(var_ref, input_buf);
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
                        assign_value_raw(var_ref, input_buf);
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
                    assign_value_raw(var_ref, input_buf);
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
/* v162 HIGH-1: raw-text assignment mode for INPUT/READ/LINE INPUT,
 * whose value argument is literal text, not an expression. Strict
 * mode (default) treats a non-string RHS as Type mismatch (13). */
static bool g_assign_allow_raw = false;

int assign_value_raw(const char *var_ref, const char *expr) {
    g_assign_allow_raw = true;
    int rc = assign_value(var_ref, expr);
    g_assign_allow_raw = false;
    return rc;
}

/* Case-insensitive string comparison (same as gweval) */
static int strcasecmp_ascii(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int assign_value(const char *var_ref, const char *expr) {
    if (!var_ref || !expr) return ERROR;
    
    
    char *base_name = NULL;
    int array_index = -1;
    int indices[8];
    int num_indices = 0;
    parse_var_ref(var_ref, &base_name, &array_index, indices, &num_indices);
    
    /* v169 MEDIUM-3: ERL is the read-only ERRLIN alias (ASM
     * GWEVAL L673); assigning it would silently create a dead
     * ordinary variable. */
    if (base_name && strcasecmp_ascii(base_name, "ERL") == 0) {
        gwdata_set_error(5, g_state ? (uint16_t)g_state->current_line : 0, 0);
        gwram_free(base_name);
        return ERROR;
    }
    
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
                /* v161 HIGH-2: multi-dimensional string arrays.
                 * v163 HIGH-1: raw mode stores the literal directly
                 * (never evaluates DATA/INPUT text as an expression). */
                gwdata_clear_error();  /* stale-error isolation */
                char *store_ptr = NULL;
                char *heap_copy = NULL;
                if (g_assign_allow_raw) {
                    size_t expr_len = strlen(expr);
                    heap_copy = gwdata_alloc_string(expr_len + 1);
                    if (heap_copy) memcpy(heap_copy, expr, expr_len + 1);
                    store_ptr = heap_copy;
                } else {
                    store_ptr = gweval_eval_string(expr);
                    if (!store_ptr && gwdata_get_error() == 0) {
                        /* v162 HIGH-1: non-string expression → 13 */
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                    }
                }
                if (store_ptr) {
                    if (num_indices > 1) {
                        extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                        biptrg_ensure_array_nd(base_name, num_indices);  /* v164: rank check too */
                        if (gwdata_get_error() != 0) {
                            result = ERROR;   /* rank mismatch — no write */
                        } else
                        result = biptrg_set_array_string_multi(base_name, indices, store_ptr);
                    }
                    else
                        result = biptrg_set_array_string(base_name, indices[0], store_ptr);
                } else {
                    /* eval error or TMERR — keep the old element value */
                    result = ERROR;
                }
            } else {
                /* Numeric array: use multi or single interface */
                gwdata_clear_error();  /* stale-error isolation (tests) */
                double val = gweval_eval_numeric(expr);   /* v170: full precision */
                if (gwdata_get_error() != 0) {
                    /* v162 MEDIUM-3: evaluation raised Overflow etc. —
                     * do NOT write 0 into the element */
                    result = ERROR;
                } else if (num_indices == 1) {
                    /* v170 HIGH-5: typed set — VT_DOUBLE arrays store
                     * the full-precision value (was float-truncated) */
                    struct variable *av = gwdata_find_variable(base_name);
                    if (av && av->type == VT_DOUBLE) {
                        biptrg_typed_val tv;
                        tv.type = VT_DOUBLE; tv.i = 0; tv.d = (double)val;
                        int tidx[1] = { indices[0] };
                        result = biptrg_set_array_element_typed(base_name, tidx, 1, &tv);
                    } else {
                        result = biptrg_set_array_element(base_name, indices[0], val);
                    }
                } else {
                    /* v162 HIGH-2: auto-DIM per dimension (GW-BASIC
                     * uses 10 per dimension for undeclared arrays) */
                    extern struct variable *biptrg_ensure_array_nd(const char *name, int ndims);
                    if (!gwdata_find_variable(base_name))
                        biptrg_ensure_array_nd(base_name, num_indices);
                    else
                        biptrg_ensure_array_nd(base_name, num_indices);  /* v163: rank check → ERRBS */
                    if (gwdata_get_error() != 0) {
                        result = ERROR;   /* rank mismatch — no write */
                    } else {
                        /* v171 HIGH-3: multi-dim typed set — VT_DOUBLE
                         * arrays store full precision (was float) */
                        struct variable *av = gwdata_find_variable(base_name);
                        if (av && (av->type == VT_DOUBLE || av->type == VT_INTEGER)) {
                            biptrg_typed_val tv;
                            tv.type = av->type; tv.i = 0;
                            tv.d = (av->type == VT_INTEGER) ? (double)(int32_t)val : val;
                            result = biptrg_set_array_element_typed(base_name, indices,
                                                                    num_indices, &tv);
                        } else {
                            result = biptrg_set_array_element_multi(base_name, indices,
                                                                    (float)val);
                        }
                    }
                }
            }
        } else {
            /* Simple variable assignment */
            /* Check if string variable (name ends with $) */
            size_t name_len = strlen(base_name);
            bool is_string_var = (name_len > 0 && base_name[name_len - 1] == '$');
            
            if (is_string_var) {
                /* v163 CRITICAL-1: decide the new value COMPLETELY
                 * before gwdata_alloc_variable — alloc on an existing
                 * string variable CLEARS value.s (gwdata.c L82-84), so
                 * any failure after alloc would lose the old value
                 * (v162 regression: A$="old" then A$=5 → LEN 0).
                 * v163 HIGH-1: raw mode (INPUT/READ/LINE INPUT) never
                 * evaluates the text as an expression — READ DATA A$
                 * must store the literal "A$", not the variable's
                 * value. */
                gwdata_clear_error();  /* stale-error isolation */
                result = SUCCESS;      /* reset: failures set ERROR below */
                char *new_ptr = NULL;
                int new_len = 0;
                if (g_assign_allow_raw) {
                    size_t expr_len = strlen(expr);
                    new_ptr = gwdata_alloc_string(expr_len + 1);
                    if (new_ptr) {
                        memcpy(new_ptr, expr, expr_len + 1);
                        new_len = (int)expr_len;
                    }
                } else {
                    char *str_val = gweval_eval_string(expr);
                    if (str_val) {
                        new_ptr = str_val;   /* borrowed (eval contract) */
                        new_len = (int)strlen(str_val);
                    } else if (gwdata_get_error() != 0) {
                        /* String function raised FCERR/SNERR (e.g.
                         * CHR$(256)) — old value must survive */
                        result = ERROR;
                    } else {
                        /* v162 HIGH-1: non-string expression → Type
                         * mismatch (13); old value must survive */
                        gwdata_set_error(13, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        result = ERROR;
                    }
                }
                if (result != ERROR && new_ptr) {
                    struct variable *v = gwdata_alloc_variable(base_name, VT_STRING);
                    if (v) {
                        v->value.s.ptr = new_ptr;
                        v->value.s.len = new_len;
                        result = SUCCESS;
                    }
                }
            } else {
                /* Numeric variable (integer or float) */
                gwdata_clear_error();  /* stale-error isolation (tests) */
                double val = gweval_eval_numeric(expr);   /* v169: full precision */
                if (gwdata_get_error() != 0) {
                    /* v161 HIGH-3/LOW-3: evaluation raised Overflow
                     * etc. — do NOT assign; propagate (A stays old) */
                    if (base_name) gwram_free(base_name);
                    return ERROR;
                }
                /* v169 HIGH: type suffix semantics (ASM PTRGET).
                 * % → 16-bit integer: round-to-nearest, overflow →
                 * FCERR 6 (ASM FWRTN); # → double; default single. */
                size_t bn_len = strlen(base_name);
                char last_ch = bn_len ? base_name[bn_len - 1] : '\0';
                if (last_ch == '%') {
                    /* round-half-away-from-zero (GW-BASIC uses the
                     * MBF rounding of the accumulator) */
                    float rounded = (val >= 0.0f) ? (float)floor(val + 0.5f)
                                                  : (float)ceil(val - 0.5f);
                    if (rounded < -32768.0f || rounded > 32767.0f) {
                        gwdata_set_error(6, g_state ? (uint16_t)g_state->current_line : 0, 0);
                        if (base_name) gwram_free(base_name);
                        return ERROR;
                    }
                    struct variable *v = gwdata_alloc_variable(base_name, VT_INTEGER);
                    if (v) {
                        v->value.i = (int32_t)rounded;
                        result = SUCCESS;
                    }
                } else if (last_ch == '#') {
                    struct variable *v = gwdata_alloc_variable(base_name, VT_DOUBLE);
                    if (v) {
                        v->value.d = val;   /* v169: full-precision double */
                        result = SUCCESS;
                    }
                } else {
                    struct variable *v = gwdata_alloc_variable(base_name, VT_SINGLE);
                    if (v) {
                        v->value.f = val;
                        result = SUCCESS;
                    }
                }
            }
        }
    }
    
    if (base_name) gwram_free(base_name);
    return result;
}