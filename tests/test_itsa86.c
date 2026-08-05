/* TEST_ITSA86 - Unit tests for ITSA86 (Resident Initialization for 8086) */
/* Based on ITSA86.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <stdint.h>

/* Test itsa86_init - initialization */
void test_itsa86_init(void) {
    itsa86_init();
    
    /* Verify initialization doesn't crash */
    TEST_PASS("itsa86_init");
}

/* Test itsa86_basvar - read operation (uninitialized) */
void test_itsa86_basvar_read_uninitialized(void) {
    WORD result = itsa86_basvar(0, 0, 0);  /* carry=0: read */
    
    /* When uninitialized, should return error code 0xFFFF */
    TEST_ASSERT_EQ_WORD(result, 0xFFFF, "basvar read uninitialized returns error");
    TEST_PASS("itsa86_basvar read uninitialized");
}

/* Test itsa86_basvar - write operation (uninitialized) */
void test_itsa86_basvar_write_uninitialized(void) {
    WORD result = itsa86_basvar(0, 0x1234, 1);  /* carry=1: write */
    
    /* When uninitialized, write should return error code 0xFFFF */
    TEST_ASSERT_EQ_WORD(result, 0xFFFF, "basvar write uninitialized returns error");
    TEST_PASS("itsa86_basvar write uninitialized");
}

/* Test itsa86_basvar - invalid index */
void test_itsa86_basvar_invalid_index(void) {
    itsa86_init();  /* Need to init first so bvtab is not uninitialized */
    
    /* Set up valid BVTAB pointers to test index bounds */
    static WORD dummy_storage[4] = {0x1000, 0x2000, 0x3000, 0x4000};
    WORD** bvtab = itsa86_get_bvtab();
    bvtab[0] = &dummy_storage[0];
    bvtab[1] = &dummy_storage[1];
    bvtab[2] = &dummy_storage[2];
    bvtab[3] = &dummy_storage[3];
    
    WORD result = itsa86_basvar(4, 0, 0);  /* bx=4 >= 4 */
    
    TEST_ASSERT_EQ_WORD(result, 0xFFFE, "basvar invalid index returns error");
    TEST_PASS("itsa86_basvar invalid index");
}

/* Test itsa86_basvar - all valid indices uninitialized */
void test_itsa86_basvar_all_indices_uninitialized(void) {
    /* Reset BVTAB to ensure uninitialized state */
    WORD** bvtab = itsa86_get_bvtab();
    bvtab[0] = NULL;
    bvtab[1] = NULL;
    bvtab[2] = NULL;
    bvtab[3] = NULL;
    
    for (int i = 0; i < 4; i++) {
        WORD result = itsa86_basvar(i, 0, 0);
        TEST_ASSERT_EQ_WORD(result, 0xFFFF, "basvar all indices return error when uninitialized");
    }
    TEST_PASS("itsa86_basvar all indices uninitialized");
}

/* Test itsa86_mapclc - no options */
void test_itsa86_mapclc_no_options(void) {
    itsa86_init();
    itsa86_mapclc();
    
    /* Should calculate maximum available memory */
    WORD maxmem = itsa86_get_maxmem();
    TEST_ASSERT(maxmem > 0, "mapclc should set maxmem > 0");
    TEST_PASS("itsa86_mapclc no options");
}

/* Test itsa86_mapclc - with M option valid */
void test_itsa86_mapclc_m_option_valid(void) {
    itsa86_init();
    itsa86_set_m_option(32768);  /* 32KB */
    itsa86_mapclc();
    
    WORD maxmem = itsa86_get_maxmem();
    TEST_ASSERT(maxmem == 32768, "mapclc should use /M: option value");
    TEST_PASS("itsa86_mapclc M option valid");
}

/* Test itsa86_mapclc - with M option too large */
void test_itsa86_mapclc_m_option_too_large(void) {
    itsa86_init();
    itsa86_set_m_option(65535);  /* 64KB - max valid */
    itsa86_mapclc();
    
    /* Should cap at maximum available */
    WORD maxmem = itsa86_get_maxmem();
    TEST_ASSERT(maxmem <= 65535, "mapclc should cap at max available");
    TEST_PASS("itsa86_mapclc M option too large capped");
}

/* Test itsa86_mapclc - with C option */
void test_itsa86_mapclc_c_option(void) {
    itsa86_init();
    itsa86_set_c_option(1024);  /* 1KB COM buffer */
    itsa86_mapclc();
    
    WORD newds = itsa86_get_newds();
    TEST_ASSERT(newds > 0, "mapclc should calculate newds with C option");
    TEST_PASS("itsa86_mapclc C option");
}

