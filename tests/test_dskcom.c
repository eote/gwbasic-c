/* GW-BASIC C Port - DSKCOM Tests */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include "../src/dskcom.h"

void test_dskcom_init(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists");
    TEST_PASS("dskcom_init");
}

void test_dskcom_open_close(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* Create a test file */
    FILE *f = fopen("test_dskcom.txt", "w");
    if (f) {
        fprintf(f, "Hello, World!\nLine 2\n");
        fclose(f);
    }
    
    /* Test OPEN FOR INPUT */
    int rc = dskcom_open("test_dskcom.txt", 1, FM_INPUT, 0);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR INPUT");
    
    struct file_desc *fd = dskcom_find_file(1);
    TEST_ASSERT(fd != NULL, "File descriptor exists");
    TEST_ASSERT_EQ_INT(fd->filenum, 1, "File number correct");
    TEST_ASSERT_EQ_INT(fd->mode, FM_INPUT, "Mode correct");
    
    /* Test CLOSE */
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE");
    
    fd = dskcom_find_file(1);
    TEST_ASSERT(fd == NULL, "File descriptor removed after CLOSE");
    
    /* Test OPEN FOR OUTPUT */
    rc = dskcom_open("test_output.txt", 2, FM_OUTPUT, 0);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR OUTPUT");
    
    rc = dskcom_close(2);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE output");
    
    remove("test_dskcom.txt");
    remove("test_output.txt");
    
    TEST_PASS("open/close");
}

void test_dskcom_print(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* OPEN FOR OUTPUT */
    int rc = dskcom_open("test_print.txt", 1, FM_OUTPUT, 0);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR OUTPUT");
    
    /* PRINT# */
    rc = dskcom_print(1, "Hello");
    TEST_ASSERT_EQ_INT(rc, 0, "PRINT# 1");
    
    rc = dskcom_print(1, "World");
    TEST_ASSERT_EQ_INT(rc, 0, "PRINT# 2");
    
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE");
    
    /* Verify content */
    FILE *f = fopen("test_print.txt", "r");
    TEST_ASSERT(f != NULL, "File created");
    if (f) {
        char buf[256];
        fgets(buf, sizeof(buf), f);
        TEST_ASSERT_EQ_STR(buf, "Hello\n", "First line");
        fgets(buf, sizeof(buf), f);
        TEST_ASSERT_EQ_STR(buf, "World\n", "Second line");
        fclose(f);
    }
    
    remove("test_print.txt");
    
    TEST_PASS("print#");
}

void test_dskcom_input(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* Create test file with comma-separated values */
    FILE *f = fopen("test_input.txt", "w");
    if (f) {
        fprintf(f, "Hello,World,42\n");
        fclose(f);
    }
    
    /* OPEN FOR INPUT */
    int rc = dskcom_open("test_input.txt", 1, FM_INPUT, 0);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR INPUT");
    
    /* First INPUT# - "Hello" */
    char buf[256];
    rc = dskcom_input(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "INPUT# 1");
    TEST_ASSERT_EQ_STR(buf, "Hello", "First field");
    
    /* Second INPUT# - "World" */
    rc = dskcom_input(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "INPUT# 2");
    TEST_ASSERT_EQ_STR(buf, "World", "Second field");
    
    /* Third INPUT# - "42" */
    rc = dskcom_input(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "INPUT# 3");
    TEST_ASSERT_EQ_STR(buf, "42", "Third field");
    
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE");
    
    remove("test_input.txt");
    
    TEST_PASS("input#");
}

void test_dskcom_line_input(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* Create test file with lines */
    FILE *f = fopen("test_line.txt", "w");
    if (f) {
        fprintf(f, "First line\nSecond line\n");
        fclose(f);
    }
    
    int rc = dskcom_open("test_line.txt", 1, FM_INPUT, 0);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR INPUT");
    
    char buf[256];
    rc = dskcom_line_input(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "LINE INPUT# 1");
    TEST_ASSERT_EQ_STR(buf, "First line", "First line");
    
    rc = dskcom_line_input(1, buf, sizeof(buf));
    TEST_ASSERT_EQ_INT(rc, 0, "LINE INPUT# 2");
    TEST_ASSERT_EQ_STR(buf, "Second line", "Second line");
    
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE");
    
    remove("test_line.txt");
    
    TEST_PASS("line input#");
}

void test_dskcom_write_seek(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* Test random file mode */
    int rc = dskcom_open("test_random.dat", 1, FM_RANDOM, 32);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR RANDOM");
    
    struct file_desc *fd = dskcom_find_file(1);
    TEST_ASSERT(fd != NULL, "File descriptor exists");
    TEST_ASSERT_EQ_INT(fd->record_length, 32, "Record length set");
    
    /* WRITE# record 1 */
    char record1[32] = "Record 1 data";
    rc = dskcom_write(1, record1, 32);
    TEST_ASSERT_EQ_INT(rc, 0, "WRITE# record 1");
    
    /* SEEK to record 1 and read back */
    rc = dskcom_seek(1, 1);
    TEST_ASSERT_EQ_INT(rc, 0, "SEEK record 1");
    
    /* For random access, we'd need a read function - test SEEK worked */
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE random");
    
    remove("test_random.dat");
    
    TEST_PASS("write/seek random");
}

void test_dskcom_get_put(void) {
    gwdata_init(); gwram_init(); gwinit_init();
    dskcom_init();
    
    /* Test PUT and GET with numeric variable */
    int rc = dskcom_open("test_getput.dat", 1, FM_RANDOM, 32);
    TEST_ASSERT_EQ_INT(rc, 0, "OPEN FOR RANDOM");
    
    struct file_desc *fd = dskcom_find_file(1);
    TEST_ASSERT(fd != NULL, "File descriptor exists");
    TEST_ASSERT_EQ_INT(fd->record_length, 32, "Record length set");
    
    /* PUT integer */
    int32_t test_int = 42;
    rc = dskcom_put(1, 1, &test_int, sizeof(int32_t));
    TEST_ASSERT_EQ_INT(rc, 0, "PUT integer");
    
    /* GET integer */
    int32_t read_int = 0;
    rc = dskcom_get(1, 1, &read_int, sizeof(int32_t));
    TEST_ASSERT_EQ_INT(rc, 0, "GET integer");
    TEST_ASSERT_EQ_INT(read_int, 42, "Integer value correct");
    
    /* PUT float */
    float test_float = 3.14159f;
    rc = dskcom_put(1, 2, &test_float, sizeof(float));
    TEST_ASSERT_EQ_INT(rc, 0, "PUT float");
    
    /* GET float */
    float read_float = 0.0f;
    rc = dskcom_get(1, 2, &read_float, sizeof(float));
    TEST_ASSERT_EQ_INT(rc, 0, "GET float");
    TEST_ASSERT(read_float > 3.14f && read_float < 3.15f, "Float value correct");
    
    rc = dskcom_close(1);
    TEST_ASSERT_EQ_INT(rc, 0, "CLOSE");
    
    remove("test_getput.dat");
    
    TEST_PASS("get/put");
}