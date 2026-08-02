/* GW-BASIC C Port - Common Header */
/* Based on Microsoft GW-BASIC Interpreter Source Code (1983) */

#ifndef GWBASIC_H
#define GWBASIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Windows.h defines ERROR as 0, but we need it as -1 */
#ifdef ERROR
#undef ERROR
#endif

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

/* GENGRP module functions - Generalized Graphics */
void gengrp_init(void);
void gengrp_scan_coord(int *x, int *y, bool relative);
void gengrp_pset(int x, int y, int color);
void gengrp_preset(int x, int y);
int gengrp_point(int x, int y);
void gengrp_line(const char *params);  /* LINE command parser */
void gengrp_grpini(void);              /* Initialize graphics */
void gengrp_grprst(void);              /* Reset graphics */
void gengrp_set_atr(int color);        /* Set current attribute */
int gengrp_get_atr(void);              /* Get current attribute */

/* SCNDRV module functions - Screen Driver (text mode) */
void scndrv_init(void);
void scndrv_swi(int width, int height);      /* SCNSWI - set screen width/height */
void scndrv_loc(int col, int row);           /* SCNLOC - locate cursor */
void scndrv_clr(void);                       /* SCNCLR - clear screen */
void scndrv_out(int ch);                     /* SCNOUT - output character */
int scndrv_pos(int *col, int *row);          /* SCNPOS - get cursor position */
int scndrv_wdo(int col, int row, int width, int height);  /* SCNWDO - set text window */
void scndrv_ipi(void);                       /* SCNIPL - initialize */
void scndrv_brk(void);                       /* SCNBRK - break handler */
int scndrv_gwi(void);                        /* SCNGWI - get logical width */
int scndrv_rdl(int line, char *buf, int maxlen);  /* SCNRDL - read logical line */
void scndrv_mrk(bool is_input);              /* SCNMRK - mark position */

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

/* MATH1 module functions - Transcendental math */
void math1_init(void);
float math1_sin(float x);
float math1_cos(float x);
float math1_tan(float x);
float math1_atn(float x);
float math1_exp(float x);
float math1_log(float x);

/* MATH2 module functions - Elementary math */
void math2_init(void);
float math2_sqr(float x);
float math2_abs(float x);
float math2_sgn(float x);
float math2_rnd(float x);
float math2_int(float x);
float math2_fix(float x);

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

/* File descriptor structure - opaque handle for cross-platform compatibility */
/* Defined in dskcom.h */
struct file_desc;

/* DSKCOM module functions are declared in dskcom.h */
void dskcom_init(void);

/* BIMISC module functions */
void bimisc_init(void);
void bimisc_new(void);              /* NEW command - clear program and variables */
void bimisc_clear(void);            /* CLEAR command - clear variables, optionally set memory */
void bimisc_clear_with_args(int stack_size, int max_mem);  /* CLEAR with parameters */
void bimisc_run_setup(void);        /* RUN command setup */
void bimisc_restore(int line_num);  /* RESTORE [line] */
void bimisc_stop(void);             /* STOP statement */
void bimisc_end(void);              /* END statement */
void bimisc_cont(void);             /* CONT command */
void bimisc_tron(void);             /* TRON command */
void bimisc_troff(void);            /* TROFF command */
void bimisc_swap(const char *var1, const char *var2);  /* SWAP var1, var2 */
void bimisc_erase(const char *var_name);  /* ERASE array */
int bimisc_is_letter(char c);       /* Test if character is A-Z */
void bimisc_control_char(int ch);   /* Handle control characters (^C, etc.) */
int bimisc_is_file_io(void);        /* Check if file I/O active */
void bimisc_stack_init(void);       /* Stack initialization (STKINI) */
void bimisc_get_stack(int entries); /* Check stack space (GETSTK) */

/* GIOLPT module functions - Line Printer */
void giolpt_init(void);
void giolpt_open(int device_id, int mode);      /* LPTOPN - open printer */
void giolpt_close(int device_id);               /* LPTCLS - close printer */
void giolpt_swd(int device_id, int width);      /* LPTSWD - set device width */
void giolpt_sot(int device_id, int ch);         /* LPTSOT - sequential output */
int giolpt_gps(int device_id);                  /* LPTGPS - get position */
int giolpt_gwd(int device_id);                  /* LPTGWD - get width */
int giolpt_lpos(int device_id);                 /* LPOS function */

/* GIOCOM module functions - Serial Communications */
void giocom_init(void);
void giocom_open(int device_id, int baud, int parity, int data_bits, int stop_bits);  /* COMOPN */
void giocom_close(int device_id);               /* COMCLS */
void giocom_swd(int device_id, int width);      /* COMSWD - set width */
int giocom_sin(int device_id);                  /* COMSIN - sequential input */
void giocom_sot(int device_id, int ch);         /* COMSOT - sequential output */
int giocom_loc(int device_id);                  /* COMLOC - bytes in input queue */
int giocom_lof(int device_id);                  /* COMLOF - free space in input buffer */
int giocom_gps(int device_id);                  /* COMGPS - get position */
int giocom_gwd(int device_id);                  /* COMGWD - get width */

#endif /* GWBASIC_H */