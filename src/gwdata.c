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
        char c = *name;
        /* Convert to uppercase for case-insensitive hashing */
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        hash = hash * 31 + (unsigned char)c;
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
    gwdata_lit_cache_reset();  /* v165: literals die with the heap */
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
        if (strcasecmp(v->name, name) == 0) {
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
        /* v165 MEDIUM-3: OOM sets Out of memory (7), not silent NULL */
        gwdata_set_error(7, g_state ? (uint16_t)g_state->current_line : 0, 0);
        return NULL;  /* Out of memory */
    }
    
    char *ptr = &g_state->str_heap.data[g_state->str_heap.used];
    g_state->str_heap.used += len;
    return ptr;
}

/* v165/v166 HIGH-1: literal interning — repeated string literals (loop
 * bodies like IF C$="" re-parse the RHS every iteration) reuse the
 * same heap allocation instead of draining the string heap.
 * v166: growable table — NO length/count caps (a fixed 32x64 table
 * still let >63-byte literals or the 33rd literal drain the heap).
 * Table entries are malloc'd and freed on reset (heap rebuild). */
typedef struct gw_lit_entry {
    char *text;
    char *ptr;
} gw_lit_entry;
static gw_lit_entry *g_lit_cache = NULL;
static int g_lit_count = 0;
static int g_lit_cap = 0;

void gwdata_lit_cache_reset(void) {
    for (int i = 0; i < g_lit_count; i++) free(g_lit_cache[i].text);
    free(g_lit_cache);
    g_lit_cache = NULL;
    g_lit_count = 0;
    g_lit_cap = 0;
}

