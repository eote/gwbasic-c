/* FIVEO - 5.0 Features: WHILE/WEND, CALL, CHAIN, WRITE */
/* Based on FIVEO.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * FIVEO.ASM implements four main features:
 * 1. WHILE/WEND - Loop control using a stack-based entry system
 * 2. CHAIN - Program chaining with COMMON variable preservation
 * 3. COMMON - Variable marking for CHAIN
 * 4. WRITE - Output with automatic quoting and delimiters
 *
 * This implementation provides WHILE/WEND stack management and
 * WRITE state management. CHAIN is stubbed (requires full program
 * loader integration).
 */

/* ---------- WHILE/WEND Stack ---------- */

#define MAX_WHILE_DEPTH 16  /* Match MAX_FOR_DEPTH in next86.c */

/* while_entry struct is defined in gwbasic.h */

static struct while_entry g_while_stack[MAX_WHILE_DEPTH];
static int g_while_sp = 0;

/* ---------- WRITE State ---------- */

static bool g_write_active = false;
static int g_write_file_num = 0;  /* 0 = console, >0 = file number */

/* ---------- COMMON / CHAIN State ---------- */

#define MAX_COMMON_VARS 256
static char g_common_names[MAX_COMMON_VARS][MAX_VAR_NAME + 1];
static int g_common_count = 0;
static int g_common_iter = 0;
static bool g_chain_merge_flag = false;
static int g_chain_line = 0;

/* ---------- Module Initialization ---------- */

void fiveo_init(void) {
    g_while_sp = 0;
    g_write_active = false;
    g_write_file_num = 0;
    g_common_count = 0;
    g_common_iter = 0;
    g_chain_merge_flag = false;
    g_chain_line = 0;
}

/* ---------- WHILE/WEND Stack Management ---------- */

/* Push a WHILE entry onto the stack
 * Returns 0 on success, -1 if stack overflow
 */
int fiveo_push_while(int while_text_ptr, int wend_text_ptr, int while_line) {
    if (g_while_sp >= MAX_WHILE_DEPTH) {
        return -1;  /* Stack overflow */
    }
    
    struct while_entry *entry = &g_while_stack[g_while_sp++];
    entry->while_text_ptr = while_text_ptr;
    entry->wend_text_ptr = wend_text_ptr;
    entry->while_line = while_line;
    
    return 0;
}

/* Pop a WHILE entry from the stack
 * Returns 0 on success, -1 if stack is empty
 */
int fiveo_pop_while(struct while_entry *entry) {
    if (g_while_sp <= 0) {
        return -1;  /* Stack underflow */
    }
    
    struct while_entry *src = &g_while_stack[--g_while_sp];
    if (entry) {
        entry->while_text_ptr = src->while_text_ptr;
        entry->wend_text_ptr = src->wend_text_ptr;
        entry->while_line = src->while_line;
    }
    
    return 0;
}

/* Get current while stack depth */
int fiveo_while_depth(void) {
    return g_while_sp;
}

/* Clear the entire while stack (e.g., on CLEAR/NEW/RUN) */
void fiveo_clear_while_stack(void) {
    g_while_sp = 0;
}

/* Find a WHILE entry by matching wend_text_ptr
 * Returns index of matching entry (0-based from bottom), or -1 if not found
 * Scans from top of stack downward (most recent first)
 */
int fiveo_find_while(int wend_text_ptr) {
    for (int i = g_while_sp - 1; i >= 0; i--) {
        if (g_while_stack[i].wend_text_ptr == wend_text_ptr) {
            return i;
        }
    }
    return -1;  /* Not found */
}

/* Peek at the top entry without popping
 * Returns 0 on success, -1 if stack is empty
 */
int fiveo_peek_while(struct while_entry *entry) {
    if (g_while_sp <= 0) {
        return -1;
    }
    
    struct while_entry *src = &g_while_stack[g_while_sp - 1];
    if (entry) {
        entry->while_text_ptr = src->while_text_ptr;
        entry->wend_text_ptr = src->wend_text_ptr;
        entry->while_line = src->while_line;
    }
    
    return 0;
}

/* ---------- WRITE State Management ---------- */

/* Reset WRITE state */
void fiveo_write_reset(void) {
    g_write_active = false;
    g_write_file_num = 0;
}

