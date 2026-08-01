/* GWDATA - Global Data Structures */
/* Based on GWDATA.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>

#define MAX_VARIABLES  256

/* Global state definition */
struct _runtime_state *g_state = NULL;

static unsigned int hash_name(const char *name) {
    unsigned int hash = 0;
    while (*name) {
        hash = hash * 31 + (unsigned char)(*name);
        name++;
    }
    return hash % MAX_VARIABLES;
}

void gwdata_init(void) {
    if (g_state) {
        /* Reset variable table for test isolation - free all variables and arrays */
        for (int i = 0; i < MAX_VARIABLES; i++) {
            struct variable *v = g_state->var_table[i];
            while (v) {
                struct variable *next = v->next;
                if (v->is_array) {
                    biptrg_free_array(v);
                }
                free(v);
                v = next;
            }
            g_state->var_table[i] = NULL;
        }
        g_state->str_heap.used = 0;
        g_state->prog_buf.used = 0;
        g_state->current_line = 0;
        g_state->in_direct_mode = true;
        g_state->running = true;
        return;
    }
    
    g_state = (struct _runtime_state *)calloc(1, sizeof(struct _runtime_state));
    if (!g_state) return;
    
    g_state->running = true;
    g_state->exit_code = SUCCESS;
    g_state->in_direct_mode = true;
    g_state->current_line = 0;
    g_state->str_heap.used = 0;
    g_state->prog_buf.used = 0;
    
    /* Initialize variable table */
    for (int i = 0; i < MAX_VARIABLES; i++) {
        g_state->var_table[i] = NULL;
    }
}

struct variable *gwdata_alloc_variable(const char *name, var_type_t type) {
    if (!g_state) return NULL;
    
    unsigned int idx = hash_name(name);
    
    /* Check if already exists */
    struct variable *existing = g_state->var_table[idx];
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            /* If existing variable is an array, free it first */
            if (existing->is_array) {
                biptrg_free_array(existing);
            }
            existing->type = type;
            existing->is_array = false;
            existing->value.s = NULL;  /* Clear any stale pointer */
            return existing;
        }
        existing = existing->next;
    }
    
    /* Allocate new variable */
    struct variable *v = (struct variable *)calloc(1, sizeof(struct variable));
    if (!v) return NULL;
    
    strncpy(v->name, name, MAX_VAR_NAME);
    v->name[MAX_VAR_NAME] = '\0';
    v->type = type;
    v->is_array = false;
    v->next = g_state->var_table[idx];
    g_state->var_table[idx] = v;
    
    return v;
}

struct variable *gwdata_find_variable(const char *name) {
    if (!g_state) return NULL;
    
    unsigned int idx = hash_name(name);
    struct variable *v = g_state->var_table[idx];
    
    while (v) {
        if (strcmp(v->name, name) == 0) {
            return v;
        }
        v = v->next;
    }
    return NULL;
}

char *gwdata_alloc_string(size_t len) {
    if (!g_state) return NULL;
    
    /* Align to 2 bytes */
    len = (len + 1) & ~1;
    
    if (g_state->str_heap.used + len > STRING_HEAP_SIZE) {
        return NULL;  /* Out of memory */
    }
    
    char *ptr = &g_state->str_heap.data[g_state->str_heap.used];
    g_state->str_heap.used += len;
    return ptr;
}

void gwdata_free_string(char *s) {
    /* Simple bump allocator - no actual free, just for interface compatibility */
    (void)s;
    /* In a real implementation, we'd have a free list or garbage collection */
}