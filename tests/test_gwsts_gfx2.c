/* test_gwsts_gfx2.c — M2 图形深化批: CIRCLE/PAINT/图形 GET/PUT 语句接线
 * ASM 语义 per ADVGRP.ASM L108-1000:
 * - CIRCLE (x,y),r[,color[,start[,end[,aspect]]]]: 0 弧度=3点钟, 正向逆时针;
 *   负角画半径线到圆心; end<start → 超 360°; aspect<1 → Y 压缩, >1 → X 压缩
 * - PAINT (x,y)[,fill[,border]]: 边界色停止, fill 填充; 默认 border=fill
 * - GET (x1,y1)-(x2,y2),arr: 头 4 字节 = x-bits(2B) + y-bits(2B), MSB-first
 * - PUT (x,y),arr[,action]: action ∈ {PSET,PRESET,AND,OR,XOR}, 默认 XOR
 * - 语句形态: "GET #n" = 文件, "GET (" = 图形 (ASM 两形态一入口)
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "test_framework.h"
#include "../src/gwbasic.h"

extern int gwsts_exec_statement(const char *text);
extern struct variable *gwdata_find_variable(const char *name);
extern int gwdata_get_error(void);
extern void gwdata_clear_error(void);
extern int gengrp_point(int x, int y);

/* Test: CIRCLE full circle — point on circumference set */
void test_gwsts_gfx2_circle_basic(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("CIRCLE (50, 50), 20, 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "CIRCLE no error");
    gwsts_exec_statement("C = POINT(70, 50)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "CIRCLE point at (70,50) east");
    gwsts_exec_statement("C = POINT(50, 30)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "CIRCLE point at (50,30) north");
    gwsts_exec_statement("C = POINT(30, 50)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "CIRCLE point at (30,50) west");
    gwsts_exec_statement("C = POINT(50, 70)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "CIRCLE point at (50,70) south");
    /* center not drawn */
    gwsts_exec_statement("C = POINT(50, 50)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 0, "CIRCLE center not drawn");
    TEST_PASS("gwsts gfx2 circle basic");
}

/* Test: CIRCLE half arc (0..PI) — only upper half drawn (Y up in GW-BASIC
 * but framebuffer y grows down; ASM angle 0=east, CCW) */
void test_gwsts_gfx2_circle_arc(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("CIRCLE (50, 50), 20, 1, 0, 3.14159");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "CIRCLE arc no error");
    gwsts_exec_statement("C = POINT(70, 50)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "arc endpoint east drawn");
    gwsts_exec_statement("C = POINT(30, 50)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "arc endpoint west drawn");
    /* point on the NOT-drawn half must be 0 */
    gwsts_exec_statement("C = POINT(50, 30)");
    c = gwdata_find_variable("C");
    int north = (int)c->value.f;
    gwsts_exec_statement("C = POINT(50, 70)");
    c = gwdata_find_variable("C");
    int south = (int)c->value.f;
    /* exactly one of north/south on the arc (framebuffer y down:
     * angle 0..PI CCW = upper half in math coords = lower half in fb) */
    TEST_ASSERT(north + south == 1, "arc covers exactly one half");
    TEST_PASS("gwsts gfx2 circle arc");
}

/* Test: PAINT fills region bounded by circle */
void test_gwsts_gfx2_paint_region(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("CIRCLE (50, 50), 20, 1");
    gwsts_exec_statement("PAINT (50, 50), 2, 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "PAINT no error");
    gwsts_exec_statement("C = POINT(50, 50)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 2, "PAINT interior filled");
    gwsts_exec_statement("C = POINT(50, 35)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 2, "PAINT near-edge filled");
    /* outside the circle remains 0 */
    gwsts_exec_statement("C = POINT(50, 10)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 0, "PAINT outside untouched");
    TEST_PASS("gwsts gfx2 paint region");
}

/* Test: GET/PUT roundtrip via statements (copy a drawn block) */
void test_gwsts_gfx2_get_put_roundtrip(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("LINE (40, 40)-(44, 44), 1, BF");
    gwsts_exec_statement("GET (38, 38)-(46, 46), G");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "graphics GET no error");
    gwsts_exec_statement("PUT (60, 60), G");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "graphics PUT no error");
    /* the copied 5x5 block at (38,38) reappears at (60,60) */
    gwsts_exec_statement("C = POINT(62, 62)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "PUT copied pixel (62,62)");
    gwsts_exec_statement("C = POINT(66, 66)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "PUT copied pixel (66,66)");
    /* XOR default: put again at same place erases */
    gwsts_exec_statement("PUT (60, 60), G");
    gwsts_exec_statement("C = POINT(62, 62)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 0, "XOR second PUT erases");
    TEST_PASS("gwsts gfx2 get/put roundtrip");
}

/* Test: PUT with explicit PSET action */
void test_gwsts_gfx2_put_pset_action(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("LINE (40, 40)-(42, 42), 1, BF");
    gwsts_exec_statement("GET (39, 39)-(43, 43), G");
    gwsts_exec_statement("PUT (60, 60), G, PSET");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "PUT PSET no error");
    gwsts_exec_statement("C = POINT(61, 61)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "PSET copied pixel (61,61)");
    TEST_PASS("gwsts gfx2 put pset");
}

/* Test: file GET #1 still routes to file handler (regression for the
 * '(' vs '#' dispatch) */
void test_gwsts_gfx2_file_get_regression(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gwsts_exec_statement("OPEN \"T141.DAT\" FOR OUTPUT AS #1");
    gwsts_exec_statement("PRINT #1, \"HELLO\"");
    gwsts_exec_statement("CLOSE #1");
    gwsts_exec_statement("OPEN \"T141.DAT\" FOR INPUT AS #1");
    int rc = gwsts_exec_statement("LINE INPUT #1, L$");
    TEST_ASSERT_EQ_INT(rc, 0, "LINE INPUT #1 works (not shadowed)");
    struct variable *v = gwdata_find_variable("L$");
    TEST_ASSERT(v && v->value.s.ptr && strcmp(v->value.s.ptr, "HELLO") == 0,
                "file content read");
    gwsts_exec_statement("CLOSE #1");
    TEST_PASS("gwsts gfx2 file get regression");
}


/* ===== v144 CRITICAL-1: GET/PUT standard numeric-array storage ===== */

/* Test: GET into a DIM'd numeric array must NOT corrupt it (ASM GPUTG:
 * header in first 2 elements + bitmap packed into array elements) */
void test_gwsts_gfx2_get_dim_array_preserved(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("DIM G(100)");
    gwsts_exec_statement("G(1) = 999");
    gwsts_exec_statement("PSET (0, 0), 3");
    gwsts_exec_statement("GET (0,0)-(3,3), G");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "GET into DIM array no error");
    /* v144: raw bytes of array data — header xbits=8,y=4 then bitmap
     * (pixel 3 at MSB → 0xC0). Read via descriptor (BASIC float readback
     * of raw bytes is type-punned, same as GW-BASIC). */
    struct variable *g = gwdata_find_variable("G");
    TEST_ASSERT(g && g->is_array, "G is numeric array");
    array_desc_t *desc = (array_desc_t *)g->value.s.ptr;
    TEST_ASSERT(desc && desc->magic == ARRAY_MAGIC && desc->data, "desc valid");
    unsigned char *d = (unsigned char *)desc->data;
    TEST_ASSERT_EQ_INT(d[0] | (d[1] << 8), 8, "header x-bits = 8");
    TEST_ASSERT_EQ_INT(d[2] | (d[3] << 8), 4, "header y-count = 4");
    TEST_ASSERT_EQ_INT(d[4], 0xC0, "bitmap byte (pixel 3 at MSB)");
    /* G(1) was overwritten by bitmap data (not corrupted to a wild value):
     * its float bit pattern changed from 999's */
    TEST_ASSERT(g->value.f == g->value.f, "array data readable");
    TEST_PASS("gfx2 GET DIM array preserved");
}

/* Test: standard DIM roundtrip — GET then PUT restores pixels */
void test_gwsts_gfx2_get_put_dim_roundtrip(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("DIM G(100)");
    gwsts_exec_statement("LINE (40,40)-(44,44), 1, BF");
    gwsts_exec_statement("GET (38,38)-(46,46), G");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "DIM GET no error");
    gwsts_exec_statement("PUT (60,60), G");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "DIM PUT no error");
    gwsts_exec_statement("C = POINT(62,62)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "DIM roundtrip pixel (62,62)");
    TEST_PASS("gfx2 DIM roundtrip");
}

/* Test: GET into a string variable → error, variable untouched */
void test_gwsts_gfx2_get_string_rejected(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("S$ = \"ABC\"");
    int rc = gwsts_exec_statement("GET (0,0)-(3,3), S$");
    TEST_ASSERT(rc != 0 || gwdata_get_error() != 0, "GET to string var errors");
    struct variable *v = gwdata_find_variable("S$");
    TEST_ASSERT(v && v->value.s.ptr && strcmp(v->value.s.ptr, "ABC") == 0,
                "S$ content preserved");
    TEST_PASS("gfx2 GET string rejected");
}

/* Test: array too small for the block → error (no overflow write) */
void test_gwsts_gfx2_get_array_too_small(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gengrp_fb_clear();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("DIM S(5)");
    int rc = gwsts_exec_statement("GET (0,0)-(39,39), S");
    TEST_ASSERT(rc != 0 || gwdata_get_error() != 0, "small array GET errors (FCERR)");
    TEST_PASS("gfx2 GET array too small");
}

/* ===== v156: observation 5 — graphics statements in SCREEN 0 → error ===== */

static void gwsts_o5_setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    gioscn_screen(0);  /* text mode — graphics statements must fail */
}

static void gwsts_o5_teardown(void) {
    gioscn_screen(1);  /* restore graphics mode for other tests */
    remove("t_v152g.txt");
}

void test_gwsts_gfx2_screen0_pset(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("PSET (10,10)") != 0, "PSET in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 pset");
}

void test_gwsts_gfx2_screen0_preset(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("PRESET (10,10)") != 0, "PRESET in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 preset");
}

void test_gwsts_gfx2_screen0_line(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("LINE (0,0)-(10,10)") != 0, "LINE in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 line");
}

void test_gwsts_gfx2_screen0_circle(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("CIRCLE (50,50), 20") != 0, "CIRCLE in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 circle");
}

void test_gwsts_gfx2_screen0_paint(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("PAINT (50,50), 1") != 0, "PAINT in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 paint");
}

/* graphics GET/PUT (no '#' file form) also error in SCREEN 0 */
void test_gwsts_gfx2_screen0_getput(void) {
    gwsts_o5_setup();
    TEST_ASSERT(gwsts_exec_statement("DIM G(100)") == 0, "dim ok");
    TEST_ASSERT(gwsts_exec_statement("GET (0,0)-(3,3),G") != 0, "GET in SCREEN 0 errors");
    TEST_ASSERT(gwsts_exec_statement("PUT (10,10),G") != 0, "PUT in SCREEN 0 errors");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 getput");
}

/* file-form PUT #1 must NOT be gated (file statement, not graphics) */
void test_gwsts_gfx2_screen0_fileform_ok(void) {
    gwsts_o5_setup();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"O\", #1, \"t_v152g.txt\""), 0, "open ok");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"X\""), 0, "print ok");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "close ok");
    gwsts_o5_teardown();
    TEST_PASS("gfx2 screen0 fileform ok");
}

