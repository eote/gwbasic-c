/* TEST_GIODSK - Unit tests for GIODSK (Generalized I/O Disk Driver) */
/* Based on GIODSK.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test giodsk_init */
void test_giodsk_init(void) {
    giodsk_init();
    TEST_PASS("giodsk_init");
}

/* Test giodsk_open - sequential input */
void test_giodsk_open_sqi(void) {
    giodsk_init();
    /* First create the file */
    giodsk_open("test_sqi.txt", 1, 2, 0);  /* FM_OUTPUT = 2 */
    giodsk_write(1, "test data", 9);
    giodsk_close(1);
    
    /* Now open for input */
    int res = giodsk_open("test_sqi.txt", 1, 1, 0);  /* FM_INPUT = 1 */
    TEST_ASSERT(res == 0, "Open sequential input succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_open_sqi");
}

/* Test giodsk_open - sequential output */
void test_giodsk_open_sqo(void) {
    giodsk_init();
    int res = giodsk_open("test_sqo.txt", 1, 2, 0);  /* FM_OUTPUT = 2 */
    TEST_ASSERT(res == 0, "Open sequential output succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_open_sqo");
}

/* Test giodsk_open - random access */
void test_giodsk_open_rnd(void) {
    giodsk_init();
    int res = giodsk_open("test_rnd.txt", 1, 3, 128);  /* FM_RANDOM = 3 */
    TEST_ASSERT(res == 0, "Open random access succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_open_rnd");
}

/* Test giodsk_open - append */
void test_giodsk_open_app(void) {
    giodsk_init();
    int res = giodsk_open("test_app.txt", 1, 4, 0);  /* FM_APPEND = 4 */
    TEST_ASSERT(res == 0, "Open append succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_open_app");
}

/* Test giodsk_open - invalid filenum */
void test_giodsk_open_invalid_filenum(void) {
    giodsk_init();
    int res = giodsk_open("test.txt", -1, 1, 0);
    TEST_ASSERT(res != 0, "Invalid filenum fails");
    TEST_PASS("giodsk_open_invalid_filenum");
}

/* Test giodsk_close */
void test_giodsk_close(void) {
    giodsk_init();
    giodsk_open("test_close.txt", 1, 2, 0);
    int res = giodsk_close(1);
    TEST_ASSERT(res == 0, "Close succeeds");
    TEST_PASS("giodsk_close");
}

/* Test giodsk_close_all */
void test_giodsk_close_all(void) {
    giodsk_init();
    giodsk_open("test1.txt", 1, 2, 0);
    giodsk_open("test2.txt", 2, 2, 0);
    int res = giodsk_close_all();
    TEST_ASSERT(res == 0, "Close all succeeds");
    TEST_PASS("giodsk_close_all");
}

/* Test giodsk_print */
void test_giodsk_print(void) {
    giodsk_init();
    giodsk_open("test_print.txt", 1, 2, 0);
    int res = giodsk_print(1, "Hello, World!");
    TEST_ASSERT(res == 0, "Print succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_print");
}

/* Test giodsk_write */
void test_giodsk_write(void) {
    giodsk_init();
    giodsk_open("test_write.txt", 1, 2, 0);
    const char *data = "Test data";
    int res = giodsk_write(1, data, (int)strlen(data));
    TEST_ASSERT(res == 0, "Write succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_write");
}

/* Test giodsk_input */
void test_giodsk_input(void) {
    giodsk_init();
    giodsk_open("test_input.txt", 1, 2, 0);
    giodsk_write(1, "Test data", 9);
    giodsk_close(1);
    
    giodsk_open("test_input.txt", 1, 1, 0);
    char buffer[64];
    int res = giodsk_input(1, buffer, 64);
    TEST_ASSERT(res >= 0, "Input succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_input");
}

/* Test giodsk_line_input */
void test_giodsk_line_input(void) {
    giodsk_init();
    giodsk_open("test_line.txt", 1, 2, 0);
    giodsk_write(1, "Line 1\nLine 2\n", 14);
    giodsk_close(1);
    
    giodsk_open("test_line.txt", 1, 1, 0);
    char buffer[64];
    int res = giodsk_line_input(1, buffer, 64);
    TEST_ASSERT(res >= 0, "Line input succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_line_input");
}

/* Test giodsk_seek */
void test_giodsk_seek(void) {
    giodsk_init();
    giodsk_open("test_seek.txt", 1, 3, 128);  /* Random mode */
    int res = giodsk_seek(1, 5);  /* Seek to record 5 */
    TEST_ASSERT(res == 0, "Seek succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_seek");
}

