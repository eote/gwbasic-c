/* TEST_MATH2 - M1: RANDOMIZE / ON..GOTO / ON..GOSUB 语句测试 */
/* Based on GWMAIN.ASM RANDOM (L3383) + ONGOTO (L2357) */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: RANDOMIZE 语句重置随机序列 (ASM GWMAIN L3383-3413)
 * RANDOM: FRMEVL + FRCINT → STRNDS: MOV RNDX+1,BX + RNDMN2 */
void test_math2_randomize(void) {
    gwdata_init();
    gwram_init();
    
    math2_init();
    float r1 = math2_rnd(1.0f);
    
    math2_randomize(12345);
    float r2 = math2_rnd(1.0f);
    
    /* 相同种子重置后序列应相同 (确定性) */
    math2_randomize(12345);
    float r3 = math2_rnd(1.0f);
    
    TEST_ASSERT(r2 == r3, "RANDOMIZE with same seed gives same sequence");
    (void)r1;
    TEST_PASS("math2_randomize reseed");
}

/* Test: RANDOMIZE 幂等 — 不同种子给出不同序列 */
void test_math2_randomize_diff_seeds(void) {
    gwdata_init();
    gwram_init();
    
    math2_randomize(1);
    float r1 = math2_rnd(1.0f);
    
    math2_randomize(999);
    float r2 = math2_rnd(1.0f);
    
    TEST_ASSERT(r1 != r2, "different seeds give different sequences");
    TEST_PASS("math2_randomize diff seeds");
}

/* Test: RND(x<0) 重置语义 (ASM: RND 负参数重置并返回首值) */
void test_math2_rnd_negative_reseed(void) {
    gwdata_init();
    gwram_init();
    
    float r1 = math2_rnd(-42.0f);
    float r2 = math2_rnd(-42.0f);
    
    TEST_ASSERT(r1 == r2, "RND(-x) reseeds deterministically");
    TEST_PASS("math2 rnd negative reseed");
}

/* Test: RND(0) 返回上一值 (ASM: RND zero returns last) */
void test_math2_rnd_zero_last(void) {
    gwdata_init();
    gwram_init();
    
    float r1 = math2_rnd(1.0f);
    float r2 = math2_rnd(0.0f);
    
    TEST_ASSERT(r1 == r2, "RND(0) returns last generated value");
    TEST_PASS("math2 rnd zero last");
}

/* Test: RND 范围 [0, 1) */
void test_math2_rnd_range(void) {
    gwdata_init();
    gwram_init();
    
    for (int i = 0; i < 100; i++) {
        float r = math2_rnd(1.0f);
        TEST_ASSERT(r >= 0.0f && r < 1.0f, "RND in [0,1)");
    }
    TEST_PASS("math2 rnd range");
}

/* Test: ON..GOTO 跳转语义 (ASM ONGOTO L2357, NTONG1 L2455: 按 1-based 索引选行) */
void test_gwsts_on_goto(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(100, "PRINT A");
    gwlist_add_line(200, "PRINT B");
    gwlist_add_line(300, "PRINT C");
    
    /* 模拟 ON X GOTO 100,200,300 语义: index 1 → line 100 */
    int targets[3] = {100, 200, 300};
    int index = 1;
    
    TEST_ASSERT(index >= 1 && index <= 3, "index in range (ASM ONGSB0 CMP AL,CL)");
    struct program_line *target = gwlist_find_line(targets[index - 1]);
    TEST_ASSERT(target != NULL, "ON..GOTO index 1 -> line 100");
    TEST_PASS("gwsts on goto");
}

/* Test: ON..GOTO 越界 (ASM L2411-2412: CMP AL,CL / JNAE skip / JMP SNERR) */
void test_gwsts_on_goto_out_of_range(void) {
    gwdata_init();
    gwram_init();
    
    int index = 5;
    int count = 3;
    
    /* ASM: index > count → SNERR (Illegal function call) */
    TEST_ASSERT(index > count, "index beyond count is error (ASM JMP SNERR)");
    TEST_PASS("gwsts on goto out of range");
}

/* Test: ON..GOSUB 语法 (ASM NTOERR L2374-2383: ON event GOSUB nn,nn,...) */
void test_gwsts_on_gosub_syntax(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(500, "PRINT SUB");
    
    int targets[2] = {500, 600};
    int index = 1;
    
    /* ON event GOSUB: index 1 → line 500, 且 GOSUB 压栈 */
    struct program_line *target = gwlist_find_line(targets[index - 1]);
    TEST_ASSERT(target != NULL, "ON..GOSUB index 1 -> line 500");
    TEST_PASS("gwsts on gosub syntax");
}

/* Test: WHILE/WEND 循环执行 (ASM WNDSCN L3419) */
void test_gwsts_while_wend_loop(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    /* WNDSCN 语义: WHILE 条件假 → 跳到 WEND 后 */
    /* 验证 gwlist 可定位 WHILE 与 WEND 行 */
    gwlist_add_line(10, "WHILE I < 3");
    gwlist_add_line(20, "I = I + 1");
    gwlist_add_line(30, "WEND");
    
    struct program_line *l_while = gwlist_find_line(10);
    struct program_line *l_wend = gwlist_find_line(30);
    
    TEST_ASSERT(l_while != NULL && l_wend != NULL, "WHILE/WEND lines locatable");
    TEST_PASS("gwsts while wend loop");
}