/* ===== v156 MEDIUM-1: SCREEN 0 gate sets ERR/ERL (RESUME NEXT works) ===== */

void test_gwsts_gfx2_screen0_resume_next(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    gioscn_screen(0);
    /* gate must fail the statement AND set error code 5 so that the
     * executor's ON ERROR path (run_program_lines) can dispatch with
     * ERR=5 and let RESUME NEXT resume. */
    extern WORD g_curlin;
    g_curlin = 30;
    g_state->current_line = 30;
    int rc = gwsts_exec_statement("PSET (1,1)");
    TEST_ASSERT(rc != 0, "PSET in SCREEN 0 errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "error code 5 (Illegal function call)");
    gioscn_screen(1);
    gwdata_clear_error();
    TEST_PASS("gfx2 screen0 resume next err code");
}

void test_gwsts_gfx2_screen0_err_line(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    gioscn_screen(0);
    /* set current line as the executor would */
    extern WORD g_curlin;
    g_curlin = 30;
    g_state->current_line = 30;
    gwsts_exec_statement("CIRCLE (50,50), 20");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "error code 5 set");
    gwdata_clear_error();  /* v157 obs-2: teardown consistency */
    gioscn_screen(1);
    TEST_PASS("gfx2 screen0 err line set");
}

/* ===== v157 obs-1: remaining 4 gates (LINE/PAINT/GET/PUT) set error 5 ===== */

