/* NEXT86 - NEXT Loop Handling */
/* Based on NEXT86.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* External references */
extern void gweval_init(void);
extern double gweval_eval_numeric(const char *expr);
extern void gweval_chrgt2(const char **text);
extern void gweval_synchr(const char **text, char expected);
extern void gweval_scan_var(const char **text, int *var_ptr);
extern int gweval_frqint(void);
extern struct variable *gwdata_alloc_variable(const char *name, var_type_t type);

/* FOR stack management */
#define MAX_FOR_DEPTH 16
struct for_entry g_for_stack[MAX_FOR_DEPTH];
int g_for_sp = 0;  /* Stack pointer */

/* State variables */
BYTE g_nxtflg = 0;      /* NXTFLG - FOR using NEXT code */
WORD g_fvalsv = 0;      /* FVALSV - saved initial value */
WORD g_nxttxt = 0;      /* NXTTXT - text pointer */
WORD g_temp = 0;        /* TEMP - temp storage */
WORD g_curlin = 0;      /* CURLIN - current line number */
WORD g_savstk = 0;      /* SAVSTK - saved stack pointer */
BYTE g_valtyp = 0;      /* VALTYP - variable type */

/* Module initialization */
void next86_init(void) {
    g_for_sp = 0;
    g_nxtflg = 0;
    g_fvalsv = 0;
    g_nxttxt = 0;
    g_temp = 0;
    g_curlin = 0;
    g_savstk = 0;
    g_valtyp = 0;
    
    /* Initialize FOR stack */
    for (int i = 0; i < MAX_FOR_DEPTH; i++) {
        g_for_stack[i].token = 0;
        g_for_stack[i].var_ptr = 0;
        g_for_stack[i].next_text_ptr = 0;
        g_for_stack[i].step_sign = 0;
        g_for_stack[i].var_type = 0;
        g_for_stack[i].step = 0.0f;
        g_for_stack[i].limit = 0.0f;
        g_for_stack[i].for_line = 0;
        g_for_stack[i].for_next_line = 0;
    }
}

/* Forward declarations */
static int next86_int_next(struct for_entry *entry);
static int next86_float_next(struct for_entry *entry);

/* Push a FOR entry onto the stack */
int next86_push_for(BYTE token, intptr_t var_ptr, WORD next_text_ptr, 
                    BYTE step_sign, BYTE var_type, float step, float limit,
                    WORD for_line, WORD for_next_line, const char *var_name) {
    if (g_for_sp >= MAX_FOR_DEPTH) {
        return -1;  /* Stack overflow */
    }
    
    struct for_entry *entry = &g_for_stack[g_for_sp++];
    entry->token = token;
    entry->var_ptr = var_ptr;
    entry->next_text_ptr = next_text_ptr;
    entry->step_sign = step_sign;
    entry->var_type = var_type;
    entry->step = step;
    entry->limit = limit;
    entry->for_line = for_line;
    entry->for_next_line = for_next_line;
    if (var_name) {
        strncpy(entry->var_name, var_name, sizeof(entry->var_name) - 1);
        entry->var_name[sizeof(entry->var_name) - 1] = '\0';
    }
    
    return 0;  /* Success */
}

/* Pop a FOR entry from the stack */
int next86_pop_for(struct for_entry *entry) {
    if (g_for_sp == 0) {
        return -1;  /* Stack underflow */
    }
    
    *entry = g_for_stack[--g_for_sp];
    return 0;  /* Success */
}

/* Find matching FOR entry for a NEXT statement */
struct for_entry* next86_find_for(const char *var_name) {
    /* Search from top of stack down - match by variable name */
    for (int i = g_for_sp - 1; i >= 0; i--) {
        struct for_entry *entry = &g_for_stack[i];
        if (strcasecmp(entry->var_name, var_name) == 0) {
            return entry;
        }
    }
    return NULL;  /* Not found */
}

