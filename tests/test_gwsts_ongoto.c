/* TEST_GWSTS_ONGOTO - M2: ON..GOTO / ON..GOSUB 语句 (v129 M2 建议) */
/* Based on GWMAIN.ASM NTONG1/LOOPON L2446-2460 — 1-based 索引选择跳转目标;
 * 越界 (index > count) → ASM L2453-2456: 列表耗尽 → RET → 下一语句 (无分支) */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: ON I GOTO — I=1 → 第一目标 (ASM 1-based) */
void test_gwsts_on_goto_index1(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *i = gwdata_alloc_variable("I", VT_SINGLE);
    i->value.f = 1.0f;

    int rc = gwsts_exec_statement("ON I GOTO 100, 200, 300");
    TEST_ASSERT_EQ_INT(rc, 100, "ON I GOTO I=1 -> line 100");
    TEST_PASS("gwsts on goto index1");
}

/* Test: ON I GOTO — I=2 → 第二目标 */
void test_gwsts_on_goto_index2(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *i = gwdata_alloc_variable("I", VT_SINGLE);
    i->value.f = 2.0f;

    int rc = gwsts_exec_statement("ON I GOTO 100, 200, 300");
    TEST_ASSERT_EQ_INT(rc, 200, "ON I GOTO I=2 -> line 200");
    TEST_PASS("gwsts on goto index2");
}

/* Test: ON I GOTO — I=3 → 第三目标 */
void test_gwsts_on_goto_index3(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *i = gwdata_alloc_variable("I", VT_SINGLE);
    i->value.f = 3.0f;

    int rc = gwsts_exec_statement("ON I GOTO 100, 200, 300");
    TEST_ASSERT_EQ_INT(rc, 300, "ON I GOTO I=3 -> line 300");
    TEST_PASS("gwsts on goto index3");
}

/* Test: ON I GOTO — 越界 → 下一语句 (ASM L2453-2456 RET) */
void test_gwsts_ongoto_out_of_range(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *i = gwdata_alloc_variable("I", VT_SINGLE);
    i->value.f = 5.0f;

    int rc = gwsts_exec_statement("ON I GOTO 100, 200, 300");
    TEST_ASSERT_EQ_INT(rc, 0, "ON I GOTO I=5 out of range -> next statement");
    TEST_PASS("gwsts on goto out of range");
}

/* Test: ON I GOSUB — 跳转 + gosub 栈 */
void test_gwsts_on_gosub(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    gwlist_add_line(10, "ON I GOSUB 100, 200");
    gwlist_add_line(20, "PRINT back");
    gwlist_add_line(100, "PRINT one");

    struct variable *i = gwdata_alloc_variable("I", VT_SINGLE);
    i->value.f = 1.0f;

    extern WORD g_curlin;
    g_curlin = 10;

    int rc = gwsts_exec_statement("ON I GOSUB 100, 200");
    TEST_ASSERT_EQ_INT(rc, 100, "ON I GOSUB I=1 -> line 100");
    TEST_PASS("gwsts on gosub");
}

/* Test: ON 0 GOTO — 无分支 (0 不在 1..count) */
void test_gwsts_on_goto_zero(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    int rc = gwsts_exec_statement("ON 0 GOTO 100, 200");
    TEST_ASSERT_EQ_INT(rc, 0, "ON 0 GOTO -> next statement");
    TEST_PASS("gwsts on goto zero");
}

/* Test: 变量名含 GOTO 子串 (v130 MEDIUM-1) — ON XGOTO GOTO 表达式不截断 */
void test_gwsts_on_goto_var_with_goto_substring(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *xg = gwdata_alloc_variable("XGOTO", VT_SINGLE);
    xg->value.f = 2.0f;

    int rc = gwsts_exec_statement("ON XGOTO GOTO 100, 200");
    TEST_ASSERT_EQ_INT(rc, 200, "ON XGOTO GOTO XGOTO=2 -> line 200 (no truncation)");
    TEST_PASS("gwsts on goto var with GOTO substring");
}

/* Test: 缺真关键字 — ON XGOTO 100 → SNERR (v130 MEDIUM-1 连带) */
void test_gwsts_on_goto_missing_keyword(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    struct variable *xg = gwdata_alloc_variable("XGOTO", VT_SINGLE);
    xg->value.f = 1.0f;

    int rc = gwsts_exec_statement("ON XGOTO 100");
    TEST_ASSERT_EQ_INT(rc, -1, "ON XGOTO 100 (no GOTO keyword) -> SNERR");
    TEST_PASS("gwsts on goto missing keyword");
}

/* Test: 紧凑写法 ON2GOTO (v130 LOW-1) — 数字紧邻关键字 */
void test_gwsts_on_goto_compact(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwsts_init();
    gwlist_init();

    int rc = gwsts_exec_statement("ON 2GOTO 100, 200");
    TEST_ASSERT_EQ_INT(rc, 200, "ON 2GOTO 100, 200 compact -> line 200");
    TEST_PASS("gwsts on goto compact");
}
