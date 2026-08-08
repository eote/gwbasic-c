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

/* Device name table entry */
typedef struct {
    const char *name;
    int device_id;
} device_name_t;

/* Device dispatch table */
typedef struct {
    const char *name;
    void *dsp_addr;
    void *ini_addr;
    void *trm_addr;
} device_dispatch_t;

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
    bool is_common;     /* COMMON flag: variable preserved across CHAIN */
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

/* WHILE/WEND stack entry - matches FIVEO.ASM WHLSIZ=7 format */
struct while_entry {
    int while_text_ptr;  /* Text pointer after WHILE keyword */
    int wend_text_ptr;   /* Text pointer after WEND keyword */
    int while_line;       /* Line number of WHILE statement */
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
void gweval_scan_var(const char **text, int *var_ptr);
int gweval_frqint(void);
void gweval_chrgt2(const char **text);
void gweval_synchr(const char **text, char expected);

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

/* FIVEO module functions - WHILE/WEND stack, WRITE, CHAIN */
void fiveo_init(void);
int fiveo_push_while(int while_text_ptr, int wend_text_ptr, int while_line);
int fiveo_pop_while(struct while_entry *entry);
int fiveo_while_depth(void);
void fiveo_clear_while_stack(void);
int fiveo_find_while(int wend_text_ptr);
int fiveo_peek_while(struct while_entry *entry);
void fiveo_write_reset(void);
void fiveo_write_begin(int filenum);
void fiveo_write_end(void);
bool fiveo_write_active(void);
int fiveo_write_file(void);

/* FIVEO CHAIN/COMMON functions */
int fiveo_chain(const char *filename, int line_number, bool merge, bool all_vars);
void fiveo_common_mark(const char *varname);
void fiveo_common_clear_all(void);
int fiveo_common_get_marked(char *names[], int max_count);
bool fiveo_common_is_marked(const char *varname);
void fiveo_common_list_reset(void);
const char *fiveo_common_list_next(void);
int fiveo_get_chain_line(void);
bool fiveo_get_chain_merge(void);

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

/* OEMSND module functions - Sound (SOUND, PLAY) */
void oemsnd_init(void);
void oemsnd_term(void);
void oemsnd_start(void);
int oemsnd_donote(int mode, WORD frequency, WORD duration);
void oemsnd_sndsta(void);
void oemsnd_sndtrm(void);
void oemsnd_play(const char *music_string);
void oemsnd_sound(WORD frequency, WORD duration);

/* IBMRES module functions - IBM Reserved Words / Extended Tokens */
void ibmres_init(void);
void ibmres_term(void);
int ibmres_is_reserved_word(const char *text, WORD *out_token);
WORD ibmres_get_token(const char *name);
const char* ibmres_get_token_name(WORD token);
BYTE ibmres_get_keyword_type(const char *name);
int ibmres_crunch_extended(const char **text, WORD *out_token);
const char* ibmres_list_token(WORD token);
int ibmres_eval_extended(const char **text, float *result);
int ibmres_list_extended(WORD token, char *buffer, int buflen);

/* ITSA86 module functions - Resident Initialization for 8086 */
void itsa86_init(void);
WORD itsa86_basvar(WORD bx, WORD ax, int carry);
void itsa86_mapini(void);
void itsa86_mapclc(void);
int itsa86_seg_off(WORD *bx);
void itsa86_get_memory_info(WORD *maxmem, WORD *freemem, WORD *txttab, WORD *vartab);
void itsa86_set_c_option(WORD size);
void itsa86_set_m_option(WORD size);
void itsa86_dump_memory_map(void);
void itsa86_csend(void);

/* Accessor functions for testing */
WORD itsa86_get_maxmem(void);
WORD itsa86_get_newds(void);
WORD itsa86_get_data_seg(void);
WORD itsa86_get_savseg(void);
WORD itsa86_get_txttab(void);

/* BASVAR table access for testing */
WORD** itsa86_get_bvtab(void);

/* IBMRES module functions - IBM Reserved Words / Extended Tokens */
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

/* CALL86 module functions - CALL absolute, USR functions */
void call86_init(void);
void call86_call(const char **text);
void call86_calls(const char **text);
float call86_usr(int usr_num, float arg);
void call86_defusr(int usr_num, int address);

/* NEXT86 module functions - NEXT Loop Handling */
void next86_init(void);
int next86_next(const char **text);
int next86_for(const char **text);
int next86_get_for_depth(void);
int next86_inside_for(void);
void next86_clear_for_stack(void);

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

/* KANJ86 module functions - Kanji String Functions */
void kanj86_init(void);
void kanj86_term(void);
int kanj86_ktnfn(const char **text);
int kanj86_jisfn(const char **text);
int kanj86_klenfn(const char **text);
int kanj86_kposfn(const char **text);

/* MACLNG module functions - Macro Language Driver */
void maclng_init(void);
void maclng_term(void);
void maclng_maclng(const char **text);
void maclng_mclxeq(const char **text);
void maclng_maclng_main(const char **text);
void maclng_fetchr(const char **text);
void maclng_fetchz(const char **text);
void maclng_decfet(const char **text);
void maclng_valscn(const char **text);
void maclng_valsc2(const char **text);
void maclng_varget(const char **text);
void maclng_negd(const char **text);
void maclng_islet2(const char **text);
void maclng_scvar(const char **text);
void maclng_mclnew(const char **text);
void maclng_mclscn(const char **text);

/* GIOCAS module functions - Cassette I/O */
void giocas_init(void);
void giocas_term(void);
int giocas_motor(const char **text);
int giocas_read(const char **text, char *buffer, int maxlen);
int giocas_write(const char **text, const char *buffer, int len);
int giocas_ioctl(const char **text, int cmd, void *arg);

/* GIO86 module functions - 8086-specific I/O */
void gio86_init(void);
void gio86_term(void);
int gio86_open(const char **text);
int gio86_close(const char **text);
int gio86_width(const char **text);
int gio86_bsave(const char **text);
int gio86_bload(const char **text);
int gio86_lprint(const char **text);
int gio86_print(const char **text);
int gio86_bparms(const char **text);
int gio86_opnfil(void);
int gio86_pofnum(const char **text);
int gio86_porlen(const char **text);
int gio86_pnrlen(const char **text);
int gio86_getwdt(const char **text);

/* GIOTBL module functions - I/O Tables */
void giotbl_init(void);
void giotbl_term(void);
int giotbl_get_device_id(const char *name);
const char* giotbl_get_device_name(int device_id);
int giotbl_get_dispatch(int device_id, void **dsp, void **ini, void **trm);
int giotbl_get_ndev(void);
int giotbl_lookup_device(const char *name, int *device_id);
const device_name_t* giotbl_get_name_table(void);
const device_dispatch_t* giotbl_get_dispatch_table(void);
int giotbl_find_device(const char *name);
void giotbl_init_all(void);

/* OEM module accessor functions for testing */
BYTE oem_get_pixbts(void);
void oem_set_pixbts(BYTE val);
WORD oem_get_pixofs(void);
void oem_set_pixofs(WORD val);
BYTE oem_get_atrmsk(void);
void oem_set_atrmsk(BYTE val);
BYTE oem_get_atrpat(void);
void oem_set_atrpat(BYTE val);
BYTE oem_get_borpat(void);
void oem_set_borpat(BYTE val);
WORD oem_get_pgtary(void);
void oem_set_pgtary(WORD val);
WORD oem_get_pgtshf(void);
void oem_set_pgtshf(WORD val);
WORD oem_get_pgtact(void);
void oem_set_pgtact(WORD val);
BYTE oem_get_crtmod(void);
void oem_set_crtmod(BYTE val);
BYTE oem_get_crtwid(void);
void oem_set_crtwid(BYTE val);
BYTE oem_get_lincnt(void);
void oem_set_lincnt(BYTE val);
BYTE oem_get_atrmsk(void);
void oem_set_atrmsk(BYTE val);
BYTE oem_get_atrpat(void);
void oem_set_atrpat(BYTE val);
BYTE oem_get_borpat(void);
void oem_set_borpat(BYTE val);
WORD oem_get_pgtary(void);
void oem_set_pgtary(WORD val);
WORD oem_get_pgtshf(void);
void oem_set_pgtshf(WORD val);
WORD oem_get_pgtact(void);
void oem_set_pgtact(WORD val);

/* BIBOOT module functions */
void biboot_init(void);
void *biboot_lstvar(void);

/* IBMRES module functions */
void ibmres_init(void);
void ibmres_term(void);
const struct keyword_entry* ibmres_lookup_keyword(const char *name);
WORD ibmres_get_token(const char *name);
const char* ibmres_get_token_name(WORD token);
int ibmres_is_reserved_word(const char *text, WORD *out_token);
BYTE ibmres_get_keyword_type(const char *name);
int ibmres_crunch_extended(const char **text, WORD *out_token);
int ibmres_eval_extended(const char **text, float *result);
const char* ibmres_list_token(WORD token);
int ibmres_list_extended(WORD token, char *buffer, int buflen);

/* Token types */
#define TK_STMT 0x00    /* 1-byte statement token */
#define TK_FUNC 0x80    /* 1-byte function token (high bit set) */
#define TK_STMT_EXT 0xFE /* 2-byte extended statement token (FE xx) */
#define TK_FUNC_EXT 0xFD /* 2-byte extended function token (FD xx) */

/* IBMRES helper functions */
extern int ibmres_is_extended_token(BYTE token);
extern int ibmres_get_token_length(BYTE first_byte);

/* OEMEV module functions */
void oemev_init(void);
void oemev_evsta(void);
void oemev_evtrm(void);
BOOL oemev_pollev(void);
int oemev_rdpen(int func, int *result);
int oemev_rdtrig(int button, int mode, int *result);
int oemev_rdstik(int coord, int *result);

/* OEMSND module functions */
void oemsnd_init(void);
void oemsnd_term(void);
void oemsnd_start(void);
int oemsnd_donote(int mode, WORD frequency, WORD duration);
void oemsnd_sound(WORD frequency, WORD duration);
void oemsnd_play(const char *music_string);
void oemsnd_sndsta(void);
void oemsnd_sndtrm(void);
void oemsnd_play(const char *music_string);  /* alias */
void oemsnd_sound(WORD frequency, WORD duration);

/* SCNEDT module functions */
void scnedt_init(void);
void scnedt_clrflg(void);
void scnedt_edtbrk(void);
void scnedt_pinlin(void);
void scnedt_qinlin(void);
void scnedt_inlin(void);
void scnedt_sinlin(void);
void scnedt_set_buf(const char *buf);
const char* scnedt_get_buf(void);
int scnedt_get_buf_len(void);
void scnedt_clrflg(void);
void scnedt_edtbrk(void);
void scnedt_erredt(int errlin);
void scnedt_edit(int line_num);
void scnedt_scnsem(void);
const char* scnedt_get_buf(void);
int scnedt_get_buf_len(void);
void scnedt_set_buf(const char *buf);

/* OEMCBK module functions */
void oemcbk_init(void);
void oem_cbksta(void);
void oem_cbktrm(void);
BOOL oem_check_ctrl_break(void);
BOOL oem_check_pause(void);
void oem_simulate_ctrl_break(void);
void oem_simulate_pause(void);
void oem_process_keyboard_input(INPUT_RECORD *ir);

/* GENGRP module functions */
void gengrp_init(void);
void gengrp_grpini(void);
void gengrp_grprst(void);
void gengrp_scan_coord(int *x, int *y, bool relative);
void gengrp_pset(int x, int y, int color);
void gengrp_preset(int x, int y);
int gengrp_point(int x, int y);
void gengrp_set_atr(int color);
void gengrp_direct_pset(int x, int y, int color);
int gengrp_get_atr(void);
void gengrp_line(const char *params);

/* GENGRP accessor functions for testing */
int gengrp_get_gxpos(void);
int gengrp_get_gypos(void);
int gengrp_get_grpacx(void);
int gengrp_get_grpacy(void);
int gengrp_get_screen_mode(void);
int gengrp_get_fb_width(void);
int gengrp_get_fb_height(void);
void gengrp_set_screen_mode(int mode);
void gengrp_set_grpacx(int x);
void gengrp_set_grpacy(int y);
void gengrp_set_gxpos(int x);
void gengrp_set_gypos(int y);

/* GIODSK module functions */
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

/* GIOLPT module functions */
void giolpt_init(void);
void giolpt_open(int device_id, int mode);
void giolpt_close(int device_id);
void giolpt_swd(int device_id, int width);
int giolpt_gwd(int device_id);
void giolpt_sot(int device_id, int ch);
int giolpt_gps(int device_id);
int giolpt_gwd(int device_id);
int giolpt_lpos(int device_id);

/* GIOCOM module functions */
void giocom_init(void);
void giocom_open(int device_id, int baud, int parity, int data_bits, int stop_bits);
void giocom_close(int device_id);
void giocom_swd(int device_id, int width);
int giocom_gwd(int device_id);
void giocom_sot(int device_id, int ch);
int giocom_loc(int device_id);
int giocom_lof(int device_id);
int giocom_gps(int device_id);
int giocom_gwd(int device_id);

/* GWLIST module functions */
void gwlist_init(void);
void gwlist_add_line(int number, const char *text);
struct program_line *gwlist_find_line(int number);
void gwlist_delete_line(int number);
void gwlist_list(int start, int end);
struct program_line *gwlist_get_first_line(void);

#endif /* GWBASIC_H */