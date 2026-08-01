/* GW-BASIC C Port - Common Header */
/* Based on Microsoft GW-BASIC Interpreter Source Code (1983) */

#ifndef GWBASIC_H
#define GWBASIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <windows.h>

/* Basic type definitions */
typedef int8_t    SBYTE;
typedef uint8_t   BYTE;
typedef int16_t   SWORD;
typedef uint16_t  WORD;
typedef int32_t   SDWORD;
typedef uint32_t  UDWORD;

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
    bool is_array;          /* 标记是否为数组，解决 union 别名问题 */
    union {
        int32_t  i;
        float    f;
        double   d;
        char    *s;
    } value;
    struct variable *next;
};

/* Program line for tokenized program storage */
struct program_line {
    int number;
    char *text;
    struct program_line *next;
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

/* GWINIT module functions */
void gwinit_init(void);

/* GWEVAL module functions */
void gweval_init(void);
float gweval_eval_numeric(const char *expr);
char *gweval_eval_string(const char *expr);

/* GWSTS module functions */
void gwsts_init(void);
int gwsts_exec_statement(const char *stmt);

/* GIOCON module functions */
void giocon_init(void);
void giocon_print_num(float val);
void giocon_print_str(const char *str);
void giocon_locate(int row, int col);
void giocon_color(int fg, int bg);
void giocon_cls(void);

/* GIOKYB module functions */
void giokyb_init(void);
char *giokyb_inkey(void);
char *giokyb_input(int n);

/* GIOSCN module functions */
void gioscn_init(void);
void gioscn_screen(int mode);
void gioscn_pset(int x, int y, int color);
void gioscn_preset(int x, int y);
int gioscn_point(int x, int y);

/* GWLIST module functions */
void gwlist_init(void);
void gwlist_add_line(int number, const char *text);
struct program_line *gwlist_find_line(int number);
void gwlist_delete_line(int number);
void gwlist_list(int start, int end);
struct program_line *gwlist_get_first_line(void);

/* BISTRS module functions */
void bistrs_init(void);
char *bistrs_left(const char *str, int n);
char *bistrs_right(const char *str, int n);
char *bistrs_mid(const char *str, int start, int len);
int bistrs_len(const char *str);
int bistrs_asc(const char *str);
char *bistrs_chr(int code);
char *bistrs_str(double val);
double bistrs_val(const char *str);
char *bistrs_string(int n, char ch);
char *bistrs_space(int n);

/* BIPRTU module functions */
void biprtu_init(void);
void biprtu_print_using(const char *fmt, double val);
void biprtu_print_using_str(const char *fmt, const char *str);

/* BIPTRG module functions */
void biptrg_init(void);
int biptrg_dim_array(const char *name, int size, var_type_t type);
int biptrg_set_array_element(const char *name, int index, float val);
float biptrg_get_array_element(const char *name, int index);
int biptrg_set_array_string(const char *name, int index, const char *str);
char *biptrg_get_array_string(const char *name, int index);
void biptrg_free_array(struct variable *v);

/* DSKCOM module - File I/O */
/* File modes */
#define FM_INPUT  1
#define FM_OUTPUT 2
#define FM_RANDOM 3
#define FM_APPEND 4
#define FM_BINARY 5

/* File descriptor structure */
struct file_desc {
    int filenum;              /* GW-BASIC file number (1-255) */
    int mode;                 /* FM_INPUT, FM_OUTPUT, etc. */
    HANDLE handle;            /* Windows file handle */
    char *filename;           /* Full path */
    int record_length;        /* For random files */
    struct file_desc *next;   /* Linked list */
};

void dskcom_init(void);
int dskcom_open(const char *filename, int filenum, int mode, int reclen);
int dskcom_close(int filenum);
struct file_desc *dskcom_find_file(int filenum);
int dskcom_print(int filenum, const char *data);
int dskcom_write(int filenum, const void *data, int len);
int dskcom_input(int filenum, char *buffer, int maxlen);
int dskcom_line_input(int filenum, char *buffer, int maxlen);
int dskcom_seek(int filenum, long record);
int dskcom_get(int filenum, long record, void *buffer, int buflen);
int dskcom_put(int filenum, long record, const void *buffer, int buflen);

#endif /* GWBASIC_H */