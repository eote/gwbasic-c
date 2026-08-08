/* TEST_SCNDRV - Unit tests for SCNDRV (Screen Driver - text mode) */
/* Based on SCNDRV.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* ========== Init Tests ========== */

void test_scndrv_init(void) {
    scndrv_init();
    TEST_PASS("scndrv_init");
}

void test_scndrv_init_multiple(void) {
    scndrv_init();
    scndrv_init();
    scndrv_init();
    TEST_PASS("scndrv_init multiple calls");
}

/* ========== SCNSWI Tests ========== */

void test_scndrv_swi_default(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 80, "Default width 80");
    TEST_PASS("scndrv_swi default 80x25");
}

void test_scndrv_swi_40(void) {
    scndrv_init();
    scndrv_swi(40, 25);
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 40, "Width 40");
    TEST_PASS("scndrv_swi 40x25");
}

void test_scndrv_swi_80(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 80, "Width 80");
    TEST_PASS("scndrv_swi 80x25");
}

/* ========== SCNLOC Tests ========== */

void test_scndrv_loc_basic(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 5);
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 10, "Column set");
    TEST_ASSERT_EQ_INT(row, 5, "Row set");
    TEST_PASS("scndrv_loc basic");
}

void test_scndrv_loc_origin(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(1, 1);
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "Column 1");
    TEST_ASSERT_EQ_INT(row, 1, "Row 1");
    TEST_PASS("scndrv_loc origin");
}

void test_scndrv_loc_clamp_low(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(-5, -3);
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "Column clamped to 1");
    TEST_ASSERT_EQ_INT(row, 1, "Row clamped to 1");
    TEST_PASS("scndrv_loc clamp low");
}

void test_scndrv_loc_clamp_high(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(200, 100);
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 80, "Column clamped to width");
    TEST_ASSERT_EQ_INT(row, 25, "Row clamped to height");
    TEST_PASS("scndrv_loc clamp high");
}

/* ========== SCNPOS Tests ========== */

void test_scndrv_pos_after_init(void) {
    scndrv_init();
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "Initial column 1");
    TEST_ASSERT_EQ_INT(row, 1, "Initial row 1");
    TEST_PASS("scndrv_pos after init");
}

void test_scndrv_pos_null_ptrs(void) {
    scndrv_init();
    int rc = scndrv_pos(NULL, NULL);
    TEST_ASSERT_EQ_INT(rc, 0, "Returns 0 with null ptrs");
    TEST_PASS("scndrv_pos null pointers safe");
}

/* ========== SCNCLR Tests ========== */

void test_scndrv_clr(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 10);
    scndrv_clr();
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "Column homed after clr");
    TEST_ASSERT_EQ_INT(row, 1, "Row homed after clr");
    TEST_PASS("scndrv_clr homes cursor");
}

/* ========== SCNGWI Tests ========== */

void test_scndrv_gwi_default(void) {
    scndrv_init();
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 80, "Default logical width 80");
    TEST_PASS("scndrv_gwi default");
}

void test_scndrv_gwi_after_swi(void) {
    scndrv_init();
    scndrv_swi(40, 25);
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 40, "Width 40 after swi");
    scndrv_swi(80, 25);
    TEST_ASSERT_EQ_INT(scndrv_gwi(), 80, "Width 80 after swi");
    TEST_PASS("scndrv_gwi after swi");
}

/* ========== SCNWDO Tests ========== */

void test_scndrv_wdo_valid(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    int rc = scndrv_wdo(1, 1, 40, 20);
    TEST_ASSERT_EQ_INT(rc, 0, "Valid window returns 0");
    TEST_PASS("scndrv_wdo valid");
}

void test_scndrv_wdo_out_of_bounds(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    int rc = scndrv_wdo(100, 100, 10, 10);
    TEST_ASSERT_EQ_INT(rc, -1, "Out of bounds returns -1");
    TEST_PASS("scndrv_wdo out of bounds");
}

void test_scndrv_wdo_truncate(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    /* Window extends beyond screen - should truncate */
    int rc = scndrv_wdo(70, 20, 40, 20);
    TEST_ASSERT_EQ_INT(rc, 0, "Truncated window returns 0");
    TEST_PASS("scndrv_wdo truncates to screen");
}

/* ========== SCNBRK Tests ========== */

void test_scndrv_brk(void) {
    scndrv_init();
    scndrv_brk();
    TEST_PASS("scndrv_brk safe");
}

/* ========== SCNIPL Tests ========== */

void test_scndrv_ipi(void) {
    scndrv_init();
    scndrv_ipi();
    TEST_PASS("scndrv_ipi safe");
}

/* ========== SCNMRK Tests ========== */

void test_scndrv_mrk_input(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 5);
    scndrv_mrk(true);
    TEST_PASS("scndrv_mrk input");
}

void test_scndrv_mrk_output(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 5);
    scndrv_mrk(false);
    TEST_PASS("scndrv_mrk output");
}

/* ========== SCNRDL Tests ========== */

void test_scndrv_rdl(void) {
    scndrv_init();
    char buf[256];
    int rc = scndrv_rdl(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "rdl returns 0");
    TEST_PASS("scndrv_rdl safe");
}

/* ========== SCNOUT Tests ========== */

void test_scndrv_out_printable(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_out('A');
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 2, "Cursor advances after printable char");
    TEST_PASS("scndrv_out printable char");
}

void test_scndrv_out_cr(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 5);
    scndrv_out('\r');
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "CR moves to column 1");
    TEST_PASS("scndrv_out CR");
}

void test_scndrv_out_bs(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(10, 5);
    scndrv_out('\b');
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 9, "BS moves cursor back");
    TEST_PASS("scndrv_out BS");
}

void test_scndrv_out_bs_at_origin(void) {
    scndrv_init();
    scndrv_swi(80, 25);
    scndrv_loc(1, 1);
    scndrv_out('\b');
    int col, row;
    scndrv_pos(&col, &row);
    TEST_ASSERT_EQ_INT(col, 1, "BS at origin stays at 1");
    TEST_PASS("scndrv_out BS at origin");
}
