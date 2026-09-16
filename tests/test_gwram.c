/* test_gwram.c - GWRAM 模块单元测试
 * 基于 GWRAM.ASM: 内存分配、数据段变量、缓冲区管理 */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: gwram 内存分配器基本功能 */
void test_gwram_alloc_free(void) {
    gwdata_init();
    gwram_init();
    
    void *p1 = gwram_alloc(100);
    TEST_ASSERT(p1 != NULL, "alloc 100 bytes");
    
    void *p2 = gwram_alloc(200);
    TEST_ASSERT(p2 != NULL, "alloc 200 bytes");
    
    TEST_ASSERT(p1 != p2, "different allocations return different pointers");
    
    gwram_free(p1);
    gwram_free(p2);
    TEST_PASS("basic alloc/free");
}

/* Test: gwram_realloc 基本功能 */
void test_gwram_realloc(void) {
    gwdata_init();
    gwram_init();
    
    void *p = gwram_alloc(50);
    TEST_ASSERT(p != NULL, "initial alloc");
    
    p = gwram_realloc(p, 150);
    TEST_ASSERT(p != NULL, "realloc larger");
    
    gwram_free(p);
    TEST_PASS("realloc");
}

/* Test: gwram_calloc 零初始化 */
void test_gwram_calloc(void) {
    gwdata_init();
    gwram_init();
    
    int *arr = (int *)gwram_calloc(10, sizeof(int));
    TEST_ASSERT(arr != NULL, "calloc 10 ints");
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQ_INT(arr[i], 0, "calloc zero-initializes");
    }
    
    gwram_free(arr);
    TEST_PASS("calloc zero-initializes");
}

/* Test: gwram 内存不足处理 */
void test_gwram_oom_handling(void) {
    gwdata_init();
    gwram_init();
    
    /* 尝试分配超过限制的内存 */
    void *p = gwram_alloc(100000000);  /* 100MB */
    /* 可能成功也可能失败，不应崩溃 */
    if (p) {
        gwram_free(p);
    }
    TEST_PASS("oom handling no crash");
}

/* Test: 多次分配释放循环 */
void test_gwram_stress(void) {
    gwdata_init();
    gwram_init();
    
    for (int i = 0; i < 1000; i++) {
        void *p = gwram_alloc(64);
        TEST_ASSERT(p != NULL, "stress alloc");
        gwram_free(p);
    }
    TEST_PASS("stress alloc/free cycle");
}

/* Test: gwram 变量存储区初始化 */
void test_gwram_variable_storage(void) {
    gwdata_init();
    gwram_init();
    
    /* 测试变量表区域可访问 */
    struct variable *v = gwdata_alloc_variable("TEST", VT_INTEGER);
    TEST_ASSERT(v != NULL, "variable allocation in gwram");
    TEST_ASSERT(v->value.i == 0, "initial value zero");
    
    /* 字符串堆也在 gwram 管理范围 */
    char *s = gwdata_alloc_string(20);
    TEST_ASSERT(s != NULL, "string heap allocation");
    strcpy(s, "TEST STRING");
    TEST_ASSERT(strcmp(s, "TEST STRING") == 0, "string content");
    
    TEST_PASS("variable storage area");
}

/* Test: 边界条件 - 零大小分配 */
void test_gwram_zero_size(void) {
    gwdata_init();
    gwram_init();
    
    void *p = gwram_alloc(0);
    /* 零大小分配行为实现定义，不应崩溃 */
    if (p) gwram_free(p);
    TEST_PASS("zero size alloc no crash");
}

/* Test: realloc 到 0 相当于 free */
void test_gwram_realloc_zero(void) {
    gwdata_init();
    gwram_init();
    
    void *p = gwram_alloc(100);
    TEST_ASSERT(p != NULL, "initial alloc");
    
    p = gwram_realloc(p, 0);
    /* realloc(ptr, 0) 实现定义，可能返回 NULL 或有效指针 */
    /* 不应崩溃 */
    TEST_PASS("realloc to zero no crash");
}