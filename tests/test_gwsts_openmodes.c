/* test_gwsts_openmodes.c — v149 MEDIUM-1: classic mode-first OPEN syntax
 * (GW-BASIC: OPEN "O",#1,"file" / "I" / "A" / "R"; ASM GIO86.ASM L59-101).
 * Regression for the silent mis-parse that opened a file literally named O. */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "../src/gwbasic.h"

extern int gwsts_exec_statement(const char *text);
extern void gwdata_clear_error(void);

static void cleanup_files(void) {
    remove("t_v149o.txt");
    remove("t_v149i.txt");
    remove("t_v149a.txt");
    remove("t_v149r.dta");
    remove("t_v149b.dta");
}

/* OPEN "O",#1,"file" → sequential output, PRINT #1 writes, CLOSE persists */
void test_gwsts_openmodes_output(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"O\", #1, \"t_v149o.txt\""), 0, "mode-first O opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"HELLO-V149\""), 0, "PRINT #1 works");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "CLOSE");
    /* read back with C stdio */
    FILE *fp = fopen("t_v149o.txt", "r");
    TEST_ASSERT(fp != NULL, "file exists after CLOSE");
    if (fp) {
        char buf[64] = {0};
        fgets(buf, sizeof(buf), fp);
        fclose(fp);
        TEST_ASSERT(strstr(buf, "HELLO-V149") != NULL, "content written");
    }
    cleanup_files();
    TEST_PASS("openmodes output");
}

/* OPEN "I",#1,"file" → sequential input, INPUT #1 reads */
void test_gwsts_openmodes_input(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    /* create file with new-style syntax first */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"t_v149i.txt\" FOR OUTPUT AS #2"), 0, "new-style OUTPUT");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #2, \"LINE-1\""), 0, "write line");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "close write");
    /* classic mode-first INPUT */
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"I\", #1, \"t_v149i.txt\""), 0, "mode-first I opens");
    char buf[64] = {0};
    extern int giodsk_input(int filenum, char *buffer, int maxlen);
    int rc = giodsk_input(1, buf, sizeof(buf) - 1);
    TEST_ASSERT(rc >= 0, "INPUT #1 reads");
    TEST_ASSERT(strstr(buf, "LINE-1") != NULL, "content read back");
    gwsts_exec_statement("CLOSE");
    cleanup_files();
    TEST_PASS("openmodes input");
}

/* OPEN "A",#1,"file" → append mode (ASM MD_APP) */
void test_gwsts_openmodes_append(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"t_v149a.txt\" FOR OUTPUT AS #1"), 0, "seed file");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"ONE\""), 0, "seed write");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "seed close");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"A\", #1, \"t_v149a.txt\""), 0, "mode-first A opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"TWO\""), 0, "append write");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "close");
    FILE *fp = fopen("t_v149a.txt", "r");
    TEST_ASSERT(fp != NULL, "append file exists");
    if (fp) {
        char all[256] = {0};
        size_t got = fread(all, 1, sizeof(all) - 1, fp);
        (void)got;
        fclose(fp);
        TEST_ASSERT(strstr(all, "ONE") != NULL && strstr(all, "TWO") != NULL,
                    "append preserved old + new");
    }
    cleanup_files();
    TEST_PASS("openmodes append");
}

/* OPEN "R",#1,"file",reclen → random mode with record length */
void test_gwsts_openmodes_random(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"R\", #1, \"t_v149r.dta\", 64"), 0, "mode-first R opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "close");
    cleanup_files();
    TEST_PASS("openmodes random");
}

/* invalid mode letter → error (ASM ERBFM0 "Bad file mode"), no file created */
void test_gwsts_openmodes_bad_mode(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    int rc = gwsts_exec_statement("OPEN \"X\", #1, \"t_v149b.dta\"");
    TEST_ASSERT(rc != 0 || gwdata_get_error() != 0, "bad mode errors (BFERR)");
    gwdata_clear_error();
    gwsts_exec_statement("CLOSE");
    FILE *fp = fopen("t_v149b.dta", "r");
    TEST_ASSERT(fp == NULL, "no file created on bad mode");
    if (fp) fclose(fp);
    cleanup_files();
    TEST_PASS("openmodes bad mode");
}

