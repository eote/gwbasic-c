/* TEST_GWMAIN_DELETE_UAF - CRITICAL-1 (v125): DELETE 删除当前/后续行 → 执行指针悬垂 */
/* 回归测试: 10 DELETE 20 (20 不应执行) / 30 DELETE -40 (删除当前行, 从下一现存行继续) */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: gwlist_delete_line 返回下一行指针 (修复后 API) — 删除当前行后可安全重定位 */
void test_gwlist_delete_returns_next(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");

    /* 修复后: gwlist_delete_line 返回被删行的下一行节点 (无 UAF 重定位用) */
    struct program_line *next = gwlist_delete_line(20);
    TEST_ASSERT(next != NULL, "delete returns next node");
    TEST_ASSERT_EQ_INT(next->number, 30, "next is line 30");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone");
    TEST_PASS("gwlist_delete_line returns next");
}

/* Test: gwlist_delete_range 返回第一被删行的下一行 (区间) */
void test_gwlist_delete_range_returns_next(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");
    gwlist_add_line(30, "C = 3");
    gwlist_add_line(40, "D = 4");

    /* 修复后: 区间删除返回区间后第一个现存行 (安全重定位锚点) */
    struct program_line *next = gwlist_delete_range(20, 30);
    TEST_ASSERT(next != NULL, "delete range returns next node");
    TEST_ASSERT_EQ_INT(next->number, 40, "next is line 40");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "20 gone");
    TEST_ASSERT(gwlist_find_line(30) == NULL, "30 gone");
    TEST_PASS("gwlist_delete_range returns next");
}

/* Test: gwlist_delete_range 删到末尾 — 返回 NULL (无下一行)
 * (v127 LOW-1: (20, 65530) — 与 DELETE 40- 的 ALLLST 哨兵一致; 补 "20 gone" 断言) */
void test_gwlist_delete_range_to_end(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();

    gwlist_add_line(10, "A = 1");
    gwlist_add_line(20, "B = 2");

    struct program_line *next = gwlist_delete_range(20, 65530);  /* 20-末尾 (ASM ALLLST 哨兵) */
    TEST_ASSERT(next == NULL, "delete to end returns NULL");
    TEST_ASSERT(gwlist_find_line(20) == NULL, "line 20 gone (actually deleted)");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line 10 kept");
    TEST_PASS("gwlist_delete_range to end NULL");
}
