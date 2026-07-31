/* GW-BASIC C Port - Common Header */
/* Based on Microsoft GW-BASIC Interpreter Source Code (1983) */

#ifndef GWBASIC_H
#define GWBASIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Basic type definitions */
typedef int8_t    SBYTE;
typedef uint8_t   BYTE;
typedef int16_t   SWORD;
typedef uint16_t  WORD;
typedef int32_t   SDWORD;
typedef uint32_t  DWORD;

/* Function result codes */
#define SUCCESS     0
#define ERROR       -1

/* Memory limits */
#define MAX_LINE_LEN    255
#define MAX_VAR_NAME    40

/* Variable types (matching GW-BASIC) */
typedef enum {
    VT_INTEGER = 2,   /* % suffix */
    VT_SINGLE  = 4,   /* ! suffix (default) */
    VT_DOUBLE  = 8,   /* # suffix */
    VT_STRING  = 3    /* $ suffix */
} var_type_t;

/* Variable entry */
struct variable {
    char name[MAX_VAR_NAME + 1];
    var_type_t type;
    union {
        int32_t  i;
        float    f;
        double   d;
        char    *s;
    } value;
    struct variable *next;
};

/* String heap for dynamic string storage */
#define STRING_HEAP_SIZE  16384

struct string_heap {
    char data[STRING_HEAP_SIZE];
    size_t used;
};

/* Program buffer for tokenized lines */
#define PROGRAM_BUFFER_SIZE  32768

struct program_buffer {
    char data[PROGRAM_BUFFER_SIZE];
    size_t used;
};

/* Runtime state - defined in gwdata.c */
struct _runtime_state {
    bool running;
    int exit_code;
    struct variable *var_table[256];  /* hash table for variables */
    struct string_heap str_heap;
    struct program_buffer prog_buf;
    /* interpreter state */
    uint16_t current_line;
    bool in_direct_mode;
};

/* Global state (defined in gwdata.c) */
extern struct _runtime_state *g_state;

/* GWDATA module functions */
void gwdata_init(void);
struct variable *gwdata_alloc_variable(const char *name, var_type_t type);
struct variable *gwdata_find_variable(const char *name);
char *gwdata_alloc_string(size_t len);
void gwdata_free_string(char *s);

/* GWRAM module functions */
void gwram_init(void);
void *gwram_alloc(size_t size);
void *gwram_realloc(void *ptr, size_t size);
void *gwram_calloc(size_t nmemb, size_t size);
void gwram_free(void *ptr);

#endif /* GWBASIC_H */