/* regression: lowercase mode-first "o" also works (ASM forces upper case) */
void test_gwsts_openmodes_lowercase(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"o\", #1, \"t_v149o.txt\""), 0, "lowercase o opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "close");
    cleanup_files();
    TEST_PASS("openmodes lowercase");
}

/* ===== v152: observation 2/3 — expression filespec + reclen semantics ===== */

/* classic mode-first with VARIABLE filespec (ASM NAMSCN uses FRMEVL) */
void test_gwsts_openmodes_var_filespec_classic(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("F$ = \"t_v149o.txt\""), 0, "set F$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"O\", #1, F$"), 0, "classic var filespec opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"VAR-OK\""), 0, "PRINT #1 works");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "CLOSE");
    FILE *fp = fopen("t_v149o.txt", "r");
    TEST_ASSERT(fp != NULL, "var-name file created");
    if (fp) {
        char buf[64] = {0};
        fgets(buf, sizeof(buf), fp);
        fclose(fp);
        TEST_ASSERT(strstr(buf, "VAR-OK") != NULL, "content written via var filespec");
    }
    cleanup_files();
    TEST_PASS("openmodes var filespec classic");
}

/* new-style with variable filespec */
void test_gwsts_openmodes_var_filespec_newstyle(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("N$ = \"t_v149i.txt\""), 0, "set N$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN N$ FOR OUTPUT AS #1"), 0, "new-style var filespec opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("PRINT #1, \"NS-OK\""), 0, "PRINT #1 works");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "CLOSE");
    FILE *fp = fopen("t_v149i.txt", "r");
    TEST_ASSERT(fp != NULL, "var file created");
    if (fp) {
        char buf[64] = {0};
        fgets(buf, sizeof(buf), fp);
        fclose(fp);
        TEST_ASSERT(strstr(buf, "NS-OK") != NULL, "content written");
    }
    cleanup_files();
    TEST_PASS("openmodes var filespec newstyle");
}

/* concatenation filespec */
void test_gwsts_openmodes_concat_filespec(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("A$ = \"t_v149a\""), 0, "set A$");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("OPEN \"O\", #1, A$+\".txt\""), 0, "concat filespec opens");
    TEST_ASSERT_EQ_INT(gwsts_exec_statement("CLOSE"), 0, "CLOSE");
    FILE *fp = fopen("t_v149a.txt", "r");
    TEST_ASSERT(fp != NULL, "concat file created");
    if (fp) fclose(fp);
    cleanup_files();
    TEST_PASS("openmodes concat filespec");
}

/* classic mode O + reclen → SNERR (ASM PRLENC: reclen only for RANDOM),
 * and NO file must be created */
void test_gwsts_openmodes_modeO_reclen_snerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    int rc = gwsts_exec_statement("OPEN \"O\", #1, \"t_v149b.dta\", 128");
    TEST_ASSERT(rc != 0, "mode O + reclen errors (SNERR)");
    gwsts_exec_statement("CLOSE");
    FILE *fp = fopen("t_v149b.dta", "r");
    TEST_ASSERT(fp == NULL, "no file created");
    if (fp) fclose(fp);
    cleanup_files();
    TEST_PASS("openmodes modeO reclen snerr");
}

/* new-style OUTPUT + LEN= → SNERR (ASM PNRLEN: LEN only for RANDOM) */
void test_gwsts_openmodes_output_len_snerr(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    int rc = gwsts_exec_statement("OPEN \"t_v149b.dta\" FOR OUTPUT AS #1 LEN=128");
    TEST_ASSERT(rc != 0, "OUTPUT + LEN= errors (SNERR)");
    gwsts_exec_statement("CLOSE");
    FILE *fp = fopen("t_v149b.dta", "r");
    TEST_ASSERT(fp == NULL, "no file created");
    if (fp) fclose(fp);
    cleanup_files();
    TEST_PASS("openmodes output len snerr");
}

/* reclen = 0 → FCERR (ASM VARECS: OR CX,CX; JZ FCERR1) */
void test_gwsts_openmodes_reclen_zero(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    giodsk_init();
    gwdata_clear_error();
    cleanup_files();
    int rc = gwsts_exec_statement("OPEN \"R\", #1, \"t_v149r.dta\", 0");
    TEST_ASSERT(rc != 0, "reclen 0 errors (FCERR)");
    gwsts_exec_statement("CLOSE");
    cleanup_files();
    TEST_PASS("openmodes reclen zero");
}