/* Process NEXT statement */
int next86_next(const char **text) {
    g_nxttxt = (WORD)(uintptr_t)*text;
    
    /* Parse variable name (optional) — v169 HIGH-2: suffixes too */
    char var_name[41] = {0};
    int var_name_len = 0;
    while (**text && (isalpha((unsigned char)**text) || **text == '%' ||
                      **text == '!' || **text == '#' || **text == '$') && var_name_len < 40) {
        var_name[var_name_len++] = **text;
        (*text)++;
    }
    var_name[var_name_len] = '\0';
    
    /* Find matching FOR entry - if var_name is empty, match top of stack */
    struct for_entry *entry = NULL;
    if (var_name[0] == '\0') {
        /* NEXT without variable name - match top of stack (most recent FOR) */
        if (g_for_sp > 0) {
            entry = &g_for_stack[g_for_sp - 1];
        }
    } else {
        entry = next86_find_for(var_name);
    }
    
    if (!entry) {
        return -1;  /* NEXT without FOR error */
    }
    
    /* Set up stack by chopping to FOR entry */
    g_savstk = g_for_sp;  /* Simplified */
    
    /* Process based on variable type — v169: VT_INTEGER (2) →
     * integer NEXT; VT_SINGLE/VT_DOUBLE → float/double NEXT
     * (the old `var_type & 0x80` test never matched VT_INTEGER=2,
     * so integer loops incremented in float). */
    if (entry->var_type == VT_INTEGER) {
        return next86_int_next(entry);
    } else {
        return next86_float_next(entry);
    }
}

/* Integer NEXT processing */
int next86_int_next(struct for_entry *entry) {
    struct variable *v = (struct variable*)entry->var_ptr;
    
    /* Add step to variable — v169: signed step (negative STEP on
     * integer loops) */
    v->value.i += (int32_t)entry->step;
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (v->value.i <= (WORD)entry->limit) ? 0 : 1;
    } else {
        cmp = (v->value.i >= (WORD)entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues - return FOR next line number to continue from */
        return entry->for_next_line;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;
    }
}

/* Floating point NEXT processing — v169 HIGH-2: double-aware
 * (VT_DOUBLE loop variables increment/compare in double) */
int next86_float_next(struct for_entry *entry) {
    struct variable *v = (struct variable*)entry->var_ptr;
    
    if (entry->var_type == VT_DOUBLE) {
        v->value.d += entry->step;
        int cmp2 = entry->step_sign ? (v->value.d <= entry->limit ? 0 : 1)
                                    : (v->value.d >= entry->limit ? 0 : 1);
        if (cmp2 == 0) return entry->for_next_line;
        g_for_sp--;
        return 0;
    }
    
    /* Add step to variable */
    v->value.f += entry->step;
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (v->value.f <= entry->limit) ? 0 : 1;
    } else {
        cmp = (v->value.f >= entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues - return FOR next line number to continue from */
        return entry->for_next_line;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;
    }
}

/* v145: scan forward from from_line for the NEXT that closes the FOR just
 * pushed at from_line (ASM NXTSCN). FOR +1 / NEXT -1 by line keyword;
 * returns the line number AFTER the matching NEXT, or 0 if not found. */
static int next86_scan_matching_next(int from_line) {
    int depth = 1;   /* we are looking for the NEXT of this FOR */
    struct program_line *curr = gwlist_get_first_line();
    while (curr && curr->number <= from_line) {
        curr = curr->next;   /* skip the FOR line itself and everything before */
    }
    while (curr) {
        const char *t = curr->text;
        while (t && *t == ' ') t++;
        if (t) {
            if ((t[0] == 'F' || t[0] == 'f') &&
                (t[1] == 'O' || t[1] == 'o') &&
                (t[2] == 'R' || t[2] == 'r') &&
                (t[3] == ' ' || t[3] == '\t')) {
                depth++;   /* nested FOR */
            } else if ((t[0] == 'N' || t[0] == 'n') &&
                       (t[1] == 'E' || t[1] == 'e') &&
                       (t[2] == 'X' || t[2] == 'x') &&
                       (t[3] == 'T' || t[3] == 't')) {
                depth--;
                if (depth == 0) {
                    /* matching NEXT found — resume after its line */
                    struct program_line *after = curr->next;
                    return after ? after->number : 0;
                }
            }
        }
        curr = curr->next;
    }
    return 0;
}

