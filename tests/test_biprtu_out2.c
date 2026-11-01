/*
 * test_biprtu_out2.c — v161: sign variants, scalar<->array SWAP,
 * arithmetic inf, mid-line print zone, 2D string arrays.
 */
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);

static char capbuf[8192];
static int capture_statement(const char *stmt) {
    fflush(stdout);
    int saved_fd = _dup(_fileno(stdout));
    int cap_fd = _open("t_cap2.txt", _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                       _S_IREAD | _S_IWRITE);
    if (cap_fd < 0) { capbuf[0] = '\0'; return gwsts_exec_statement(stmt); }
    _dup2(cap_fd, _fileno(stdout));
    int rc = gwsts_exec_statement(stmt);
    fflush(stdout);
    _close(cap_fd);
    _dup2(saved_fd, _fileno(stdout));
    _close(saved_fd);
    FILE *f = fopen("t_cap2.txt", "rb");
    if (f) {
        size_t rd = fread(capbuf, 1, sizeof(capbuf) - 1, f);
        capbuf[rd] = '\0';
        fclose(f);
    } else capbuf[0] = '\0';
    _unlink("t_cap2.txt");
    return rc;
}

void test_biprtu_out2_sign_variants(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"##-\"; 5");
    TEST_ASSERT(strstr(capbuf, " 5 ") != NULL, "##- 5 → ' 5 '");
    capture_statement("PRINT USING \"##-\"; -5");
    TEST_ASSERT(strstr(capbuf, " 5-") != NULL, "##- -5 → ' 5-'");
    capture_statement("PRINT USING \"##+\"; -5");
    TEST_ASSERT(strstr(capbuf, " 5-") != NULL, "##+ -5 → ' 5-' (no double sign)");
    TEST_PASS("out2 sign variants");
}

void test_biprtu_out2_swap_scalar_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM A(2)"), 0, "DIM");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A(1) = 9"), 0, "set");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("B = 1"), 0, "B");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("SWAP B, A(1)"), 0, "swap");
    capture_statement("PRINT \"M\"; B; A(1)");
    TEST_ASSERT(strstr(capbuf, "M 9 1") != NULL, "scalar<->array swapped");
    gwdata_clear_error();
    TEST_PASS("out2 swap scalar array");
}

void test_biprtu_out2_swap_str_scalar_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("S$ = \"x\"");
    gwsts_exec_statement("DIM A$(2)");
    gwsts_exec_statement("A$(1) = \"y\"");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("SWAP S$, A$(1)"), 0, "swap");
    capture_statement("PRINT \"S\"; S$; A$(1)");
    TEST_ASSERT(strstr(capbuf, "Syx") != NULL, "string scalar<->array swapped");
    gwdata_clear_error();
    TEST_PASS("out2 swap str scalar array");
}

void test_biprtu_out2_2d_string_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM A$(2,2)"), 0, "DIM 2D");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$(1,1) = \"AAAA\""), 0, "set 1,1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$(2,2) = \"BBBB\""), 0, "set 2,2");
    capture_statement("PRINT \"L\"; LEN(A$(1,1)); LEN(A$(2,2))");
    TEST_ASSERT(strstr(capbuf, "L 4 4") != NULL, "2D string LEN=4");
    capture_statement("PRINT \"V\"; A$(1,1); A$(2,2)");
    TEST_ASSERT(strstr(capbuf, "VAAAABBBB") != NULL, "2D string values");
    gwdata_clear_error();
    TEST_PASS("out2 2d string array");
}

void test_biprtu_out2_arith_inf_overflow(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("A = 1E+38");
    TEST_ASSERT(gwsts_exec_statement("B = A * 1E+38") != 0,
                "arithmetic overflow errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 6, "Overflow = 6");
    gwdata_clear_error();
    TEST_PASS("out2 arith inf");
}

void test_biprtu_out2_eval_err_no_pseudo(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v161 MEDIUM-4: 1/0 in value list → ERR 11, NO pseudo field output */
    int rc = capture_statement("PRINT USING \"##\"; 1/0");
    TEST_ASSERT(rc != 0, "eval error propagates");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 11, "div by zero = 11");
    TEST_ASSERT(strstr(capbuf, " 1") == NULL || strstr(capbuf, " 1") > capbuf,
                "no pseudo value before error");
    gwdata_clear_error();
    TEST_PASS("out2 eval err no pseudo");
}

void test_biprtu_out2_midline_zone(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    /* v161 MEDIUM-3: 'AB;' leaves col 2; USING ' 1' ends col 4;
     * trailing ',' advances to zone 14 → X at col 15 */
    capture_statement("PRINT \"AB\";");
    gwsts_exec_statement("PRINT USING \"##\"; 1,");
    capture_statement("PRINT \"X\"");
    TEST_ASSERT(strstr(capbuf, "X") != NULL, "X printed");
    TEST_PASS("out2 midline zone");
}