/* Start a WRITE statement to a file or console
 * filenum: 0 for console, >0 for file number
 */
void fiveo_write_begin(int filenum) {
    g_write_active = true;
    g_write_file_num = filenum;
}

/* End a WRITE statement */
void fiveo_write_end(void) {
    g_write_active = false;
    g_write_file_num = 0;
}

/* Check if currently in a WRITE statement */
bool fiveo_write_active(void) {
    return g_write_active;
}

/* Get the WRITE file number */
int fiveo_write_file(void) {
    return g_write_file_num;
}

/* ---------- CHAIN / COMMON Implementation ---------- */

/*
 * ASM CHAIN steps (FIVEO.ASM lines 194-695):
 *  1. Parse: CHAIN [MERGE] filename [, line#] [, ALL] [, DELETE range]
 *  2. Scan program for COMMON statements, mark variables
 *  3. Squeeze unmarked entries from symbol table (unless ALL)
 *  4. Copy string literals to string space
 *  5. Move simple vars + arrays to bottom of string space
 *  6. Load new program (MERGE overlays if specified)
 *  7. Move variables back down after program
 *  8. RUN the new program (optionally at specified line)
 *
 * C implementation simplifies: our variable table is a linked list
 * with heap allocation, not fixed memory segments. We:
 *  - Mark COMMON variables (step 2)
 *  - If not ALL: delete unmarked variables (step 3)
 *  - Load new program via giodsk (step 6)
 *  - MERGE: append lines instead of replacing (step 6)
 *  - Return line number for execution (step 8)
 */

/* Mark a variable as COMMON (preserved across CHAIN)
 * In ASM, this sets bit 7 of the 2nd char of the variable name.
 * In C, we track names in a list and set is_common on matching vars.
 */
void fiveo_common_mark(const char *varname) {
    if (!varname || !varname[0]) return;
    
    /* Check if already marked */
    for (int i = 0; i < g_common_count; i++) {
        if (strcasecmp(g_common_names[i], varname) == 0) {
            return;  /* Already marked */
        }
    }
    
    if (g_common_count >= MAX_COMMON_VARS) return;
    
    /* Add to list */
    strncpy(g_common_names[g_common_count], varname, MAX_VAR_NAME);
    g_common_names[g_common_count][MAX_VAR_NAME] = '\0';
    g_common_count++;
    
    /* Mark existing variable if it exists */
    struct variable *v = gwdata_find_variable(varname);
    if (v) {
        v->is_common = true;
    }
}

/* Check if a variable name is marked as COMMON */
bool fiveo_common_is_marked(const char *varname) {
    if (!varname || !varname[0]) return false;
    
    for (int i = 0; i < g_common_count; i++) {
        if (strcasecmp(g_common_names[i], varname) == 0) {
            return true;
        }
    }
    return false;
}

/* Clear all COMMON marks */
void fiveo_common_clear_all(void) {
    g_common_count = 0;
    
    /* Clear is_common on all variables */
    if (!g_state) return;
    for (int i = 0; i < 256; i++) {
        struct variable *v = g_state->var_table[i];
        while (v) {
            v->is_common = false;
            v = v->next;
        }
    }
}

/* Get list of marked COMMON variable names
 * Returns count, fills names[] up to max_count
 */
int fiveo_common_get_marked(char *names[], int max_count) {
    int count = (g_common_count < max_count) ? g_common_count : max_count;
    for (int i = 0; i < count; i++) {
        names[i] = g_common_names[i];
    }
    return g_common_count;
}

/* Reset iterator for listing COMMON vars */
void fiveo_common_list_reset(void) {
    g_common_iter = 0;
}

/* Get next COMMON variable name (iterator)
 * Returns NULL when exhausted
 */
const char *fiveo_common_list_next(void) {
    if (g_common_iter >= g_common_count) {
        return NULL;
    }
    return g_common_names[g_common_iter++];
}

/* Execute CHAIN statement
 * Syntax: CHAIN [MERGE] filename [, line_number] [, ALL] [, DELETE range]
 *
 * Parameters:
 *   filename   - program file to chain to
 *   line_number - starting line (0 = from beginning)
 *   merge      - if true, MERGE lines into current program
 *   all_vars   - if true, preserve ALL variables (not just COMMON)
 *
 * Returns:
 *   0 on success (program loaded, ready to RUN)
 *  -1 if file not found or load error
 *  -2 if MERGE specified but file read error
 *
 * Note: This function loads the program text via giodsk.
 * The caller is responsible for jumping to the specified line.
 */