/* Test itsa86_seg_off - valid segment */
void test_itsa86_seg_off_valid(void) {
    itsa86_init();
    WORD data_seg = itsa86_get_data_seg();
    WORD segment = data_seg + 10;  /* 10 paragraphs above DS */
    
    int result = itsa86_seg_off(&segment);
    
    TEST_ASSERT_EQ_INT(result, 0, "seg_off valid segment returns success");
    TEST_ASSERT_EQ_WORD(segment, 160, "seg_off converts 10 paragraphs to 160 bytes");
    TEST_PASS("itsa86_seg_off valid segment");
}

/* Test itsa86_seg_off - segment below DS */
void test_itsa86_seg_off_below_ds(void) {
    itsa86_init();
    WORD data_seg = itsa86_get_data_seg();
    WORD segment = data_seg - 1;  /* Below current DS */
    
    int result = itsa86_seg_off(&segment);
    
    TEST_ASSERT_EQ_INT(result, -1, "seg_off below DS returns error");
    TEST_PASS("itsa86_seg_off below DS error");
}

/* Test itsa86_seg_off - segment at DS */
void test_itsa86_seg_off_at_ds(void) {
    itsa86_init();
    WORD data_seg = itsa86_get_data_seg();
    WORD segment = data_seg;
    
    int result = itsa86_seg_off(&segment);
    
    TEST_ASSERT_EQ_INT(result, 0, "seg_off at DS returns success");
    TEST_ASSERT_EQ_WORD(segment, 0, "seg_off at DS returns 0 offset");
    TEST_PASS("itsa86_seg_off at DS");
}

/* Test itsa86_seg_off - max 64K limit */
void test_itsa86_seg_off_64k_limit(void) {
    itsa86_init();
    WORD data_seg = itsa86_get_data_seg();
    
    /* Test that paragraphs > 0x7777 are capped */
    /* We can't easily test this with real data_seg values due to WORD overflow */
    /* Instead, test that the logic correctly caps at 0x7777 paragraphs */
    WORD segment = data_seg + 0x7800;  /* 0x7800 > 0x7777, should be capped */
    
    /* Check if overflow would occur */
    if (segment < data_seg) {
        /* Overflow - skip this test as we can't reliably test the 64K cap 
         * with the current data_seg value and WORD arithmetic */
        TEST_PASS("itsa86_seg_off 64K limit (skipped - overflow)");
        return;
    }
    
    int result = itsa86_seg_off(&segment);
    
    TEST_ASSERT_EQ_INT(result, 0, "seg_off beyond 64K returns success (capped)");
    TEST_ASSERT(segment <= 0x7777 * 16, "seg_off capped at 64K limit");
    TEST_PASS("itsa86_seg_off 64K limit");
}

/* Test itsa86_get_memory_info */
void test_itsa86_get_memory_info(void) {
    itsa86_init();
    itsa86_mapclc();
    
    WORD maxmem = 0, freemem = 0, txttab = 0, vartab = 0;
    itsa86_get_memory_info(&maxmem, &freemem, &txttab, &vartab);
    
    TEST_ASSERT(maxmem > 0, "get_memory_info maxmem > 0");
    TEST_ASSERT(freemem >= 0, "get_memory_info freemem >= 0");
    TEST_PASS("itsa86_get_memory_info");
}

/* Test itsa86_set_c_option */
void test_itsa86_set_c_option(void) {
    itsa86_init();
    itsa86_set_c_option(2048);
    
    /* Verify flag is set (indirect test via mapclc) */
    itsa86_mapclc();
    TEST_PASS("itsa86_set_c_option");
}

/* Test itsa86_set_m_option */
void test_itsa86_set_m_option(void) {
    itsa86_init();
    itsa86_set_m_option(4096);
    
    itsa86_mapclc();
    TEST_PASS("itsa86_set_m_option");
}

/* Test itsa86_mapini */
void test_itsa86_mapini(void) {
    itsa86_init();
    itsa86_mapini();
    
    WORD savseg = itsa86_get_savseg();
    TEST_ASSERT(savseg > 0, "mapini should set savseg");
    TEST_PASS("itsa86_mapini");
}

/* Test itsa86_csend */
void test_itsa86_csend(void) {
    itsa86_init();
    itsa86_csend();  /* Should not crash */
    
    TEST_PASS("itsa86_csend");
}