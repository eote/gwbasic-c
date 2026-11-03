/* BIMISC - Miscellaneous Built-in Functions */
/* Based on BIMISC.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* External references */
extern void gwlist_init(void);
extern void dskcom_close_all(void);
extern void math2_init(void);  /* For RND reset */
extern int assign_value(const char *var_ref, const char *expr);  /* From gwsts.c */
extern int assign_value_raw(const char *var_ref, const char *expr);  /* v162 HIGH-1: READ values are literal text */

/* Trap handling state */
#define MAX_TRAPS 16
static unsigned char trap_table[MAX_TRAPS * 3];  /* flag, gosub_addr_low, gosub_addr_high */
static unsigned char ong_sbf = 0;  /* Global trap flag counter */
static bool trace_flag = false;

/* Stack management */
#define NUMLEV 8  /* Minimum stack levels required */

/* Forward declarations */
static void init_trap_table(void);

/* Module initialization */
void bimisc_init(void) {
    init_trap_table();
    ong_sbf = 0;
    trace_flag = false;
}

/* Initialize trap table - called from CLEARC/NEW */
static void init_trap_table(void) {
    for (int i = 0; i < MAX_TRAPS * 3; i++) {
        trap_table[i] = 0;
    }
    ong_sbf = 0;
}

/* NEW command - clear program text and variable space */
void bimisc_new(void) {
    if (!g_state) return;
    
    /* Clear program lines */
    gwlist_init();
    
    /* Clear variables and arrays */
    for (int i = 0; i < 256; i++) {
        struct variable *v = g_state->var_table[i];
        while (v) {
            struct variable *next = v->next;
            if (v->is_array) {
                biptrg_free_array(v);
            }
            /* NOTE: string pointers point into the static string heap —
             * never pass them to gwram_free (invalid free). The heap is
             * reclaimed wholesale below via str_heap.used = 0. */
            gwram_free(v);
            v = next;
        }
        g_state->var_table[i] = NULL;
    }
    
    /* Reset string heap and program buffer */
    g_state->str_heap.used = 0;
    g_state->prog_buf.used = 0;
    
    /* Reset interpreter state */
    g_state->current_line = 0;
    g_state->in_direct_mode = true;
    
    /* Close all open files */
    dskcom_close_all();
    
    /* Initialize trap table */
    init_trap_table();
    
    /* Reset RND sequence */
    math2_init();
}

/* CLEAR command - clear variables, optionally set stack/memory */
void bimisc_clear(void) {
    bimisc_clear_with_args(0, 0);  /* Default values */
}

/* CLEAR with parameters: CLEAR [,[max_mem][,stack_size]] */
void bimisc_clear_with_args(int stack_size, int max_mem) {
    if (!g_state) return;
    
    /* Close all files first */
    dskcom_close_all();
    
    /* Clear variables and arrays */
    for (int i = 0; i < 256; i++) {
        struct variable *v = g_state->var_table[i];
        while (v) {
            struct variable *next = v->next;
            if (v->is_array) {
                biptrg_free_array(v);
            }
            /* NOTE: string pointers point into the static string heap
             * (g_state->str_heap.data) — they must NOT be passed to
             * gwram_free (invalid free). The heap is reclaimed wholesale
             * below via str_heap.used = 0. */
            gwram_free(v);
            v = next;
        }
        g_state->var_table[i] = NULL;
    }
    
    /* Reset string heap */
    g_state->str_heap.used = 0;
    
    /* Reset RND sequence */
    math2_init();
    
    /* Stack/memory parameters would adjust MEMSIZ/STKLOW in full implementation */
    (void)stack_size;
    (void)max_mem;
}

/* RUN command setup - called before executing program */
void bimisc_run_setup(void) {
    if (!g_state) return;
    
    /* Clear variables but keep program */
    for (int i = 0; i < 256; i++) {
        struct variable *v = g_state->var_table[i];
        while (v) {
            struct variable *next = v->next;
            if (v->is_array) {
                biptrg_free_array(v);
            }
            /* NOTE: string pointers point into the static string heap —
             * never pass them to gwram_free (invalid free). The heap is
             * reclaimed wholesale below via str_heap.used = 0. */
            gwram_free(v);
            v = next;
        }
        g_state->var_table[i] = NULL;
    }
    
    /* Reset string heap */
    g_state->str_heap.used = 0;
    
    /* Close all files */
    dskcom_close_all();
    
    /* Initialize trap table */
    init_trap_table();
    
    /* Reset RND sequence */
    math2_init();
    
    /* Reset error state */
    g_state->current_line = 0;
    g_state->exit_code = SUCCESS;
}

