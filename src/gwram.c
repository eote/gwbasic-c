/* GWRAM - Memory Management */
/* Based on GWRAM.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>

/* Allocation/free counters (v128 CRITICAL-1 regression): enable leak-balance
 * assertions in tests (alloc == free after reseq/error paths). */
static long g_alloc_count = 0;
static long g_free_count = 0;

long gwram_get_alloc_count(void) { return g_alloc_count; }
long gwram_get_free_count(void) { return g_free_count; }

void gwram_init(void) {
    /* No initialization needed for standard malloc-based implementation */
    g_alloc_count = 0;
    g_free_count = 0;
}

void *gwram_alloc(size_t size) {
    if (size == 0) return NULL;
    void *p = malloc(size);
    if (p) g_alloc_count++;
    return p;
}

void *gwram_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return gwram_alloc(size);
    }
    if (size == 0) {
        gwram_free(ptr);
        return NULL;
    }
    void *p = realloc(ptr, size);
    if (p) { g_alloc_count++; g_free_count++; }  /* realloc = free + alloc */
    return p;
}

void *gwram_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    
    /* Check for overflow */
    if (nmemb > SIZE_MAX / size) return NULL;
    
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
        g_alloc_count++;
    }
    return ptr;
}

void gwram_free(void *ptr) {
    if (ptr) {
        free(ptr);
        g_free_count++;
    }
}

/* Delete a file (for KILL statement) */
int gwram_delete_file(const char *filename) {
    if (!filename) return ERROR;
    if (remove(filename) == 0) {
        return SUCCESS;
    }
    return ERROR;
}