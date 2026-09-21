/* test_gwdata.c - GWDATA 模块单元测试
 * 基于 GWDATA.ASM: 变量表、字符串堆、程序缓冲区、运行时状态 */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: 变量表初始化 */
void test_gwdata_var_table_init(void) {
    gwdata_init();
    gwram_init();
    
    /* 验证变量表初始状态 */
    struct variable *v = gwdata_find_variable("A");
    TEST_ASSERT(v == NULL, "新初始化时变量表应为空");
    
    /* 分配简单变量 */
    v = gwdata_alloc_variable("A", VT_INTEGER);
    TEST_ASSERT(v != NULL, "应能分配整型变量 A");
    TEST_ASSERT(v->name[0] == 'A', "变量名应为 A");
    TEST_ASSERT(v->type == VT_INTEGER, "变量类型应为 INTEGER");
    TEST_ASSERT(v->value.i == 0, "整型变量默认值应为 0");
    
    /* 查找已分配变量 */
    struct variable *v2 = gwdata_find_variable("A");
    TEST_ASSERT(v2 == v, "find 应返回同一变量指针");
}

/* Test: 字符串堆分配 */
void test_gwdata_string_heap(void) {
    gwdata_init();
    gwram_init();
    
    /* 分配字符串 */
    char *s1 = gwdata_alloc_string(10);
    TEST_ASSERT(s1 != NULL, "字符串分配应成功");
    
    /* 写入并验证 */
    strcpy(s1, "HELLO");
    TEST_ASSERT(strcmp(s1, "HELLO") == 0, "字符串内容应正确");
    
    /* 多次分配 */
    char *s2 = gwdata_alloc_string(20);
    TEST_ASSERT(s2 != NULL, "第二次分配应成功");
    TEST_ASSERT(s2 != s1, "应分配不同地址");
}

/* Test: 变量类型处理 */
void test_gwdata_variable_types(void) {
    gwdata_init();
    gwram_init();
    
    /* 整型 % */
    struct variable *vi = gwdata_alloc_variable("I%", VT_INTEGER);
    vi->value.i = 32767;
    TEST_ASSERT(vi->value.i == 32767, "整型赋值");
    
    /* 单精度 ! */
    struct variable *vs = gwdata_alloc_variable("S!", VT_SINGLE);
    vs->value.f = 3.14f;
    TEST_ASSERT(fabs(vs->value.f - 3.14f) < 0.001f, "单精度赋值");
    
    /* 双精度 # */
    struct variable *vd = gwdata_alloc_variable("D#", VT_DOUBLE);
    vd->value.d = 2.718281828;
    TEST_ASSERT(fabs(vd->value.d - 2.718281828) < 0.000001, "双精度赋值");
    
    /* 字符串 $ */
    struct variable *vstr = gwdata_alloc_variable("STR$", VT_STRING);
    vstr->value.s.ptr = gwdata_alloc_string(10);
    vstr->value.s.len = 4;
    strcpy(vstr->value.s.ptr, "TEST");
    TEST_ASSERT(strcmp(vstr->value.s.ptr, "TEST") == 0, "字符串变量赋值");
}

/* Test: 数组变量 */
void test_gwdata_array_variables(void) {
    gwdata_init();
    gwram_init();
    
    /* 分配一维数组 A(10) */
    int dims[1] = {11};
    struct variable *arr = gwdata_alloc_array("A", VT_SINGLE, 1, dims);
    TEST_ASSERT(arr != NULL, "数组分配应成功");
    TEST_ASSERT(arr->is_array, "应标记为数组");
    TEST_ASSERT(arr->dim_count == 1, "维度数应为 1");
    TEST_ASSERT(arr->dims[0] == 11, "维度大小应为 11 (0-10)");
    
    /* 设置和获取元素 */
    float *data = (float*)arr->array_data;
    data[5] = 123.45f;
    TEST_ASSERT(fabs(data[5] - 123.45f) < 0.001f, "数组元素赋值");
}

/* Test: 字符串堆垃圾回收触发条件 */
void test_gwdata_string_heap_gc(void) {
    gwdata_init();
    gwram_init();
    
    /* 分配多个字符串直到触发 GC */
    char *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = gwdata_alloc_string(50);
        sprintf(ptrs[i], "STR%d", i);
    }
    
    /* 验证内容仍正确 */
    for (int i = 0; i < 100; i++) {
        char expected[32];
        sprintf(expected, "STR%d", i);
        TEST_ASSERT(strcmp(ptrs[i], expected) == 0, "GC 后字符串内容应保持");
    }
}

/* Test: 程序缓冲区 */
void test_gwdata_program_buffer(void) {
    gwdata_init();
    gwram_init();
    
    /* 获取程序文本起始指针 */
    char *txttab = gwdata_get_txttab();
    TEST_ASSERT(txttab != NULL, "TXTTAB 应有效");
    
    /* 写入简单程序 */
    const char *prog = "10 PRINT \"HELLO\"\n20 END\n\0";
    strcpy(txttab, prog);
    
    /* 验证 */
    TEST_ASSERT(strstr(txttab, "PRINT") != NULL, "程序应包含 PRINT");
    TEST_ASSERT(strstr(txttab, "END") != NULL, "程序应包含 END");
}

/* Test: 错误消息表 */
void test_gwdata_error_messages(void) {
    gwdata_init();
    gwram_init();
    
    const char *msg = gwdata_get_error_message(ERRSN);  /* Syntax error */
    TEST_ASSERT(msg != NULL, "错误消息应存在");
    TEST_ASSERT(strlen(msg) > 0, "错误消息非空");
    
    msg = gwdata_get_error_message(ERRDV0);  /* Division by zero */
    TEST_ASSERT(msg != NULL, "除零错误消息应存在");
}

/* Test: 变量名大小写不敏感 */
void test_gwdata_case_insensitive(void) {
    gwdata_init();
    gwram_init();
    
    gwdata_alloc_variable("ABC", VT_INTEGER);
    struct variable *v = gwdata_find_variable("abc");
    TEST_ASSERT(v != NULL, "变量查找应不区分大小写");
    
    v = gwdata_find_variable("AbC");
    TEST_ASSERT(v != NULL, "混合大小写也应找到");
}

/* Test: 变量名最大长度 */
void test_gwdata_max_var_name(void) {
    gwdata_init();
    gwram_init();
    
    /* 40 字符变量名 */
    char name[41];
    memset(name, 'A', 40);
    name[40] = '\0';
    
    struct variable *v = gwdata_alloc_variable(name, VT_INTEGER);
    TEST_ASSERT(v != NULL, "40 字符变量名应支持");
    TEST_ASSERT(strlen(v->name) == 40, "变量名长度应为 40");
}