int fiveo_chain(const char *filename, int line_number, bool merge, bool all_vars) {
    if (!filename || !filename[0]) {
        return -1;  /* No filename */
    }
    
    g_chain_merge_flag = merge;
    g_chain_line = line_number;
    
    /*
     * Step 2-3: Mark COMMON variables and delete unmarked (if not ALL).
     * In ASM, this scans the program text for COMMON statements.
     * In our implementation, variables are already marked via
     * fiveo_common_mark() calls when COMMON statements are parsed.
     * If all_vars is false, we delete non-COMMON variables.
     */
    if (!all_vars && g_state) {
        /* Delete all variables not marked COMMON */
        for (int i = 0; i < 256; i++) {
            struct variable *v = g_state->var_table[i];
            struct variable *prev = NULL;
            while (v) {
                struct variable *next = v->next;
                if (!v->is_common) {
                    /* Unlink and free non-COMMON variable */
                    if (prev) {
                        prev->next = next;
                    } else {
                        g_state->var_table[i] = next;
                    }
                    /* Free string data if string type */
                    if (v->type == VT_STRING && v->value.s) {
                        gwdata_free_string(v->value.s);
                    }
                    gwram_free(v);
                } else {
                    prev = v;
                }
                v = next;
            }
        }
    }
    
    /*
     * Step 4-5: In ASM, string literals are copied to string space
     * and variables are moved up. In our heap-based implementation,
     * COMMON variables are already preserved in the heap. No movement
     * needed. String literal handling is automatic via reference counting.
     */
    
    /*
     * Step 6: Load new program.
     * If MERGE, we append lines from the file to the existing program.
     * If not MERGE, we clear the program first.
     * 
     * For now, we open the file and read program lines.
     * A full implementation would use the tokenized program loader.
     * This simplified version reads lines as text and adds them via gwlist.
     */
    /* Use free file number to avoid conflicting with user's OPEN files */
    int filenum = giodsk_find_free_filenum();
    if (filenum == 0) {
        return -1;  /* No free file numbers */
    }
    int rc = giodsk_open(filename, filenum, 0, 0);  /* Mode 0 = input */
    if (rc != 0) {
        return -1;  /* File not found or open error */
    }
    
    /* If not MERGE, clear current program */
    if (!merge) {
        /* Delete all program lines using gwlist_clear to avoid memory leaks */
        gwlist_clear();
        /* Reset program buffer used count for consistent state */
        if (g_state) {
            g_state->prog_buf.used = 0;
        }
    }
    
    /* Read program lines from file */
    char line_buf[256];
    while (giodsk_input(filenum, line_buf, sizeof(line_buf) - 1) == 0) {
        /* Parse line number from start of line: "10 PRINT ..." */
        char *endptr;
        long linenum = strtol(line_buf, &endptr, 10);
        
        /* Check for overflow - GW-BASIC line numbers max 65535 */
        if (linenum > 65535) linenum = 65535;
        
        char *p = endptr;
        
        /* Skip whitespace between number and text */
        while (*p == ' ' || *p == '\t') p++;
        
        if (linenum > 0 && *p) {
            gwlist_add_line((int)linenum, p);
        }
    }
    
    giodsk_close(filenum);
    
    /*
     * Step 7: Move variables back down.
     * Not needed in heap implementation - COMMON variables are
     * already in the heap and remain accessible.
     */
    
    /*
     * Step 8: Set up to run the chained program.
     * If line_number > 0 , the caller should start at that line.
     * If line_number == 0, start from the beginning.
     * The caller checks g_chain_line to decide.
     */
    
    /* Reset stacks for the new program */
    fiveo_clear_while_stack();
    
    return 0;  /* Success */
}

/* Get the CHAIN line number (where to start execution after CHAIN) */
int fiveo_get_chain_line(void) {
    return g_chain_line;
}

/* Get the CHAIN MERGE flag */
bool fiveo_get_chain_merge(void) {
    return g_chain_merge_flag;
}
