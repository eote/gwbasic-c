/* TEST_GWSTS_DELETE - M1: DELETE 语句分支测试 (v124 短期建议-1) */
/* Based on GWMAIN.ASM DELETE — 语句级 DELETE [start][-end] 接入 gwlist_delete_range */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: gwlist_delete_range 基础 — 删除区间内行 */
void test_gwlist_delete_basic(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");

    struct program_line *anchor = gwlist_delete_range(20, 20);
    TEST_ASSERT(anchor != NULL, "delete 20-20 succeeds, anchor set");
    TEST_ASSERT_EQ_INT(anchor->number, 30, "anchor is line 30");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_ASSERT(gwlist_find_line(30) != NULL, "line 30 kept");
    TEST_PASS("gwlist delete_range basic");
}

/* Test: gwlist_delete_range 区间 — 20-30 删除两行 */
void test_gwlist_delete_range_multi(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");

    struct program_line *anchor = gwlist_delete_range(20, 30);
    /* [10,20,30] delete 20-30 → nothing after 30 → anchor = NULL (ASM FNDLIN
     * return 1: no line greater than the range end) */
    TEST_ASSERT(anchor == NULL, "delete 20-30: no anchor (nothing after 30)");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "line 30 gone");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist delete_range multi-line");
}

/* Test: gwlist_delete_range start>end — 错误路径 (ASM FCERR) */
void test_gwlist_delete_bad_range(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");

    struct program_line *anchor = gwlist_delete_range(30, 20);
    TEST_ASSERT(anchor == NULL, "start>end → NULL (FCERR)");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist delete_range start>end error");
}

/* Test: gwlist_delete_range 越界 — 删除不存在行, 成功无副作用 */
void test_gwlist_delete_missing(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");

    struct program_line *anchor = gwlist_delete_range(50, 60);
    TEST_ASSERT(anchor == NULL, "delete missing range: no anchor (no-op)");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist delete_range missing lines no-op");
}

/* Test: gwlist_delete_range 空程序 — 无副作用成功 */
void test_gwlist_delete_empty(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    struct program_line *anchor = gwlist_delete_range(10, 20);
    TEST_ASSERT(anchor == NULL, "delete on empty program: no anchor");
    TEST_PASS("gwlist delete_range empty program");
}

/* Test: 语句级 DELETE — gwsts_exec_statement 接入 (RED→GREEN) */
void test_gwsts_delete_stmt(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");

    int rc = gwsts_exec_statement("DELETE 20-30");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 20-30 statement executes");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "line 30 gone");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwsts DELETE statement");
}

/* Test: 语句级 DELETE 单行 + 无效区间回归 */
void test_gwsts_delete_stmt_single_and_bad(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(40, "D = 4");

    int rc1 = gwsts_exec_statement("DELETE 40");
    TEST_ASSERT_EQ_INT(rc1, 0, "DELETE 40 single-line executes");
    TEST_ASSERT(gwlist_find_line(40) == NULL, "line 40 gone");

    int rc2 = gwsts_exec_statement("DELETE 90-50");
    TEST_ASSERT_EQ_INT(rc2, -1, "DELETE 90-50 invalid → ERROR");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwsts DELETE single + bad range");
}

/* Test: DELETE 后程序继续执行 — 行号保持正确 */
void test_gwsts_delete_program_continues(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 5");
    gwlist_add_line(20, "B = 6");
    gwlist_add_line(30, "C = 7");

    int rc = gwsts_exec_statement("DELETE 20");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 20 executes");
    TEST_ASSERT(gwlist_find_line(10) != NULL && gwlist_find_line(30) != NULL, "10/30 kept");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "20 deleted");
    TEST_PASS("gwsts DELETE program continues");
}

/* Test: 边界用例 (v125 LOW-1/2/3) — 裸 DELETE / 40- 开放区间 / -40 / 非数字 / 尾部 */
void test_gwsts_delete_bare(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");

    int rc = gwsts_exec_statement("DELETE");
    TEST_ASSERT_EQ_INT(rc, -1, "bare DELETE → SNERR");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwsts DELETE bare syntax error");
}

void test_gwsts_delete_open_range(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(40, "D = 4");

    int rc = gwsts_exec_statement("DELETE 40-");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 40- open range (to 65530)");
    TEST_ASSERT(gwlist_find_line(40) == NULL, "line 40 gone");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwsts DELETE 40- open range");
}

