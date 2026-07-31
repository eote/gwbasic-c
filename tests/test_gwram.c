/* GW-BASIC C Port - GWRAM Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"

void test_gwram_alloc_free(void) {
    gwdata_init();
    
    void *p1 = gwram_alloc(100);
    TEST_ASSERT(p1 != NULL, "alloc 100 bytes");
    
    void *p2 = gwram_alloc(200);
    TEST_ASSERT(p2 != NULL, "alloc 200 bytes");
    
    TEST_ASSERT(p1 != p2, "different allocations return different pointers");
    
    gwram_free(p1);
    gwram_free(p2);
    TEST_PASS("basic alloc/free");
}

void test_gwram_realloc(void) {
    gwdata_init();
    
    void *p = gwram_alloc(50);
    TEST_ASSERT(p != NULL, "initial alloc");
    
    p = gwram_realloc(p, 150);
    TEST_ASSERT(p != NULL, "realloc larger");
    
    gwram_free(p);
    TEST_PASS("realloc");
}

void test_gwram_calloc(void) {
    gwdata_init();
    
    int *arr = (int *)gwram_calloc(10, sizeof(int));
    TEST_ASSERT(arr != NULL, "calloc 10 ints");
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQ_INT(arr[i], 0, "calloc zero-initializes");
    }
    
    gwram_free(arr);
    TEST_PASS("calloc zero-initializes");
}

void test_gwram_oom_handling(void) {
    gwdata_init();
    
    /* Try to allocate more than available - should return NULL */
    void *p = gwram_alloc(SIZE_MAX);
    TEST_ASSERT(p == NULL, "OOM returns NULL");
    TEST_PASS("OOM handling");
}