/* RESTORE [line] - reset DATA pointer */
void bimisc_restore(int line_num) {
    if (!g_state) return;
    
    if (line_num == 0) {
        /* RESTORE without line number - reset to beginning */
        g_state->prog_buf.used = 0;  /* Simplified: reset to start of program */
        bimisc_data_init();
    } else {
        /* RESTORE with line number - find line and set DATA pointer */
        struct program_line *line = gwlist_find_line(line_num);
        if (line) {
            /* In full implementation, would set DATPTR to after this line */
            (void)line;
        }
    }
}

/* STOP statement */
void bimisc_stop(void) {
    if (!g_state) return;
    g_state->running = false;
    g_state->exit_code = SUCCESS;
}

/* END statement */
void bimisc_end(void) {
    if (!g_state) return;
    g_state->running = false;
    g_state->exit_code = SUCCESS;
    
    /* Close all files */
    dskcom_close_all();
}

/* CONT command - continue after STOP/END/^C */
void bimisc_cont(void) {
    if (!g_state) return;
    
    /* In full implementation, would check OLDTXT/OLDLIN and resume */
    /* For now, just continue running */
    g_state->running = true;
}

/* TRON - trace on */
void bimisc_tron(void) {
    trace_flag = true;
}

/* TROFF - trace off */
void bimisc_troff(void) {
    trace_flag = false;
}

/* SWAP var1, var2 - exchange values of two variables */
void bimisc_swap(const char *var1, const char *var2) {
    if (!g_state || !var1 || !var2) return;
    
    struct variable *v1 = gwdata_find_variable(var1);
    struct variable *v2 = gwdata_find_variable(var2);
    
    if (!v1 || !v2) return;
    
    /* Check type compatibility */
    if (v1->type != v2->type) return;  /* Type mismatch */
    
    if (v1->is_array || v2->is_array) return;  /* Can't swap arrays */
    
    /* Swap values based on type */
    switch (v1->type) {
        case VT_INTEGER: {
            int32_t tmp = v1->value.i;
            v1->value.i = v2->value.i;
            v2->value.i = tmp;
            break;
        }
        case VT_SINGLE: {
            float tmp = v1->value.f;
            v1->value.f = v2->value.f;
            v2->value.f = tmp;
            break;
        }
        case VT_DOUBLE: {
            double tmp = v1->value.d;
            v1->value.d = v2->value.d;
            v2->value.d = tmp;
            break;
        }
        case VT_STRING: {
            int tmp_len = v1->value.s.len;
            char *tmp_ptr = v1->value.s.ptr;
            v1->value.s.len = v2->value.s.len;
            v1->value.s.ptr = v2->value.s.ptr;
            v2->value.s.len = tmp_len;
            v2->value.s.ptr = tmp_ptr;
            break;
        }
    }
}

/* ERASE array - deallocate array storage */
void bimisc_erase(const char *var_name) {
    if (!g_state || !var_name) return;
    
    struct variable *v = gwdata_find_variable(var_name);
    if (!v || !v->is_array) return;
    
    biptrg_free_array(v);
    v->is_array = false;
    v->value.s.ptr = NULL;
    v->value.s.len = 0;
}

/* Test if character is a letter (A-Z) */
int bimisc_is_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/* Handle control characters (^C, etc.) */
void bimisc_control_char(int ch) {
    /* In full implementation, would handle ^C (interrupt), ^O (output control), etc. */
    (void)ch;
}

/* Check if file I/O is active */
int bimisc_is_file_io(void) {
    /* In full implementation, would check PTRFIL chain */
    return 0;
}

