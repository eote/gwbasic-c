/* TEST_GWSTS_RENUM - M1: RENUM/RESEQ 语句分支测试 (MEDIUM-1, v118) */
/* Based on GWMAIN.ASM RESEQ (L3102-3222) — 语句级 RESEQ [NN[,MM[,INC]]] */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: RENUM 语句默认 — RENUM = RESEQ 10,10 (ASM L3105-3111) */
void test_gwsts_renum_default(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");
    gwlist_add_line(15, "B = 2");
    
    int rc = gwsts_exec_statement("RENUM");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RENUM default succeeds");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 5 -> 10");
    TEST_ASSERT(gwlist_find_line(20) != NULL, "line 15 -> 20");
    TEST_PASS("gwsts renum default");
}

/* Test: RENUM 指定起点 — RENUM 100 (ASM L3113-3120: LINSPC 新 NN) */
void test_gwsts_renum_start(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");
    
    int rc = gwsts_exec_statement("RENUM 100");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RENUM 100 succeeds");
    TEST_ASSERT(gwlist_find_line(100) != NULL, "line 5 -> 100");
    TEST_PASS("gwsts renum start");
}

/* Test: RENUM 起点+阈值 — RENUM 100,20 (MM 阈值, ASM L3107) */
void test_gwsts_renum_start_mm(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");    /* < 20: 不重排 */
    gwlist_add_line(25, "B = 2");   /* >= 20: 重排 */
    
    int rc = gwsts_exec_statement("RENUM 100,20");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RENUM 100,20 succeeds");
    TEST_ASSERT(gwlist_find_line(5) != NULL, "line 5 preserved (< MM)");
    TEST_ASSERT(gwlist_find_line(100) != NULL, "line 25 -> 100");
    TEST_PASS("gwsts renum start/mm");
}

/* Test: RENUM 起点+阈值+增量 — RENUM 100,20,5 */
void test_gwsts_renum_full_args(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(20, "A = 1");
    gwlist_add_line(30, "B = 2");
    
    int rc = gwsts_exec_statement("RENUM 100,20,5");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RENUM 100,20,5 succeeds");
    TEST_ASSERT(gwlist_find_line(100) != NULL, "line 20 -> 100");
    TEST_ASSERT(gwlist_find_line(105) != NULL, "line 30 -> 105");
    TEST_PASS("gwsts renum full args");
}

/* Test: RENUM INC=0 非法 (ASM L3140-3142: JMP FCERR) */
void test_gwsts_renum_inc_zero(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(20, "A = 1");
    
    int rc = gwsts_exec_statement("RENUM 100,20,0");
    
    TEST_ASSERT(rc != 0, "RENUM 100,20,0 → error (ASM FCERR)");
    TEST_PASS("gwsts renum inc=0");
}

/* Test: RENUM 后 GOTO 引用行号不自动更新 (ASM SCCPTR 转换是单独 pass,
 * 简化实现: 行号内容保持原样) */
void test_gwsts_renum_content_kept(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "PRINT \"X\"");
    
    int rc = gwsts_exec_statement("RENUM 50");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RENUM 50 succeeds");
    struct program_line *l = gwlist_find_line(50);
    TEST_ASSERT(l != NULL && l->text && strcmp(l->text, "PRINT \"X\"") == 0, "content kept");
    TEST_PASS("gwsts renum content kept");
}

/* Test: RANDOMIZE 语句回归 (v116 已实现，防退化) */
void test_gwsts_randomize_regress(void) {
    gwdata_init();
    gwram_init();
    
    int rc = gwsts_exec_statement("RANDOMIZE 42");
    
    TEST_ASSERT_EQ_INT(rc, 0, "RANDOMIZE 42 succeeds (v116 regression)");
    TEST_PASS("gwsts randomize regress");
}