char *gwdata_intern_literal(const char *text, size_t n) {
    /* v166: no length cap — long literals intern too */
    for (int i = 0; i < g_lit_count; i++) {
        if (strcmp(g_lit_cache[i].text, text) == 0) return g_lit_cache[i].ptr;
    }
    if (g_lit_count >= g_lit_cap) {
        int new_cap = g_lit_cap ? g_lit_cap * 2 : 32;
        gw_lit_entry *nc = (gw_lit_entry *)realloc(g_lit_cache,
                                                   new_cap * sizeof(gw_lit_entry));
        if (!nc) return NULL;  /* fall back to per-eval alloc */
        g_lit_cache = nc;
        g_lit_cap = new_cap;
    }
    /* v167 MEDIUM-4: allocate the table text FIRST — if it fails we
     * return without having consumed any string-heap space (the old
     * order leaked the heap allocation). */
    char *key = (char *)malloc(n + 1);
    if (!key) return NULL;
    memcpy(key, text, n + 1);
    char *ptr = gwdata_alloc_string(n + 1);
    if (!ptr) { free(key); return NULL; }
    memcpy(ptr, text, n + 1);
    g_lit_cache[g_lit_count].text = key;
    g_lit_cache[g_lit_count].ptr = ptr;
    g_lit_count++;
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
    /* Note: on_error_active should only be set by ON ERROR GOTO statement (via gwdata_set_on_error_goto) */
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

/* Clear only the error code, preserving error_line/error_text_ptr for RESUME */
void gwdata_clear_error(void) {
    if (!g_state) return;
    g_state->error_code = 0;
    /* Preserve error_line and error_text_ptr for RESUME NEXT/RESUME line */
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

/* Handle error - returns true if error was handled (jump to handler), false otherwise */
bool gwdata_handle_error(uint16_t line) {
    if (!g_state) return false;
    /* GW-BASIC: an error inside an active error handler is FATAL — never
     * re-enter the handler (prevents infinite OM handler loops). */
    if (gwdata_is_in_error_handler()) {
        printf("Error at line %d\n", line);
        return false;
    }
    if (gwdata_is_on_error_active()) {
        int handler_line = gwdata_get_on_error_line();
        if (handler_line > 0) {
            gwdata_enter_error_handler();
            /* Note: caller must set curr to handler_line after this returns true */
            return true;
        }
    }
    /* No ON ERROR handler or handler not found */
    printf("Error at line %d\n", line);
    return false;
}

/* DEF FN support */
struct variable *gwdata_alloc_array(const char *name, var_type_t type, int dim_count, const int *dims) {
    if (!g_state || dim_count <= 0 || dim_count > 8) return NULL;
    
    struct variable *v = gwdata_alloc_variable(name, type);
    if (!v) return NULL;
    
    v->is_array = true;
    v->dim_count = dim_count;
    
    /* Calculate total elements */
    int total_elements = 1;
    for (int i = 0; i < dim_count; i++) {
        if (dims[i] <= 0) return NULL;
        v->dims[i] = dims[i];
        total_elements *= dims[i];
    }
    
    /* Calculate element size based on type */
    size_t elem_size = 0;
    switch (type) {
        case VT_INTEGER: elem_size = sizeof(int32_t); break;
        case VT_SINGLE:  elem_size = sizeof(float); break;
        case VT_DOUBLE:  elem_size = sizeof(double); break;
        case VT_STRING:  elem_size = sizeof(char*); break;  /* String descriptor pointer */
        default: elem_size = sizeof(float); break;
    }
    
    size_t total_size = total_elements * elem_size;
    (void)total_size;  /* Reserved for future heap-quota checks */
    v->array_data = calloc(total_elements, elem_size);
    if (!v->array_data) return NULL;
    
    return v;
}

char *gwdata_get_txttab(void) {
    if (!g_state) return NULL;
    return g_state->prog_buf.data;
}

const char *gwdata_get_error_message(int err_code) {
    /* Error messages from GWDATA.ASM ERRTAB (1:1 order).
     * DCL macro: QQ=QQ+1 per entry, 1-indexed.
     * Non-disk errors: 1..30 (DCL count before QQ=49D).
     * Disk errors start at 50 ("FIELD overflow" .. "Rename across disks").
     * Slots 31..49 are unused in the original table. */
    static const char *messages[] = {
        [1]  = "NEXT without FOR",
        [2]  = "Syntax error",
        [3]  = "RETURN without GOSUB",
        [4]  = "Out of DATA",
        [5]  = "Illegal function call",
        [6]  = "Overflow",
        [7]  = "Out of memory",
        [8]  = "Undefined line number",
        [9]  = "Subscript out of range",
        [10] = "Duplicate Definition",
        [11] = "Division by zero",
        [12] = "Illegal direct",
        [13] = "Type mismatch",
        [14] = "Out of string space",
        [15] = "String too long",
        [16] = "String formula too complex",
        [17] = "Can't continue",
        [18] = "Undefined user function",
        [19] = "No RESUME",
        [20] = "RESUME without error",
        [21] = "Unprintable error",
        [22] = "Missing operand",
        [23] = "Line buffer overflow",
        [24] = "Device Timeout",
        [25] = "Device Fault",
        [26] = "FOR Without NEXT",
        [27] = "Out of Paper",
        [28] = "?",
        [29] = "WHILE without WEND",
        [30] = "WEND without WHILE",
        [50] = "FIELD overflow",
        [51] = "Internal error",
        [52] = "Bad file number",
        [53] = "File not found",
        [54] = "Bad file mode",
        [55] = "File already open",
        [56] = "?",
        [57] = "Device I/O Error",
        [58] = "File already exists",
        [59] = "?",
        [60] = "?",
        [61] = "Disk full",
        [62] = "Input past end",
        [63] = "Bad record number",
        [64] = "Bad file name",
        [65] = "?",
        [66] = "Direct statement in file",
        [67] = "Too many files",
        [68] = "Device Unavailable",
        [69] = "Communication buffer overflow",
        [70] = "Disk write protected",
        [71] = "Disk not Ready",
        [72] = "Disk media error",
        [73] = "Advanced Feature",
        [74] = "Rename across disks",
    };

    if (err_code >= 1 && err_code < (int)(sizeof(messages) / sizeof(messages[0]))) {
        return messages[err_code];
    }
    return "Unknown error";
}

/* DEF FN support */
struct variable *gwdata_find_fn_def(const char *name) {
    if (!g_state) return NULL;
    
    unsigned int idx = hash_name(name);
    struct variable *v = g_state->var_table[idx];
    
    while (v) {
        if (strcmp(v->name, name) == 0 && v->is_fn_def) {
            return v;
        }
        v = v->next;
    }
    return NULL;
}