/* Stack initialization (STKINI) - reset stack, string temps, I/O flags */
void bimisc_stack_init(void) {
    if (!g_state) return;
    
    /* Reset string temporary pointer */
    /* In full implementation, would reset TEMPPT, SAVSTK, SP, etc. */
    g_state->current_line = 0;
    g_state->in_direct_mode = true;
    
    /* Reset RND */
    math2_init();
    
    /* Close all files */
    dskcom_close_all();
    
    /* Initialize trap table */
    init_trap_table();
}

/* Check stack space (GETSTK) - ensure minimum stack entries available */
void bimisc_get_stack(int entries) {
    /* In full implementation, would check SP against STKLOW */
    /* For now, just a stub */
    (void)entries;
}

/* Trap handling functions */

/* Turn trap ON */
void bimisc_trap_on(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    unsigned char old = *trap_flag_ptr;
    unsigned char new_val = (old & 4) | 1;  /* Keep REQUEST bit, add ON bit */
    
    if (new_val != old) {
        *trap_flag_ptr = new_val;
        if (new_val & 4) {  /* If REQUEST bit was set */
            ong_sbf++;
        }
    }
}

/* Turn trap OFF */
void bimisc_trap_off(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    *trap_flag_ptr = 0;
    if (ong_sbf > 0) ong_sbf--;
}

/* Stop trap */
void bimisc_trap_stop(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    *trap_flag_ptr |= 2;  /* Set STOP bit */
    if (ong_sbf > 0) ong_sbf--;
}

/* Reset trap */
void bimisc_trap_reset(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    unsigned char old = *trap_flag_ptr;
    unsigned char new_val = old & (1 | 4);  /* Keep ON and REQUEST bits */
    
    if (new_val != old) {
        *trap_flag_ptr = new_val;
        if ((old & (1 | 4)) == (1 | 4)) {  /* Was ON and REQUEST */
            ong_sbf++;
        }
    }
}

/* Request trap */
void bimisc_trap_request(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    if (*trap_flag_ptr & 1) {  /* If ON bit set */
        *trap_flag_ptr |= 4;   /* Set REQUEST bit */
        ong_sbf++;
    }
}

/* Free trap */
void bimisc_trap_free(unsigned char *trap_flag_ptr) {
    if (!trap_flag_ptr) return;
    
    unsigned char old = *trap_flag_ptr;
    *trap_flag_ptr &= (1 | 2);  /* Keep ON and STOP bits */
    
    if ((old & (1 | 4)) == (1 | 4)) {  /* Was ON and REQUEST */
        if (ong_sbf > 0) ong_sbf--;
    }
}

/* Check and execute pending traps (GOTRP) */
int bimisc_check_traps(void) {
    if (!g_state) return 0;
    if (ong_sbf == 0) return 0;
    if (g_state->in_direct_mode) return 0;  /* Can't trap from direct mode */
    if (g_state->current_line == 0xFFFF) return 0;  /* Can't trap from error handler */
    
    for (int i = 0; i < MAX_TRAPS; i++) {
        unsigned char flag = trap_table[i * 3];
        if ((flag & (1 | 4)) == (1 | 4)) {  /* ON and REQUEST */
            unsigned char addr_low = trap_table[i * 3 + 1];
            unsigned char addr_high = trap_table[i * 3 + 2];
            
            if (addr_low != 0 || addr_high != 0) {
                /* Trap has a GOSUB address - would execute it */
                /* In full implementation, would call GOSUB2 with the address */
                bimisc_trap_free(&trap_table[i * 3]);
                bimisc_trap_stop(&trap_table[i * 3]);
                return 1;  /* Trap executed */
            }
        }
    }
    return 0;
}

/* Set trap GOSUB address (for ON ... GOSUB) */
void bimisc_set_trap_address(int trap_num, int line_number) {
    if (trap_num < 0 || trap_num >= MAX_TRAPS) return;
    
    trap_table[trap_num * 3 + 1] = (unsigned char)(line_number & 0xFF);
    trap_table[trap_num * 3 + 2] = (unsigned char)((line_number >> 8) & 0xFF);
}

/* Get trace flag status */
int bimisc_get_trace_flag(void) {
    return trace_flag ? 1 : 0;
}

/* READ/DATA support implementation */

/* Initialize DATA pointer at program start */
void bimisc_data_init(void) {
    if (!g_state) return;
    g_state->data_ptr = gwlist_get_first_line();
    g_state->data_index = 0;
}

