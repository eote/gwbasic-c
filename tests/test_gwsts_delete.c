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

    int rc = gwlist_delete_range(20, 20);
    TEST_ASSERT_EQ_INT(rc, 0, "delete 20-20 succeeds");
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

    int rc = gwlist_delete_range(20, 30);
    TEST_ASSERT_EQ_INT(rc, 0, "delete 20-30 succeeds");
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

    int rc = gwlist_delete_range(30, 20);
    TEST_ASSERT_EQ_INT(rc, -1, "start>end → ERROR");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist delete_range start>end error");
}

/* Test: gwlist_delete_range 越界 — 删除不存在行, 成功无副作用 */
void test_gwlist_delete_missing(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");

    int rc = gwlist_delete_range(50, 60);
    TEST_ASSERT_EQ_INT(rc, 0, "delete missing range succeeds");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist delete_range missing lines no-op");
}

/* Test: gwlist_delete_range 空程序 — 无副作用成功 */
void test_gwlist_delete_empty(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    int rc = gwlist_delete_range(10, 20);
    TEST_ASSERT_EQ_INT(rc, 0, "delete on empty program succeeds");
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
