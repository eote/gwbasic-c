/* test_gwsts_graphics.c — M2 第 2 批: PSET/PRESET/LINE/POINT/SCREEN 语句接线
 * ASM 语义 per GENGRP.ASM L149-186:
 * - PSET (X,Y)[,attr]: 默认 FORCLR; 越界静默跳过 (JAE PSTNOT)
 * - PRESET (X,Y)[,attr]: 默认 BAKCLR
 * - LINE [(x1,y1)]-(x2,y2)[,attr[,B[F]]]: 省略首坐标用当前 graphics cursor
 * - POINT(x,y): 越界返回 -1
 * 坐标/属性为表达式（ASM SCAND/FRMEVL）。
 */
#include <stdio.h>
#include <string.h>
#include "../src/gwbasic.h"
#include "test_framework.h"

/* Test: SCREEN 1 statement enters graphics mode */
void test_gwsts_gfx_screen(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    TEST_ASSERT_EQ_INT(gengrp_get_screen_mode(), 1, "SCREEN 1 sets mode 1");
    gwsts_exec_statement("SCREEN 0");
    TEST_ASSERT_EQ_INT(gengrp_get_screen_mode(), 0, "SCREEN 0 sets mode 0");
    TEST_PASS("gwsts gfx screen");
}

/* Test: PSET then POINT roundtrip */
void test_gwsts_gfx_pset_point(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("PSET (100, 100), 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "PSET no error");
    gwsts_exec_statement("C = POINT(100, 100)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "POINT no error");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "POINT(100,100) reads back 1");
    TEST_PASS("gwsts gfx pset point");
}

/* Test: PSET default color = FORCLR */
void test_gwsts_gfx_pset_default(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("PSET (50, 50)");
    gwsts_exec_statement("C = POINT(50, 50)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "PSET default = FORCLR");
    TEST_PASS("gwsts gfx pset default");
}

/* Test: PRESET default color = BAKCLR */
void test_gwsts_gfx_preset_default(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("PRESET (60, 60)");
    gwsts_exec_statement("C = POINT(60, 60)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 0, "PRESET default = BAKCLR");
    TEST_PASS("gwsts gfx preset default");
}

/* Test: PSET out of bounds — silent skip, no error (ASM JAE PSTNOT) */
void test_gwsts_gfx_pset_oob(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    int rc = gwsts_exec_statement("PSET (5000, 5000)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "PSET out-of-bounds silent");
    TEST_ASSERT_EQ_INT(rc, 0, "PSET OOB rc SUCCESS");
    TEST_PASS("gwsts gfx pset oob");
}

/* Test: LINE simple — endpoints drawn */
void test_gwsts_gfx_line_simple(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("LINE (10, 10)-(20, 20), 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "LINE no error");
    gwsts_exec_statement("C = POINT(10, 10)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE endpoint (10,10) set");
    gwsts_exec_statement("C = POINT(20, 20)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE endpoint (20,20) set");
    TEST_PASS("gwsts gfx line simple");
}

/* Test: LINE BF — filled rectangle */
void test_gwsts_gfx_line_bf(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("LINE (10, 10)-(30, 30), 1, BF");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "LINE BF no error");
    gwsts_exec_statement("C = POINT(20, 20)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE BF interior (20,20) filled");
    TEST_PASS("gwsts gfx line bf");
}

/* Test: LINE expression coordinates + variable color */
void test_gwsts_gfx_line_expr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("A = 15");
    gwsts_exec_statement("LINE (A, A)-(A + 10, A + 10), 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "LINE expr no error");
    gwsts_exec_statement("C = POINT(15, 15)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE expr endpoint (15,15) set");
    gwsts_exec_statement("C = POINT(25, 25)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE expr endpoint (25,25) set");
    TEST_PASS("gwsts gfx line expr");
}

/* Test: LINE with omitted first coordinate — uses graphics cursor
 * (PSET (5,5) moves cursor; LINE -(9,9) draws from (5,5)) */
void test_gwsts_gfx_line_default_start(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("PSET (5, 5), 1");
    gwsts_exec_statement("LINE -(9, 9), 1");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "LINE omitted-start no error");
    gwsts_exec_statement("C = POINT(9, 9)");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE omitted-start endpoint (9,9)");
    gwsts_exec_statement("C = POINT(7, 7)");
    c = gwdata_find_variable("C");
    TEST_ASSERT_EQ_INT((int)c->value.f, 1, "LINE from-cursor path (7,7) on line");
    TEST_PASS("gwsts gfx line default start");
}

/* Test: POINT out of bounds returns -1 */
void test_gwsts_gfx_point_oob(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    gengrp_init();
    gwsts_exec_statement("SCREEN 1");
    gwsts_exec_statement("C = POINT(5000, 5000)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "POINT OOB no error");
    struct variable *c = gwdata_find_variable("C");
    TEST_ASSERT(c, "C allocated");
    TEST_ASSERT_EQ_INT((int)c->value.f, -1, "POINT OOB returns -1");
    TEST_PASS("gwsts gfx point oob");
}
