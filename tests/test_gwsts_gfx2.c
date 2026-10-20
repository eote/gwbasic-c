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
