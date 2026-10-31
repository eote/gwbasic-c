/*
 * test_biprtu_out.c — v160 MEDIUM-3: output-asserting USING tests.
 * Captures stdout via _dup/_dup2 to a temp file (no freopen — keeps
 * the test framework stream alive).
 */
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "test_framework.h"
#include "gwbasic.h"

extern int gwsts_exec_statement(const char *text);

static char outbuf_capture[8192];

static int capture_statement(const char *stmt) {
    fflush(stdout);
    int saved_fd = _dup(_fileno(stdout));
    int cap_fd = _open("t_cap.txt", _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                       _S_IREAD | _S_IWRITE);
    if (cap_fd < 0) { outbuf_capture[0] = '\0'; return gwsts_exec_statement(stmt); }
    _dup2(cap_fd, _fileno(stdout));
    int rc = gwsts_exec_statement(stmt);
    fflush(stdout);
    _close(cap_fd);
    /* restore */
    _dup2(saved_fd, _fileno(stdout));
    _close(saved_fd);
    /* read back */
    FILE *f = fopen("t_cap.txt", "rb");
    if (f) {
        size_t rd = fread(outbuf_capture, 1, sizeof(outbuf_capture) - 1, f);
        outbuf_capture[rd] = '\0';
        fclose(f);
    } else outbuf_capture[0] = '\0';
    _unlink("t_cap.txt");
    return rc;
}

void test_biprtu_out_scientific(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"##.##^^^^\"; 123.45");
    TEST_ASSERT(strstr(outbuf_capture, "1.23E+02") != NULL,
                "scientific output is 1.23E+02");
    TEST_PASS("biprtu out scientific");
}

void test_biprtu_out_dollar(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"$$###.##\"; 12.34");
    TEST_ASSERT(strstr(outbuf_capture, "$12.34") != NULL,
                "dollar output is $12.34");
    TEST_PASS("biprtu out dollar");
}

void test_biprtu_out_star(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"**###\"; 12");
    TEST_ASSERT(strstr(outbuf_capture, "***12") != NULL,
                "star output is ***12");
    TEST_PASS("biprtu out star");
}

void test_biprtu_out_overflow_percent(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"###.##\"; -1234.56");
    TEST_ASSERT(strstr(outbuf_capture, "%-1234.56") != NULL,
                "overflow output is %-1234.56");
    TEST_PASS("biprtu out overflow");
}

void test_biprtu_out_sign_fields(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    capture_statement("PRINT USING \"-##\"; -5");
    TEST_ASSERT(strstr(outbuf_capture, "5-") != NULL,
                "trailing sign output contains '5-'");
    TEST_PASS("biprtu out sign fields");
}

void test_biprtu_swap_string_array(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("DIM A$(2)"), 0, "DIM A$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$(1) = \"one\""), 0, "set 1");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$(2) = \"two\""), 0, "set 2");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("SWAP A$(1), A$(2)"), 0, "swap");
    capture_statement("PRINT A$(1); A$(2)");
    TEST_ASSERT(strstr(outbuf_capture, "twoone") != NULL,
                "string array swapped: two one");
    TEST_PASS("biprtu swap string array");
}

void test_biprtu_swap_mixed_type_err13(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    gwsts_exec_statement("A$ = \"X\"");
    gwsts_exec_statement("B = 1");
    TEST_ASSERT(gwsts_exec_statement("SWAP A$, B") != 0,
                "mixed SWAP errors");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 13, "TMERR = 13");
    gwdata_clear_error();
    TEST_PASS("biprtu swap mixed type");
}

void test_biprtu_out_inf_overflow(void) {
    gwdata_init(); gwram_init(); gwinit_init(); gioscn_screen(1);
    gwdata_clear_error();
    int rc = gwsts_exec_statement("PRINT 1E+39");
    TEST_ASSERT(rc != 0, "1E+39 overflows");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 6, "Overflow = 6");
    gwdata_clear_error();
    TEST_PASS("biprtu out inf overflow");
}
