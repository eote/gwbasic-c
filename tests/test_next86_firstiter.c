/* test_next86_firstiter.c — v145: FOR 首次迭代检查 (ASM GWMAIN FOR L1445
 * → JMP NEXTS NXTFLG=0; IFORIN 用初值比较; LOOPDN 体 0 次) + STEP 0 无限
 * 循环哨兵 (ASM NT0STP: sign 强制 2, 终止测试永假)。
 * GW-BASIC 语义:
 * - FOR I=3 TO 1 (正 step) → 体不执行, I 保持 3 (不加 step)
 * - FOR I=1 TO 3 STEP -1 (负 step) → 体不执行
 * - FOR I=3 TO 3 → 体执行 1 次
 * - 嵌套 done: FOR I=1 TO 0: FOR J...: NEXT J: NEXT I → 外层 done 跳到
 *   外层 NEXT 之后 (深度感知扫描) */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "../src/gwbasic.h"

extern int gwsts_exec_statement(const char *text);
extern struct variable *gwdata_find_variable(const char *name);
extern void gwdata_clear_error(void);
extern void next86_clear_for_stack(void);

/* Test: FOR 3 TO 1 (positive step, start > limit) — body must not run,
 * loop variable keeps its initial value (ASM: comparison at FOR uses the
 * initial value; no step is added) */
void test_next86_for_firstiter_skip_positive(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    gwlist_init();
    gwlist_add_line(10, "FOR I = 3 TO 1");
    gwlist_add_line(20, "PRINT \"BODY\"");
    gwlist_add_line(30, "NEXT I");
    gwlist_add_line(40, "PRINT \"AFTER=\"; I");
    gwlist_add_line(50, "END");
    extern WORD g_curlin;
    g_curlin = 10;   /* FOR is on line 10 */
    /* program context: line 10 FOR, 20 body, 30 NEXT, 40 after */
    gwlist_add_line(10, "FOR I = 3 TO 1");
    gwlist_add_line(20, "PRINT \"BODY\"");
    gwlist_add_line(30, "NEXT I");
    gwlist_add_line(40, "PRINT \"AFTER=\"; I");
    /* FOR statement must signal "skip body" — rc = line after the matching
     * NEXT (like ASM LOOPDN → text ptr after NEXT) */
    const char *text = "I = 3 TO 1";
    extern int next86_for(const char **text);
    int rc = next86_for(&text);
    TEST_ASSERT_EQ_INT(rc, 40, "FOR 3 TO 1 → jump to line after NEXT (40)");
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "no error");
    struct variable *v = gwdata_find_variable("I");
    TEST_ASSERT(v, "I allocated");
    TEST_ASSERT_EQ_INT((int)v->value.f, 3, "I keeps initial value 3 (no step added)");
    TEST_PASS("next86 firstiter skip positive");
}

/* Test: FOR with negative step, start < limit — body must not run */
void test_next86_for_firstiter_skip_negative(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    struct variable *v = gwdata_alloc_variable("I", VT_SINGLE);
    v->value.f = 1.0f;
    /* program context for the matching-NEXT scan */
    gwlist_add_line(10, "FOR I = 1 TO 3 STEP -1");
    gwlist_add_line(20, "PRINT \"BODY\"");
    gwlist_add_line(30, "NEXT I");
    gwlist_add_line(40, "PRINT \"AFTER=\"; I");
    extern WORD g_curlin;
    g_curlin = 10;
    /* FOR I = 1 TO 3 STEP -1 */
    const char *text = "I = 1 TO 3 STEP -1";
    extern int next86_for(const char **text);
    int rc = next86_for(&text);
    TEST_ASSERT_EQ_INT(rc, 40, "negative-step skip → after NEXT (40)");
    /* I must keep initial value (no negative step applied) */
    TEST_ASSERT_EQ_INT((int)v->value.f, 1, "I keeps initial 1");
    TEST_PASS("next86 firstiter skip negative");
}

/* Test: FOR 3 TO 3 — body runs exactly once (boundary equality) */
void test_next86_for_firstiter_boundary_runs(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    /* FOR I = 3 TO 3 — start == limit → loop body runs once */
    struct variable *v = gwdata_alloc_variable("I", VT_SINGLE);
    v->value.f = 3.0f;
    const char *text = "I = 3 TO 3";
    extern int next86_for(const char **text);
    int rc = next86_for(&text);
    TEST_ASSERT_EQ_INT(rc, 0, "boundary FOR continues (rc=0, body runs)");
    TEST_PASS("next86 firstiter boundary runs");
}

/* Test: STEP 0 → infinite loop sentinel (ASM NT0STP: step sign forced to 2,
 * termination test can never be met) — FOR 5 TO 1 STEP 0 must NOT skip */
void test_next86_for_step0_infinite_sentinel(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    /* FOR I = 5 TO 1 STEP 0 — start > limit but step 0 → loop entered */
    struct variable *v = gwdata_alloc_variable("I", VT_SINGLE);
    v->value.f = 5.0f;
    const char *text = "I = 5 TO 1 STEP 0";
    extern int next86_for(const char **text);
    int rc = next86_for(&text);
    TEST_ASSERT_EQ_INT(rc, 0, "STEP 0 enters loop (infinite sentinel)");
    TEST_PASS("next86 step0 sentinel");
}


/* Test: v145 MEDIUM-1 — FOR 0 iterations with NEXT as the LAST line:
 * scan finds no line after NEXT → program end signal (-2, like END);
 * body must not run (previously returned 0 → "continue" → BODY executed) */
void test_next86_for_skip_next_last_line(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    gwlist_clear();
    gwlist_add_line(10, "FOR I=1 TO 0");
    gwlist_add_line(20, "PRINT \"BODY\"");
    gwlist_add_line(30, "NEXT I");
    extern WORD g_curlin;
    g_curlin = 10;
    /* FOR statement returns -2 (program end, like END) since there is no
     * line after the matching NEXT */
    const char *text = "I=1 TO 0";
    extern int next86_for(const char **text);
    int rc = next86_for(&text);
    TEST_ASSERT_EQ_INT(rc, -2, "NEXT last line → end-of-program signal (-2)");
    struct variable *v = gwdata_find_variable("I");
    TEST_ASSERT(v, "I allocated");
    TEST_ASSERT_EQ_INT((int)v->value.f, 1, "I keeps initial 1");
    TEST_PASS("next86 skip when NEXT is last line");
}

/* gwsts-level: FOR skip via statement dispatch returns -2 to gwmain */
void test_next86_for_skip_gwsts_end_signal(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    gwdata_clear_error();
    next86_clear_for_stack();
    gwlist_clear();
    gwlist_add_line(10, "FOR I=1 TO 0");
    gwlist_add_line(20, "PRINT \"BODY\"");
    gwlist_add_line(30, "NEXT I");
    extern WORD g_curlin;
    g_curlin = 10;
    int rc = gwsts_exec_statement("FOR I=1 TO 0");
    TEST_ASSERT_EQ_INT(rc, -2, "gwsts FOR skip → -2 (END semantics)");
    TEST_PASS("next86 skip gwsts end signal");
}