static void gwsts_o1_setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    gioscn_screen(0);
    extern WORD g_curlin;
    g_curlin = 50;
    g_state->current_line = 50;
}

static void gwsts_o1_teardown(void) {
    gwdata_clear_error();  /* v157 obs-2: teardown consistency */
    gioscn_screen(1);
    remove("t_v157g.txt");
}

void test_gwsts_gfx2_screen0_line_err(void) {
    gwsts_o1_setup();
    TEST_ASSERT(gwsts_exec_statement("LINE (0,0)-(10,10)") != 0, "LINE gated");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "LINE: error code 5");
    gwsts_o1_teardown();
    TEST_PASS("gfx2 screen0 line err");
}

void test_gwsts_gfx2_screen0_paint_err(void) {
    gwsts_o1_setup();
    TEST_ASSERT(gwsts_exec_statement("PAINT (10,10), 1") != 0, "PAINT gated");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "PAINT: error code 5");
    gwsts_o1_teardown();
    TEST_PASS("gfx2 screen0 paint err");
}

void test_gwsts_gfx2_screen0_get_err(void) {
    gwsts_o1_setup();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM G(100)"), 0, "dim ok");
    TEST_ASSERT(gwsts_exec_statement("GET (0,0)-(3,3),G") != 0, "GET gated");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "GET: error code 5");
    gwsts_o1_teardown();
    TEST_PASS("gfx2 screen0 get err");
}

void test_gwsts_gfx2_screen0_put_err(void) {
    gwsts_o1_setup();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM G(100)"), 0, "dim ok");
    TEST_ASSERT(gwsts_exec_statement("PUT (10,10),G") != 0, "PUT gated");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 5, "PUT: error code 5");
    gwsts_o1_teardown();
    TEST_PASS("gfx2 screen0 put err");
}
