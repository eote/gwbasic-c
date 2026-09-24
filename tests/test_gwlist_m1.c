/* TEST_GWLIST_M1 - M1: GWLIST 行管理深化 — RENUM/RESEQ 行号重排, DELETE range */
/* Based on GWMAIN.ASM RESEQ (L3102-3222) + SCCLIN/SCCPTR (L3222-3325) */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: RESEQ 默认 — RESEQ = 10,10 (ASM L3105-3111: CX=10 INC, DE=0 all lines) */
void test_gwlist_reseq_default(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");
    gwlist_add_line(15, "B = 2");
    gwlist_add_line(25, "C = 3");
    
    int rc = gwlist_reseq(0, 0, -1);  /* 0,0,-1 = defaults: start=10, mm=all, inc=10 (absent INC) */
    
    TEST_ASSERT_EQ_INT(rc, 0, "RESEQ default succeeds");
    /* 5→10, 15→20, 25→30 */
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 5 -> 10");
    TEST_ASSERT(gwlist_find_line(20) != NULL, "line 15 -> 20");
    TEST_ASSERT(gwlist_find_line(30) != NULL, "line 25 -> 30");
    TEST_ASSERT(gwlist_find_line(5) == NULL, "old line 5 gone");
    TEST_PASS("gwlist reseq default 10,10");
}

/* Test: RESEQ 指定起点与增量 — RESEQ 100,1 (ASM L3113-3120: LINSPC 新 NN) */
void test_gwlist_reseq_start_inc(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");
    gwlist_add_line(15, "B = 2");
    
    int rc = gwlist_reseq(100, 0, 1);  /* start=100, inc=1 */
    
    TEST_ASSERT_EQ_INT(rc, 0, "RESEQ 100,1 succeeds");
    TEST_ASSERT(gwlist_find_line(100) != NULL, "line 5 -> 100");
    TEST_ASSERT(gwlist_find_line(101) != NULL, "line 15 -> 101");
    TEST_PASS("gwlist reseq start/inc");
}

/* Test: RESEQ MM 参数 — 小于 MM 的行不重排 (ASM L3107: LINES LESS THAN MM NOT RESEQUENCED) */
void test_gwlist_reseq_mm_threshold(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(5, "A = 1");    /* < 20: 不重排 */
    gwlist_add_line(25, "B = 2");   /* >= 20: 重排 */
    
    int rc = gwlist_reseq(100, 20, 10);  /* mm=20 */
    
    TEST_ASSERT_EQ_INT(rc, 0, "RESEQ with MM succeeds");
    TEST_ASSERT(gwlist_find_line(5) != NULL, "line 5 preserved (< MM)");
    TEST_ASSERT(gwlist_find_line(100) != NULL, "line 25 -> 100 (>= MM)");
    TEST_PASS("gwlist reseq MM threshold");
}

/* Test: RESEQ INC=0 非法 (ASM L3140-3142: OR DX,DX / JZ / JMP FCERR) */
void test_gwlist_reseq_inc_zero_error(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "A = 1");
    
    int rc = gwlist_reseq(100, 0, -1);  /* INC absent — default 10 */
    /* ASM: RESEQ 100,,0 → explicit INC=0 → FCERR */
    int rc2 = gwlist_reseq(100, 5, 0);  /* inc=0 explicit */
    
    TEST_ASSERT(rc == 0, "RESEQ 100 with absent INC uses default 10");
    TEST_ASSERT(rc2 != 0, "RESEQ 100,5,0 → FCERR (explicit inc=0 illegal)");
    TEST_PASS("gwlist reseq inc=0 error");
}

/* Test: RESEQ 行号过大 (ASM L3176-3181: MOV BX,65529 / CMP / JAE / JMP FCERR) */
void test_gwlist_reseq_line_too_large(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");
    
    /* 增量太大导致重排后超 65529 → FCERR */
    int rc = gwlist_reseq(65520, 0, 10);  /* 65520, 65530>65529 → error */
    
    TEST_ASSERT(rc != 0, "RESEQ overflow beyond 65529 → FCERR (ASM L3176)");
    TEST_PASS("gwlist reseq line too large");
}

/* Test: RESEQ 空程序 */
void test_gwlist_reseq_empty_program(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    int rc = gwlist_reseq(0, 0, -1);
    
    TEST_ASSERT_EQ_INT(rc, 0, "RESEQ on empty program succeeds (no-op)");
    TEST_PASS("gwlist reseq empty");
}

/* Test: RESEQ 后程序内容保持 */
void test_gwlist_reseq_content_preserved(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "PRINT \"HELLO\"");
    gwlist_add_line(20, "GOTO 10");
    
    int rc = gwlist_reseq(0, 0, -1);  /* 10→10? no: start=10, 10→10, 20→20 (same!) */
    /* 用不同起点验证内容移动 */
    int rc2 = gwlist_reseq(100, 0, 50);  /* 10→100, 20→150 */
    
    (void)rc;
    TEST_ASSERT_EQ_INT(rc2, 0, "RESEQ 100,50 succeeds");
    struct program_line *l100 = gwlist_find_line(100);
    struct program_line *l150 = gwlist_find_line(150);
    TEST_ASSERT(l100 != NULL && l100->text && strcmp(l100->text, "PRINT \"HELLO\"") == 0, "content preserved at 100");
    TEST_ASSERT(l150 != NULL && l150->text && strcmp(l150->text, "GOTO 10") == 0, "content preserved at 150");
    TEST_PASS("gwlist reseq content preserved");
}

/* Test: DELETE range (gwmain/gwlist 扩展: 删除范围内的行) */
void test_gwlist_delete_range(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");
    gwlist_add_line(40, "D = 4");
    
    int rc = gwlist_delete_range(20, 30);
    
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 20-30 succeeds");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 preserved");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 deleted");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "line 30 deleted");
    TEST_ASSERT(gwlist_find_line(40) != NULL, "line 40 preserved");
    TEST_PASS("gwlist delete range");
}

/* Test: DELETE range 无效范围 (start > end) */
void test_gwlist_delete_range_invalid(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "A = 1");
    
    int rc = gwlist_delete_range(30, 20);  /* start > end */
    
    TEST_ASSERT(rc != 0, "DELETE 30-20 invalid (start > end)");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 preserved");
    TEST_PASS("gwlist delete range invalid");
}

/* Test: DELETE range 空范围 (范围内无行) */
void test_gwlist_delete_range_empty(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(10, "A = 1");
    
    int rc = gwlist_delete_range(100, 200);  /* 无行在范围内 */
    
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE empty range succeeds (no-op)");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 preserved");
    TEST_PASS("gwlist delete range empty");
}