/* Test giodsk_get */
void test_giodsk_get(void) {
    giodsk_init();
    giodsk_open("test_get.txt", 1, 3, 128);
    char buffer[128];
    int res = giodsk_get(1, 1, buffer, 128);
    TEST_ASSERT(res >= 0, "Get succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_get");
}

/* Test giodsk_put */
void test_giodsk_put(void) {
    giodsk_init();
    giodsk_open("test_put.txt", 1, 3, 128);
    char buffer[128] = "Record data";
    int res = giodsk_put(1, 1, buffer, 128);
    TEST_ASSERT(res == 0, "Put succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_put");
}

/* Test multiple file operations */
void test_giodsk_multiple_files(void) {
    giodsk_init();
    
    giodsk_open("file1.txt", 1, 2, 0);
    giodsk_open("file2.txt", 2, 2, 0);
    giodsk_open("file3.txt", 3, 3, 128);
    
    giodsk_close(1);
    giodsk_close(2);
    giodsk_close(3);
    
    TEST_PASS("giodsk_multiple_files");
}

/* Test open close cycle */
void test_giodsk_open_close_cycle(void) {
    giodsk_init();
    
    for (int i = 0; i < 5; i++) {
        char filename[32];
        sprintf(filename, "cycle%d.txt", i);
        giodsk_open(filename, 1, 2, 0);
        giodsk_write(1, "data", 4);
        giodsk_close(1);
    }
    
    TEST_PASS("giodsk_open_close_cycle");
}

/* Test binary mode operations */
void test_giodsk_binary_mode(void) {
    giodsk_init();
    giodsk_open("binary.dat", 1, 3, 128);
    
    struct {
        int a;
        float b;
        char c;
    } data = {42, 3.14f, 'X'};
    
    int res = giodsk_put(1, 1, &data, sizeof(data));
    TEST_ASSERT(res == 0, "Binary put succeeds");
    
    struct {
        int a;
        float b;
        char c;
    } read_data = {0, 0, 0};
    
    res = giodsk_get(1, 1, &read_data, sizeof(read_data));
    TEST_ASSERT(res >= 0, "Binary get succeeds");
    
    giodsk_close(1);
    TEST_PASS("giodsk_binary_mode");
}

/* Test empty file operations */
void test_giodsk_empty_file(void) {
    giodsk_init();
    giodsk_open("empty.txt", 1, 1, 0);
    char buffer[64];
    int res = giodsk_line_input(1, buffer, 64);
    TEST_ASSERT(res == 0 || res < 0, "Empty file handled");
    giodsk_close(1);
    TEST_PASS("giodsk_empty_file");
}

/* Test invalid operations on closed file */
void test_giodsk_invalid_closed(void) {
    giodsk_init();
    giodsk_open("test.txt", 1, 2, 0);
    giodsk_close(1);
    
    char buffer[64];
    int res = giodsk_input(1, buffer, 64);
    TEST_ASSERT(res < 0, "Input on closed file fails");
    TEST_PASS("giodsk_invalid_closed");
}

/* Test file write then read */
void test_giodsk_write_read(void) {
    giodsk_init();
    giodsk_open("wr.txt", 1, 2, 0);
    giodsk_write(1, "Write test", 10);
    giodsk_close(1);
    
    giodsk_open("wr.txt", 1, 1, 0);
    char buffer[64];
    int res = giodsk_input(1, buffer, 64);
    TEST_ASSERT(res >= 0, "Read after write succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_write_read");
}

/* Test append mode */
void test_giodsk_append(void) {
    giodsk_init();
    giodsk_open("append.txt", 1, 2, 0);
    giodsk_write(1, "First\n", 6);
    giodsk_close(1);
    
    giodsk_open("append.txt", 1, 4, 0);  /* Append mode */
    giodsk_write(1, "Second\n", 7);
    giodsk_close(1);
    
    giodsk_open("append.txt", 1, 1, 0);
    char buffer[128];
    int res = giodsk_input(1, buffer, 128);
    TEST_ASSERT(res >= 0, "Append read succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_append");
}

/* Test random record size */
void test_giodsk_record_size(void) {
    giodsk_init();
    int res = giodsk_open("recsize.dat", 1, 3, 256);  /* 256 byte records */
    TEST_ASSERT(res == 0, "Open with custom record size succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_record_size");
}

/* Test default record size (128) */
void test_giodsk_default_record_size(void) {
    giodsk_init();
    int res = giodsk_open("default.dat", 1, 3, 0);  /* 0 = default */
    TEST_ASSERT(res == 0, "Open with default record size succeeds");
    giodsk_close(1);
    TEST_PASS("giodsk_default_record_size");
}