/* FOR statement handler */
int next86_for(const char **text) {
    /* Parse FOR statement: FOR var = start TO limit [STEP step] */
    
    /* Skip leading whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse variable name — v169 HIGH-2: type suffixes (%#!$) are
     * part of the name (ASM PTRGET) */
    char var_name[41] = {0};
    int var_name_len = 0;
    while (**text && (isalnum((unsigned char)**text) || **text == '_' ||
                      **text == '%' || **text == '!' || **text == '#' ||
                      **text == '$') && var_name_len < 40) {
        var_name[var_name_len++] = **text;
        (*text)++;
    }
    var_name[var_name_len] = '\0';
    
    /* Skip whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Expect '=' */
    if (**text != '=') {
        return ERROR;
    }
    (*text)++;
    
    /* Skip whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse start value */
    float start = gweval_eval_numeric(*text);
    
    /* Skip to after start value */
    while (**text && (isdigit((unsigned char)**text) || **text == '.' || **text == '-' || **text == '+')) (*text)++;
    
    /* Skip whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Expect 'TO' */
    if ((**text == 'T' || **text == 't') && 
        (*(*text + 1) == 'O' || *(*text + 1) == 'o')) {
        (*text) += 2;
    } else {
        return ERROR;
    }
    
    /* Skip whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse limit value */
    float limit = gweval_eval_numeric(*text);
    
    /* Skip to after limit value */
    while (**text && (isdigit((unsigned char)**text) || **text == '.' || **text == '-' || **text == '+')) (*text)++;
    
    /* Skip whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse optional STEP */
    float step = 1.0f;
    if ((**text == 'S' || **text == 's') &&
        (*(*text + 1) == 'T' || *(*text + 1) == 't') &&
        (*(*text + 2) == 'E' || *(*text + 2) == 'e') &&
        (*(*text + 3) == 'P' || *(*text + 3) == 'p')) {
        (*text) += 4;  /* Skip 'STEP' */
        
        /* Skip whitespace */
        while (**text && isspace((unsigned char)**text)) (*text)++;
        
        /* Parse step value */
        step = gweval_eval_numeric(*text);
    }
    
    /* Determine variable type (v169: ftype below) */
    /* ASM NT0STP (GWMAIN L1600): STEP 0 → sign forced to 2 — the NEXT
     * termination test (sign(current-final) == sign(step)) can never be
     * met, giving the ANSI-required infinite loop. */
    BYTE step_sign;
    if (step == 0.0f) {
        step_sign = 2;
    } else {
        step_sign = (step > 0) ? 1 : 0;
    }
    
    /* Allocate variable in GW-BASIC variable table.
     * v169 HIGH-2: typed FOR — % integer (with FWRTN overflow
     * semantics on the start value), # double, default single. */
    var_type_t ftype = VT_SINGLE;
    {
        size_t vnl = strlen(var_name);
        char lastc = vnl ? var_name[vnl - 1] : '\0';
        if (lastc == '%') ftype = VT_INTEGER;
        else if (lastc == '#') ftype = VT_DOUBLE;
    }
    struct variable *v = gwdata_alloc_variable(var_name, ftype);
    if (!v) {
        return ERROR;
    }
    if (ftype == VT_INTEGER) {
        float rounded = (start >= 0.0f) ? (float)floor(start + 0.5f)
                                        : (float)ceil(start - 0.5f);
        if (rounded < -32768.0f || rounded > 32767.0f) {
            gwdata_set_error(6, g_curlin, 0);
            return ERROR;
        }
        v->value.i = (int32_t)rounded;
    } else if (ftype == VT_DOUBLE) {
        v->value.d = start;
    } else {
        v->value.f = start;
    }
    intptr_t var_ptr = (intptr_t)v;
    
    /* Save line number AFTER the FOR statement for loop continuation */
    /* We need to find the next line in the program after this FOR line */
    struct program_line *for_line_ptr = gwlist_get_first_line();
    while (for_line_ptr && for_line_ptr->number != g_curlin) {
        for_line_ptr = for_line_ptr->next;
    }
    WORD for_next_line = 0;
    if (for_line_ptr && for_line_ptr->next) {
        for_next_line = for_line_ptr->next->number;
    }
    
    /* Push FOR entry */
    next86_push_for(0x81, var_ptr, g_nxttxt, step_sign, (BYTE)ftype, 
                    step, limit, g_curlin, for_next_line, var_name);
    
    /* Initialize loop variable (typed) */
    if (ftype == VT_INTEGER) v->value.i = (int32_t)start;
    else if (ftype == VT_DOUBLE) v->value.d = start;
    else v->value.f = start;
    
    /* v145: first-iteration check (ASM GWMAIN FOR L1617: JMP NEXTS with
     * NXTFLG=0 → IFORIN compares the INITIAL value against the limit;
     * LOOPDN pops the entry and resumes after the matching NEXT — the
     * body runs 0 times and the variable keeps its initial value). */
    bool first_done;
    if (step_sign == 2) {
        first_done = false;   /* STEP 0 → infinite loop, never skip */
    } else if (step_sign) {
        first_done = (start > limit);
    } else {
        first_done = (start < limit);
    }
    if (first_done) {
        /* Pop the entry we just pushed and signal a jump past the loop.
         * Return value = line number AFTER the matching NEXT (depth-aware
         * scan, like ASM NXTSCN); gwmain treats rc > 0 as a jump target. */
        g_for_sp--;
        int next_line = next86_scan_matching_next(g_curlin);
        if (next_line > 0) {
            return next_line;   /* resume after the matching NEXT */
        }
        /* v145 MEDIUM-1: no line after the matching NEXT (NEXT is the last
         * program line) → program ends right here (ASM: NEWSTT walks past
         * the NEXT and finds no next statement). Return the END sentinel
         * (-2) — returning 0 would mean "continue with the next line",
         * wrongly executing the loop body. */
        return -2;
    }
    
    return 0;
}

