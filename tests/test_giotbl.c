/* TEST_GIOTBL - Unit tests for GIOTBL (I/O Tables) */
/* Based on GIOTBL.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test giotbl_init */
void test_giotbl_init(void) {
    giotbl_init();
    
    TEST_PASS("giotbl_init");
}

/* Test giotbl_term */
void test_giotbl_term(void) {
    giotbl_term();
    
    TEST_PASS("giotbl_term");
}

/* Test giotbl_get_ndev */
void test_giotbl_get_ndev(void) {
    int ndev = giotbl_get_ndev();
    
    TEST_ASSERT_EQ_INT(ndev, 11, "ndev should be 11");
    TEST_PASS("giotbl_get_ndev");
}

/* Test giotbl_get_device_id */
void test_giotbl_get_device_id(void) {
    int id = giotbl_get_device_id("KYBD");
    TEST_ASSERT_EQ_INT(id, 0, "KYBD should be 0");
    
    id = giotbl_get_device_id("SCRN");
    TEST_ASSERT_EQ_INT(id, 1, "SCRN should be 1");
    
    id = giotbl_get_device_id("CONS");
    TEST_ASSERT_EQ_INT(id, 2, "CONS should be 2");
    
    id = giotbl_get_device_id("LPT1");
    TEST_ASSERT_EQ_INT(id, 3, "LPT1 should be 3");
    
    id = giotbl_get_device_id("COM4");
    TEST_ASSERT_EQ_INT(id, 10, "COM4 should be 10");
    
    id = giotbl_get_device_id("UNKNOWN");
    TEST_ASSERT_EQ_INT(id, -1, "Unknown device should return -1");
    
    TEST_PASS("giotbl_get_device_id");
}

/* Test giotbl_get_device_name */
void test_giotbl_get_device_name(void) {
    const char *name = giotbl_get_device_name(0);
    TEST_ASSERT_EQ_STR(name, "KYBD", "Device 0 should be KYBD");
    
    name = giotbl_get_device_name(1);
    TEST_ASSERT_EQ_STR(name, "SCRN", "Device 1 should be SCRN");
    
    name = giotbl_get_device_name(10);
    TEST_ASSERT_EQ_STR(name, "COM4", "Device 10 should be COM4");
    
    name = giotbl_get_device_name(-1);
    TEST_ASSERT(name == NULL, "Invalid device should return NULL");
    
    name = giotbl_get_device_name(100);
    TEST_ASSERT(name == NULL, "Out of bounds device should return NULL");
    
    TEST_PASS("giotbl_get_device_name");
}

/* Test giotbl_lookup_device */
void test_giotbl_lookup_device(void) {
    int id = 0;
    int result = giotbl_lookup_device("KYBD", &id);
    TEST_ASSERT_EQ_INT(result, 0, "KYBD lookup should succeed");
    TEST_ASSERT_EQ_INT(id, 0, "KYBD id should be 0");
    
    result = giotbl_lookup_device("COM4", &id);
    TEST_ASSERT_EQ_INT(result, 0, "COM4 lookup should succeed");
    TEST_ASSERT_EQ_INT(id, 10, "COM4 id should be 10");
    
    result = giotbl_lookup_device("UNKNOWN", &id);
    TEST_ASSERT_EQ_INT(result, -1, "Unknown device lookup should fail");
    
    TEST_PASS("giotbl_lookup_device");
}

/* Test giotbl_get_dispatch */
void test_giotbl_get_dispatch(void) {
    void *dsp = NULL, *ini = NULL, *trm = NULL;
    int result = giotbl_get_dispatch(0, &dsp, &ini, &trm);
    TEST_ASSERT_EQ_INT(result, 0, "Dispatch for KYBD should succeed");
    
    result = giotbl_get_dispatch(10, &dsp, &ini, &trm);
    TEST_ASSERT_EQ_INT(result, 0, "Dispatch for COM4 should succeed");
    
    result = giotbl_get_dispatch(-1, &dsp, &ini, &trm);
    TEST_ASSERT_EQ_INT(result, -1, "Invalid device should fail");
    
    result = giotbl_get_dispatch(100, &dsp, &ini, &trm);
    TEST_ASSERT_EQ_INT(result, -1, "Out of bounds device should fail");
    
    TEST_PASS("giotbl_get_dispatch");
}

/* Test giotbl_get_name_table */
void test_giotbl_get_name_table(void) {
    const device_name_t *table = giotbl_get_name_table();
    TEST_ASSERT(table != NULL, "Name table should not be NULL");
    
    TEST_ASSERT_EQ_STR(table[0].name, "KYBD", "First entry should be KYBD");
    TEST_ASSERT_EQ_INT(table[0].device_id, 0, "KYBD id should be 0");
    
    TEST_ASSERT_EQ_STR(table[1].name, "SCRN", "Second entry should be SCRN");
    TEST_ASSERT_EQ_INT(table[1].device_id, 1, "SCRN id should be 1");
    
    TEST_ASSERT_EQ_STR(table[10].name, "COM4", "11th entry should be COM4");
    TEST_ASSERT_EQ_INT(table[10].device_id, 10, "COM4 id should be 10");
    
    TEST_ASSERT(table[11].name == NULL, "Table should end with NULL");
    
    TEST_PASS("giotbl_get_name_table");
}

/* Test giotbl_get_dispatch_table */
void test_giotbl_get_dispatch_table(void) {
    const device_dispatch_t *table = giotbl_get_dispatch_table();
    TEST_ASSERT(table != NULL, "Dispatch table should not be NULL");
    
    TEST_ASSERT_EQ_STR(table[0].name, "KYBD", "First dispatch should be KYBD");
    TEST_ASSERT_EQ_STR(table[1].name, "SCRN", "Second dispatch should be SCRN");
    TEST_ASSERT_EQ_STR(table[10].name, "COM4", "11th dispatch should be COM4");
    TEST_ASSERT(table[11].name == NULL, "Table should end with NULL");
    
    TEST_PASS("giotbl_get_dispatch_table");
}

/* Test giotbl_find_device */
void test_giotbl_find_device(void) {
    int id = giotbl_find_device("KYBD");
    TEST_ASSERT_EQ_INT(id, 0, "Find KYBD should return 0");
    
    id = giotbl_find_device("SCRN");
    TEST_ASSERT_EQ_INT(id, 1, "Find SCRN should return 1");
    
    id = giotbl_find_device("COM4");
    TEST_ASSERT_EQ_INT(id, 10, "Find COM4 should return 10");
    
    id = giotbl_find_device("UNKNOWN");
    TEST_ASSERT_EQ_INT(id, -1, "Find UNKNOWN should return -1");
    
    TEST_PASS("giotbl_find_device");
}

/* Test giotbl_init_all */
void test_giotbl_init_all(void) {
    giotbl_init_all();
    TEST_PASS("giotbl_init_all");
}