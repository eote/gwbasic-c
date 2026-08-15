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
extern float gweval_eval_numeric(const char *expr);
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
        g_for_stack[i].for_text_ptr = 0;
    }
}

/* Forward declarations */
static int next86_int_next(struct for_entry *entry);
static int next86_float_next(struct for_entry *entry);

/* Push a FOR entry onto the stack */
int next86_push_for(BYTE token, intptr_t var_ptr, WORD next_text_ptr, 
                    BYTE step_sign, BYTE var_type, float step, float limit,
                    WORD for_line, WORD for_text_ptr, const char *var_name) {
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
    entry->for_text_ptr = for_text_ptr;
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
struct for_entry* next86_find_for(const char *var_name, WORD next_text_ptr) {
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
    int nxtflg_value = 0;
    
    /* Check if FOR is using NEXT code */
    if (**text != '\0') {
        nxtflg_value = 1;
    }
    g_nxtflg = nxtflg_value;
    
    g_nxttxt = (WORD)(uintptr_t)*text;
    
    /* Parse variable name */
    char var_name[41] = {0};
    int var_name_len = 0;
    while (**text && isalpha((unsigned char)**text) && var_name_len < 40) {
        var_name[var_name_len++] = **text;
        (*text)++;
    }
    var_name[var_name_len] = '\0';
    
    /* Find matching FOR entry */
    struct for_entry *entry = next86_find_for(var_name, g_nxttxt);
    if (!entry) {
        return -1;  /* NEXT without FOR error */
    }
    
    /* Set up stack by chopping to FOR entry */
    g_savstk = g_for_sp;  /* Simplified */
    
    /* Process based on variable type */
    if (entry->var_type & 0x80) {
        /* Integer FOR loop */
        return next86_int_next(entry);
    } else {
        /* Floating point FOR loop */
        return next86_float_next(entry);
    }
}

/* Integer NEXT processing */
int next86_int_next(struct for_entry *entry) {
    struct variable *v = (struct variable*)entry->var_ptr;
    WORD step = (WORD)entry->step;
    
    if (g_nxtflg == 0) {
        /* First iteration check */
        v->value.i = g_fvalsv;
    } else {
        /* Add step to variable */
        v->value.i += (WORD)entry->step;
    }
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (v->value.i <= (WORD)entry->limit) ? 0 : 1;
    } else {
        cmp = (v->value.i >= (WORD)entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues */
        g_curlin = entry->for_line;
        /* Continue execution at FOR text pointer */
        return (int)entry->for_text_ptr;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;  /* Continue after NEXT */
    }
}

/* Floating point NEXT processing */
int next86_float_next(struct for_entry *entry) {
    struct variable *v = (struct variable*)entry->var_ptr;
    
    if (g_nxtflg == 0) {
        /* First iteration - use initial value */
        v->value.f = (float)g_fvalsv;
    } else {
        /* Add step to variable */
        v->value.f += entry->step;
    }
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (v->value.f <= entry->limit) ? 0 : 1;
    } else {
        cmp = (v->value.f >= entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues */
        g_curlin = entry->for_line;
        return (int)entry->for_text_ptr;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;
    }
}

/* FOR statement handler */
int next86_for(const char **text) {
    /* Parse FOR statement: FOR var = start TO limit [STEP step] */
    
    /* Skip leading whitespace */
    while (**text && isspace((unsigned char)**text)) (*text)++;
    
    /* Parse variable name */
    char var_name[41] = {0};
    int var_name_len = 0;
    while (**text && (isalnum((unsigned char)**text) || **text == '_') && var_name_len < 40) {
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
    
    /* Determine variable type */
    BYTE var_type = 0;  /* Default float */
    BYTE step_sign = (step >= 0) ? 1 : 0;
    
    /* Allocate variable in GW-BASIC variable table */
    struct variable *v = gwdata_alloc_variable(var_name, VT_SINGLE);
    if (!v) {
        return ERROR;
    }
    v->value.f = start;
    intptr_t var_ptr = (intptr_t)v;
    
    /* Save text pointer AFTER the FOR statement for loop continuation */
    /* We need to find the next line in the program after this FOR line */
    struct program_line *for_line_ptr = gwlist_get_first_line();
    while (for_line_ptr && for_line_ptr->number != g_curlin) {
        for_line_ptr = for_line_ptr->next;
    }
    WORD for_text_ptr = 0;
    if (for_line_ptr && for_line_ptr->next) {
        for_text_ptr = (WORD)(uintptr_t)for_line_ptr->next->text;
    }
    
    /* Push FOR entry */
    next86_push_for(0x81, var_ptr, g_nxttxt, step_sign, var_type, 
                    step, limit, g_curlin, for_text_ptr, var_name);
    
    /* Initialize loop variable */
    v->value.f = start;
    g_fvalsv = (WORD)start;  /* Save initial value for first iteration check */
    
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
    
    DEBUG_PRINT("GOTO: Jumping to line %d (text_ptr=%p)", line_num, target->text);
    /* Return the text pointer of the target line */
    return (int)(uintptr_t)target->text;
}

/* GOSUB statement handler */
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
    
    if (line_num <= 0 || line_num > 65529) {
        return -1;  /* Invalid line number */
    }
    
    /* Find the target line */
    struct program_line *target = gwlist_find_line(line_num);
    if (!target) {
        return -2;  /* Line not found */
    }
    
    /* Push return address onto GOSUB stack */
    if (g_gosub_sp >= MAX_GOSUB_DEPTH) {
        return -3;  /* Stack overflow */
    }
    
    struct program_line *curr = gwlist_get_first_line();
    WORD return_text_ptr = 0;
    while (curr && curr->number != g_curlin) {
        curr = curr->next;
    }
    if (curr && curr->next) {
        return_text_ptr = (WORD)(uintptr_t)curr->next->text;
    }
    
    g_gosub_stack[g_gosub_sp].line = g_curlin;
    g_gosub_stack[g_gosub_sp].text_ptr = return_text_ptr;
    g_gosub_sp++;
    
    /* Return the text pointer of the target line */
    return (int)(uintptr_t)target->text;
}

/* RETURN statement handler */
int next86_return(const char **text) {
    (void)text;  /* Unused */
    
    if (g_gosub_sp == 0) {
        return -1;  /* RETURN without GOSUB */
    }
    
    /* Pop return address from GOSUB stack */
    g_gosub_sp--;
    WORD return_text_ptr = g_gosub_stack[g_gosub_sp].text_ptr;
    
    if (return_text_ptr == 0) {
        return 0;  /* Continue normally */
    }
    
    return (int)return_text_ptr;
}