/* Get FOR stack depth */
int next86_get_for_depth(void) {
    return g_for_sp;
}

/* Check if inside FOR loop */
int next86_inside_for(void) {
    return g_for_sp > 0;
}

/* Clear FOR stack */
void next86_clear_for_stack(void) {
    g_for_sp = 0;
}

/* GOSUB stack structure */
#define MAX_GOSUB_DEPTH 16

struct gosub_entry {
    WORD line;          /* Line number of GOSUB */
    WORD text_ptr;      /* Text pointer to return to */
};

static struct gosub_entry g_gosub_stack[MAX_GOSUB_DEPTH];
static int g_gosub_sp = 0;

/* v148 MEDIUM-1: GOSUB stack reset (GW-BASIC RUN clears the GOSUB stack) */
void next86_clear_gosub_stack(void) {
    g_gosub_sp = 0;
}

int next86_get_gosub_depth(void) {
    return g_gosub_sp;
}

/* GOTO statement handler */
int next86_goto(const char **text) {
    /* Parse GOTO statement: GOTO line_number */
    
    /* Skip leading whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse line number */
    int line_num = 0;
    while (**text && isdigit((unsigned char)**text)) {
        line_num = line_num * 10 + (**text - '0');
        (*text)++;
    }
    
    if (line_num <= 0 || line_num > 65529) {
        return -1;  /* Invalid line number */
    }
    
    /* Find the target line */
    struct program_line *target = gwlist_find_line(line_num);
    if (!target) {
        DEBUG_PRINT("GOTO: Line %d not found", line_num);
        return -2;  /* Line not found */
    }
    
    DEBUG_PRINT("GOTO: Jumping to line %d", line_num);
    /* Return the line number (positive) for jump */
    return line_num;
}

/* GOSUB statement handler */
/* Push GOSUB return and jump to a resolved line number (ASM SETGSB).
 * Shared by GOSUB <line> and ON <expr> GOSUB n1,n2,... */
int next86_gosub_to_line(int line_num) {
    if (line_num <= 0 || line_num > 65529) {
        return -1;  /* Invalid line number */
    }
    
    /* Find the target line */
    struct program_line *target = gwlist_find_line(line_num);
    if (!target) {
        DEBUG_PRINT("GOSUB: Line %d not found", line_num);
        return -2;  /* Line not found */
    }
    
    /* Push return address onto GOSUB stack */
    if (g_gosub_sp >= MAX_GOSUB_DEPTH) {
        return -3;  /* Stack overflow */
    }
    
    struct program_line *curr = gwlist_get_first_line();
    WORD return_line = 0;
    while (curr && curr->number != g_curlin) {
        curr = curr->next;
    }
    if (curr && curr->next) {
        return_line = curr->next->number;
    }
    
    g_gosub_stack[g_gosub_sp].line = g_curlin;
    g_gosub_stack[g_gosub_sp].text_ptr = return_line;
    g_gosub_sp++;
    
    DEBUG_PRINT("GOSUB: Jumping to line %d, return to line %d", line_num, return_line);
    /* Return the line number (positive) for jump */
    return line_num;
}

int next86_gosub(const char **text) {
    /* Parse GOSUB statement: GOSUB line_number */
    
    /* Skip leading whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse line number */
    int line_num = 0;
    while (**text && isdigit((unsigned char)**text)) {
        line_num = line_num * 10 + (**text - '0');
        (*text)++;
    }
    
    return next86_gosub_to_line(line_num);
}

/* RETURN statement handler */
int next86_return(const char **text) {
    (void)text;  /* Unused */
    
    if (g_gosub_sp == 0) {
        return -1;  /* RETURN without GOSUB */
    }
    
    /* Pop return address from GOSUB stack */
    g_gosub_sp--;
    WORD return_line = g_gosub_stack[g_gosub_sp].text_ptr;
    
    DEBUG_PRINT("RETURN: Returning to line %d", return_line);
    
    if (return_line == 0) {
        return 0;  /* Continue normally */
    }
    
    /* Return the line number (positive) for jump */
    return return_line;
}
