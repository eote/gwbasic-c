/* GW-BASIC C Port - Common Header */
/* Based on Microsoft GW-BASIC Interpreter Source Code (1983) */

#ifndef GWBASIC_H
#define GWBASIC_H

/* Standard headers FIRST */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Include windows.h on Windows to control ERROR macro */
#ifdef _WIN32
#include <windows.h>
#endif

/* Undefine ERROR from windows.h (it's 0) and define our own (-1) */
#ifdef ERROR
#undef ERROR
#endif
#define ERROR -1

/* Framebuffer max dimensions */
#define MAX_WIDTH  640
#define MAX_HEIGHT 480

/* Function result codes */
#define SUCCESS     0

/* Basic type definitions - use standard types to avoid conflicts with windows.h */
/* Note: windows.h already defines SBYTE, BYTE, SWORD, WORD, SDWORD, UDWORD on Windows */
#ifndef SBYTE
typedef int8_t    SBYTE;
#endif
#ifndef BYTE
typedef uint8_t   BYTE;
#endif
#ifndef SWORD
typedef int16_t   SWORD;
#endif
#ifndef WORD
typedef uint16_t  WORD;
#endif
#ifndef SDWORD
typedef int32_t   SDWORD;
#endif
#ifndef UDWORD
typedef uint32_t  UDWORD;
#endif

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
    bool is_array;
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
    struct variable *var_table[256];
    struct string_heap str_heap;
    struct program_buffer prog_buf;
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
void gengrp_line(const char *params);
void gengrp_grpini(void);
void gengrp_grprst(void);
void gengrp_set_atr(int color);
int gengrp_get_atr(void);
void gengrp_direct_pset(int x, int y, int color);

/* SCNDRV module functions - Screen Driver (text mode) */
void scndrv_init(void);
void scndrv_swi(int width, int height);
void scndrv_loc(int col, int row);
void scndrv_clr(void);
void scndrv_out(int ch);
int scndrv_pos(int *col, int *row);
int scndrv_wdo(int col, int row, int width, int height);
void scndrv_ipi(void);
void scndrv_brk(void);
int scndrv_gwi(void);
int scndrv_rdl(int line, char *buf, int maxlen);
void scndrv_mrk(bool is_input);

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
#define FM_INPUT  1
#define FM_OUTPUT 2
#define FM_RANDOM 3
#define FM_APPEND 4
#define FM_BINARY 5

struct file_desc;

void dskcom_init(void);

/* BIMISC module functions */
void bimisc_init(void);
void bimisc_new(void);
void bimisc_clear(void);
void bimisc_clear_with_args(int stack_size, int max_mem);
void bimisc_run_setup(void);
void bimisc_restore(int line_num);
void bimisc_stop(void);
void bimisc_end(void);
void bimisc_cont(void);
void bimisc_tron(void);
void bimisc_troff(void);
void bimisc_swap(const char *var1, const char *var2);
void bimisc_erase(const char *var_name);
int bimisc_is_letter(char c);
void bimisc_control_char(int ch);
int bimisc_is_file_io(void);
void bimisc_stack_init(void);
void bimisc_get_stack(int entries);

/* ADVGRP module functions - Advanced Graphics */
void advgrp_init(void);
void advgrp_paint(int x, int y, int fill_color, int border_color);
void advgrp_circle(int x, int y, int radius, int color, float start_angle, float end_angle, float aspect);
int advgrp_get(int x1, int y1, int x2, int y2, const char *array_name);
int advgrp_put(int x, int y, const char *array_name, int action);
int advgrp_line(int x1, int y1, int x2, int y2, int color);

