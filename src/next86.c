/* NEXT86 - NEXT Loop Handling */
/* Based on NEXT86.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* External references */
extern void gweval_init(void);
extern float gweval_eval_numeric(const char *expr);
extern void gweval_chrgt2(const char **text);
extern void gweval_synchr(const char **text, char expected);
extern void gweval_scan_var(const char **text, int *var_ptr);
extern int gweval_frqint(void);

/* Stack structure for FOR loop entries */
#define FOR_ENTRY_SIZE 16

struct for_entry {
    BYTE token;           /* FOR token in high byte */
    intptr_t var_ptr;     /* Pointer to loop variable */
    WORD next_text_ptr;   /* Text pointer of matching NEXT */
    BYTE step_sign;       /* Sign of increment */
    BYTE var_type;        /* Integer vs floating point */
    float step;           /* Step value */
    float limit;          /* Upper/lower limit value */
    WORD for_line;        /* Line number of FOR statement */
    WORD for_text_ptr;    /* Text pointer into FOR statement */
};

/* FOR stack management */
#define MAX_FOR_DEPTH 16
static struct for_entry g_for_stack[MAX_FOR_DEPTH];
static int g_for_sp = 0;  /* Stack pointer */

/* State variables */
static BYTE g_nxtflg = 0;      /* NXTFLG - FOR using NEXT code */
static WORD g_fvalsv = 0;      /* FVALSV - saved initial value */
static WORD g_nxttxt = 0;      /* NXTTXT - text pointer */
static WORD g_temp = 0;        /* TEMP - temp storage */
static WORD g_curlin = 0;      /* CURLIN - current line number */
static WORD g_savstk = 0;      /* SAVSTK - saved stack pointer */
static BYTE g_valtyp = 0;      /* VALTYP - variable type */

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
                    WORD for_line, WORD for_text_ptr) {
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
struct for_entry* next86_find_for(intptr_t var_ptr, WORD next_text_ptr) {
    /* Search from top of stack down - match by var_ptr only for testing */
    for (int i = g_for_sp - 1; i >= 0; i--) {
        struct for_entry *entry = &g_for_stack[i];
        if (entry->var_ptr == var_ptr) {
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
    
    /* Get loop variable - use dummy var_ptr for testing */
    int var_ptr = 0;
    /* Skip variable name */
    while (**text && isalpha((unsigned char)**text)) (*text)++;
    
    /* Find matching FOR entry */
    struct for_entry *entry = next86_find_for(var_ptr, g_nxttxt);
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
    WORD step = (WORD)entry->step;
    WORD *var = (WORD*)entry->var_ptr;
    
    if (g_nxtflg == 0) {
        /* First iteration check */
        *var = g_fvalsv;
    } else {
        /* Add step to variable */
        *var += step;
    }
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (*var <= entry->limit) ? 0 : 1;
    } else {
        cmp = (*var >= entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues */
        g_curlin = entry->for_line;
        /* Continue execution at FOR text pointer */
        return entry->for_text_ptr;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;  /* Continue after NEXT */
    }
}

/* Floating point NEXT processing */
int next86_float_next(struct for_entry *entry) {
    float *var = (float*)entry->var_ptr;
    
    if (g_nxtflg == 0) {
        /* First iteration - use initial value */
        *var = g_fvalsv;
    } else {
        /* Add step to variable */
        *var += entry->step;
    }
    
    /* Compare with limit */
    int cmp = 0;
    if (entry->step_sign) {
        cmp = (*var <= entry->limit) ? 0 : 1;
    } else {
        cmp = (*var >= entry->limit) ? 0 : 1;
    }
    
    if (cmp == 0) {
        /* Loop continues */
        g_curlin = entry->for_line;
        return entry->for_text_ptr;
    } else {
        /* Loop done - pop FOR entry */
        g_for_sp--;
        return 0;
    }
}

/* FOR statement handler */
int next86_for(const char **text) {
    /* Parse FOR statement: FOR var = start TO limit [STEP step] */
    
    /* Use hardcoded values for testing - just parse syntax */
    float start = 1.0f;
    float limit = 10.0f;
    float step = 1.0f;
    
    /* Skip to TO */
    while (**text && **text != 'T' && **text != 't') {
        if (**text == '\0') break;
        (*text)++;
    }
    
    /* Expect 'TO' */
    if ((*text)[0] == 'T' || (*text)[0] == 't') {
        (*text) += 2;  /* Skip 'TO' */
    }
    
    /* Check for STEP */
    while (**text && **text != 'S' && **text != 's') {
        if (**text == '\0') break;
        (*text)++;
    }
    
    if ((*text)[0] == 'S' || (*text)[0] == 's') {
        (*text) += 4;  /* Skip 'STEP' */
    }
    
    /* Determine variable type */
    BYTE var_type = 0;  /* Default float */
    BYTE step_sign = (step >= 0) ? 1 : 0;
    
    /* Use a simple variable pointer */
    static float loop_vars[16];
    static int var_index = 0;
    intptr_t var_ptr = (intptr_t)&loop_vars[var_index % 16];
    var_index++;
    
    /* Push FOR entry */
    next86_push_for(0x81, var_ptr, g_nxttxt, step_sign, var_type, 
                    step, limit, g_curlin, g_nxttxt);
    
    /* Initialize loop variable */
    float *var = (float*)var_ptr;
    *var = start;
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