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
        g_state->exit_code = SUCCESS;
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
            /* Clear value based on new type - union needs proper member clearing */
            if (type == VT_STRING) {
                existing->value.s.len = 0;
                existing->value.s.ptr = NULL;
            } else {
                existing->value.f = 0.0f;  /* Clear numeric value (also clears i and d) */
            }
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

/* GWDATA module - ON ERROR / RESUME / ERROR support */

void gwdata_set_error(int code, uint16_t line, int text_ptr) {
    if (!g_state) return;
    g_state->error_code = code;
    g_state->error_line = line;
    g_state->error_text_ptr = text_ptr;
    g_state->on_error_active = true;
}

int gwdata_get_error(void) {
    if (!g_state) return 0;
    return g_state->error_code;
}

uint16_t gwdata_get_error_line(void) {
    if (!g_state) return 0;
    return g_state->error_line;
}

int gwdata_get_error_text_ptr(void) {
    if (!g_state) return 0;
    return g_state->error_text_ptr;
}

void gwdata_clear_error(void) {
    if (!g_state) return;
    g_state->error_code = 0;
    g_state->error_line = 0;
    g_state->error_text_ptr = 0;
}

bool gwdata_is_on_error_active(void) {
    if (!g_state) return false;
    return g_state->on_error_active;
}

void gwdata_set_on_error_goto(int line, int text_ptr) {
    if (!g_state) return;
    g_state->on_error_line = line;
    g_state->on_error_text_ptr = text_ptr;
    g_state->on_error_active = (line != 0);
}

void gwdata_clear_on_error(void) {
    if (!g_state) return;
    g_state->on_error_line = 0;
    g_state->on_error_text_ptr = 0;
    g_state->on_error_active = false;
}

void gwdata_enter_error_handler(void) {
    if (!g_state) return;
    g_state->in_error_handler = true;
}

void gwdata_exit_error_handler(void) {
    if (!g_state) return;
    g_state->in_error_handler = false;
}

bool gwdata_is_in_error_handler(void) {
    if (!g_state) return false;
    return g_state->in_error_handler;
}

int gwdata_get_on_error_line(void) {
    if (!g_state) return 0;
    return g_state->on_error_line;
}