/* OEM module functions - OEM Abstraction Layer */
void oem_init(void);
void oem_clrscn(int al);
void oem_clreol(int col, int row);
void oem_scroll(int src_col, int src_row, int dst_col, int dst_row, int width, int height);
int oem_scrinp(int col, int row);
void oem_scrout(int ch, int col, int row);
void oem_fetchc(int *ofs, int *mask);
void oem_storec(int ofs, int mask);
bool oem_swidth(int width, int height);
void oem_nsetcx(int count);
int oem_sndlpt(int printer, int ch);
bool oem_pekflt(int seg, int ofs, int *value);
bool oem_pokflt(int seg, int ofs, int value);
bool oem_setcbf(int segment, int size);
void oem_leftc(void);
void oem_rightc(void);
void oem_mapxyc(int x, int y);
bool oem_inkmap(int *ax, int *dx, bool carry);
bool oem_infmap(int *ax, int *dx, bool carry);
void oem_setfbc(int fg, int bg, bool graphics);
void oem_inicom(void);
const char* oem_gethed(void);
bool oem_tupc(void);
void oem_upc(void);
void oem_scalxy(int *x, int *y);
void oem_gtasp(int *bx, int *dx);
void oem_pntini(int border_color);
int oem_readc(void);
void oem_setc(void);
int oem_pixsiz(void);
void oem_pginit(int action, int *array_ptr, int bit_count);
void oem_nread(int *array_ptr, int bit_count);
void oem_nwrite(int *array_ptr, int bit_count);
bool oem_tdownc(void);
void oem_downc(void);
bool oem_lcpy(int param);
void oem_fkyfmt(int *count, int *chars, int *start);
bool oem_fkyadv(bool displaying);
bool oem_keyinp(int *ax, int *dx, bool *carry, bool *zero);
bool oem_edtmap(int *ax, bool *zero);
bool oem_prtmap(int *ax, bool *zero);
bool oem_csratr(int ah, int al, int bh, int bl, int ch, int cl);
void oem_gwini(void);
void oem_scnrst(void);
bool oem_scrstt(int *params, int param_count);
bool oem_setclr(int *params, int param_count, int mode);
int oem_scratr(int col, int row);
void oem_trmcom(void);
int oem_mapsup(int key);
void oem_segin(void);
void oem_gwterm(void);
void oem_grpsiz(int *width, int *height);
void oem_csrdsp(int type);
bool oem_setatr(int color);

/* OEMCBK module functions - Ctrl-Break & Pause Handling */
void oemcbk_init(void);
void oem_cbksta(void);
void oem_cbktrm(void);
BOOL oem_check_ctrl_break(void);
BOOL oem_check_pause(void);
void oem_simulate_ctrl_break(void);
void oem_simulate_pause(void);
void oem_process_keyboard_input(INPUT_RECORD *ir);

/* OEMEV module functions - Event Handling */
void oemev_init(void);
void oemev_evsta(void);
void oemev_evtrm(void);
BOOL oemev_pollev(void);
int oemev_rdpen(int func, int *result);
int oemev_rdtrig(int button, int mode, int *result);
int oemev_rdstik(int coord, int *result);

/* SCNEDT module functions - Screen Editor */
void scnedt_init(void);
void scnedt_pinlin(void);
void scnedt_qinlin(void);
void scnedt_inlin(void);
void scnedt_sinlin(void);
void scnedt_edtbrk(void);
void scnedt_clrflg(void);
void scnedt_erredt(int errlin);
void scnedt_edit(int line_num);
const char* scnedt_get_buf(void);
int scnedt_get_buf_len(void);
void scnedt_set_buf(const char *buf);

/* GIOLPT module functions - Line Printer */
void giolpt_init(void);
void giolpt_open(int device_id, int mode);
void giolpt_close(int device_id);
void giolpt_swd(int device_id, int width);
void giolpt_sot(int device_id, int ch);
int giolpt_gps(int device_id);
int giolpt_gwd(int device_id);
int giolpt_lpos(int device_id);

/* GIOCOM module functions - Serial Communications */
void giocom_init(void);
void giocom_open(int device_id, int baud, int parity, int data_bits, int stop_bits);
void giocom_close(int device_id);
void giocom_swd(int device_id, int width);
int giocom_sin(int device_id);
void giocom_sot(int device_id, int ch);
int giocom_loc(int device_id);
int giocom_lof(int device_id);
int giocom_gps(int device_id);
int giocom_gwd(int device_id);

/* GIODSK module functions - Disk I/O (merged with DSKCOM) */
void giodsk_init(void);
int giodsk_open(const char *filename, int filenum, int mode, int reclen);
int giodsk_close(int filenum);
int giodsk_close_all(void);
int giodsk_print(int filenum, const char *data);
int giodsk_write(int filenum, const void *data, int len);
int giodsk_input(int filenum, char *buffer, int maxlen);
int giodsk_line_input(int filenum, char *buffer, int maxlen);
int giodsk_seek(int filenum, long record);
int giodsk_get(int filenum, long record, void *buffer, int buflen);
int giodsk_put(int filenum, long record, const void *buffer, int buflen);

#endif /* GWBASIC_H */