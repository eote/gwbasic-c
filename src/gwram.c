/* GWRAM - Memory Management */
/* Based on GWRAM.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>

void gwram_init(void) {
    /* No initialization needed for standard malloc-based implementation */
}

void *gwram_alloc(size_t size) {
    if (size == 0) return NULL;
    return malloc(size);
}

void *gwram_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return gwram_alloc(size);
    }
    if (size == 0) {
        gwram_free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

void *gwram_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    
    /* Check for overflow */
    if (nmemb > SIZE_MAX / size) return NULL;
    
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void gwram_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}