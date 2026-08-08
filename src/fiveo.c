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

/* ---------- Module Initialization ---------- */

void fiveo_init(void) {
    g_while_sp = 0;
    g_write_active = false;
    g_write_file_num = 0;
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