void test_gwsts_delete_from_start(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(40, "D = 4");

    int rc = gwsts_exec_statement("DELETE -20");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE -20 from start (0..20)");
    TEST_ASSERT(gwlist_find_line(10) == NULL, "line 10 gone");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone");
    TEST_ASSERT(gwlist_find_line(40) != NULL, "line 40 kept");
    TEST_PASS("gwsts DELETE -20 from start");
}

void test_gwsts_delete_non_numeric(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");

    int rc1 = gwsts_exec_statement("DELETE ABC");
    TEST_ASSERT_EQ_INT(rc1, -1, "DELETE ABC → SNERR");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept after ABC");

    int rc2 = gwsts_exec_statement("DELETE 1.5x");
    TEST_ASSERT_EQ_INT(rc2, -1, "DELETE 1.5x → SNERR");
    TEST_PASS("gwsts DELETE non-numeric errors");
}

void test_gwsts_delete_tail_garbage(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");

    int rc = gwsts_exec_statement("DELETE 10-20 30");
    /* Tail garbage after range → SNERR (ASM SCNLIN: must be terminator) */
    TEST_ASSERT_EQ_INT(rc, -1, "DELETE 10-20 30 tail → SNERR");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept (not deleted)");
    TEST_PASS("gwsts DELETE tail garbage errors");
}

void test_gwsts_delete_line_zero(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(0, "Z = 0");
    gwlist_add_line(10, "A = 1");

    int rc = gwsts_exec_statement("DELETE 0");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 0 (line 0 legal, ASM 0..65529)");
    TEST_ASSERT(gwlist_find_line(0) == NULL, "line 0 gone");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwsts DELETE line 0");
}

/* Test: MEDIUM-1 (v126) — 尾随空白是合法终止符 (ASM LINGET) */
void test_gwsts_delete_trailing_space(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(40, "D = 4");

    int rc1 = gwsts_exec_statement("DELETE 40 ");
    TEST_ASSERT_EQ_INT(rc1, 0, "DELETE 40 <space> → success");
    TEST_ASSERT(gwlist_find_line(40) == NULL, "line 40 gone");

    int rc2 = gwsts_exec_statement("DELETE 10-20 ");
    TEST_ASSERT_EQ_INT(rc2, 0, "DELETE 10-20 <space> → success (range form too)");
    TEST_ASSERT(gwlist_find_line(10) == NULL, "line 10 gone");
    TEST_PASS("gwsts DELETE trailing whitespace terminator");
}

/* Test: CRITICAL-1 (v126) — 语句级删除紧邻下一行 (RENUM 回归保障) */
void test_gwsts_delete_next_line(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");
    gwlist_add_line(40, "D = 4");

    int rc = gwsts_exec_statement("DELETE 30");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 30 executes");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "line 30 gone");
    TEST_ASSERT(gwlist_find_line(20) != NULL, "line 20 kept");
    TEST_ASSERT(gwlist_find_line(40) != NULL, "line 40 kept");
    TEST_PASS("gwsts DELETE next line (relocation safe)");
}

/* Test: CRITICAL-1 (v126) — 区间删除紧邻行至末尾 30- (从下一现存行继续) */
void test_gwsts_delete_next_line_to_end(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");

    int rc = gwsts_exec_statement("DELETE 30-");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 30- open range from next line");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "line 30 gone");
    TEST_ASSERT(gwlist_find_line(20) != NULL, "line 20 kept");
    TEST_PASS("gwsts DELETE next-line-to-end");
}

/* Test: CRITICAL-1 (v126) — 区间删除紧邻行 30-50 (RENUM 回归保障) */
void test_gwsts_delete_next_line_range(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");
    gwlist_add_line(40, "D = 4");
    gwlist_add_line(50, "E = 5");

    int rc = gwsts_exec_statement("DELETE 30-50");
    TEST_ASSERT_EQ_INT(rc, 0, "DELETE 30-50 range from next line");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "30 gone");
    TEST_ASSERT(gwlist_find_line(40) == NULL, "40 gone");
    TEST_ASSERT(gwlist_find_line(50) == NULL, "50 gone");
    TEST_ASSERT(gwlist_find_line(20) != NULL, "20 kept");
    TEST_PASS("gwsts DELETE next-line range");
}