/* Parse a single value from DATA statement text
 * Returns pointer to next character after the parsed value (or NULL on error)
 * value buffer must be at least 256 bytes
 */
static char *parse_data_value(char *text, char *value, size_t value_size) {
    if (!text || !value) return NULL;

    /* Skip leading whitespace */
    while (*text && isspace((unsigned char)*text)) text++;
    if (!*text) return NULL;

    char *start = text;
    bool in_quotes = false;

    while (*text) {
        if (*text == '\"') {
            in_quotes = !in_quotes;
            text++;
        } else if (*text == ',' && !in_quotes) {
            break;
        } else {
            text++;
        }
    }

    size_t len = text - start;
    if (len >= value_size) len = value_size - 1;
    memcpy(value, start, len);
    value[len] = '\0';

    /* Trim whitespace from the parsed value */
    char *trim_start = value;
    char *trim_end = value + strlen(value) - 1;
    while (trim_end >= trim_start && isspace((unsigned char)*trim_end)) {
        *trim_end = '\0';
        trim_end--;
    }
    while (*trim_start && isspace((unsigned char)*trim_start)) trim_start++;

    /* Strip surrounding quotes if present */
    size_t trimmed_len = strlen(trim_start);
    if (trimmed_len >= 2 && trim_start[0] == '\"' && trim_start[trimmed_len - 1] == '\"') {
        trim_start[trimmed_len - 1] = '\0';
        trim_start++;
        /* Copy trimmed value back to value buffer */
        memmove(value, trim_start, strlen(trim_start) + 1);
    } else if (trim_start != value) {
        /* Copy trimmed value back to value buffer */
        memmove(value, trim_start, strlen(trim_start) + 1);
    }

    /* If we have a comma, advance past it */
    if (*text == ',') text++;

    return text;
}

/* READ statement: READ var1, var2, ...
 * Parses variable names from text, reads values from DATA statements
 * Returns SUCCESS or ERROR
 */
int bimisc_read(const char **text, int var_count, char **var_names) {
    if (!g_state || !text || var_count <= 0 || !var_names) return ERROR;
    
    /* Get current DATA pointer - if none, start from beginning */
    if (!g_state->data_ptr) {
        g_state->data_ptr = gwlist_get_first_line();
        g_state->data_index = 0;
    }
    
    for (int i = 0; i < var_count; i++) {
        char *var_name = var_names[i];
        if (!var_name) return ERROR;
        
        /* Find a DATA statement with remaining values */
        char value[256];
        bool found = false;
        
        while (g_state->data_ptr) {
            char *line_text = g_state->data_ptr->text;
            if (!line_text) {
                g_state->data_ptr = g_state->data_ptr->next;
                g_state->data_index = 0;
                continue;
            }
            
            /* Check if this line is a DATA statement */
            char *trimmed = line_text;
            while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
            
            if (strncasecmp(trimmed, "DATA", 4) == 0 && 
                (trimmed[4] == ' ' || trimmed[4] == '\t' || trimmed[4] == '\0')) {
                /* This is a DATA statement - parse values from it */
                char *data_text = trimmed + 4;
                while (*data_text && isspace((unsigned char)*data_text)) data_text++;
                
                /* Skip already parsed values */
                for (int j = 0; j < g_state->data_index; j++) {
                    data_text = parse_data_value(data_text, value, sizeof(value));
                    if (!data_text) break;
                }
                
                if (data_text && *data_text) {
                    /* Parse the next value */
                    char *next_text = parse_data_value(data_text, value, sizeof(value));
                    if (next_text || strlen(value) > 0) {
                        /* Assign value to variable */
                        if (assign_value_raw(var_name, value) != SUCCESS) {
                            return ERROR;
                        }
                        g_state->data_index++;
                        found = true;
                        break;
                    }
                }
                
                /* No more values in this DATA statement */
                g_state->data_ptr = g_state->data_ptr->next;
                g_state->data_index = 0;
            } else {
                g_state->data_ptr = g_state->data_ptr->next;
                g_state->data_index = 0;
            }
        }
        
        if (!found) {
            /* Out of DATA */
            return ERROR;
        }
    }
    
    return SUCCESS;
}