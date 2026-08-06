/* TEST_OEMEV - Unit tests for OEMEV (Event Handling) */
/* Based on OEMEV.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test oemev_init */
void test_oemev_init(void) {
    oemev_init();
    TEST_PASS("oemev_init");
}

/* Test oemev_evsta and oemev_evtrm */
void test_oemev_evsta_evtrm(void) {
    oemev_init();
    oemev_evsta();
    oemev_evtrm();
    TEST_PASS("oemev_evsta_evtrm");
}

/* Test oemev_pollev */
void test_oemev_pollev(void) {
    oemev_init();
    BOOL result = oemev_pollev();
    TEST_ASSERT(result == FALSE, "pollev returns FALSE when no event");
    TEST_PASS("oemev_pollev");
}

/* Test oemev_rdpen - PEN ON */
void test_oemev_rdpen_on(void) {
    oemev_init();
    int result;
    int res = oemev_rdpen(0xFE, &result);  /* PEN ON */
    TEST_ASSERT(res == 0, "PEN ON succeeds");
    TEST_PASS("oemev_rdpen_on");
}

/* Test oemev_rdpen - PEN OFF */
void test_oemev_rdpen_off(void) {
    oemev_init();
    int result;
    int res = oemev_rdpen(0xFF, &result);  /* PEN OFF */
    TEST_ASSERT(res == 0, "PEN OFF succeeds");
    TEST_PASS("oemev_rdpen_off");
}

/* Test oemev_rdpen - PEN status functions */
void test_oemev_rdpen_status(void) {
    oemev_init();
    int result;
    
    /* Enable pen first */
    oemev_rdpen(0xFE, &result);
    
    int res = oemev_rdpen(0, &result);  /* PEN(0) - pen down since last poll */
    TEST_ASSERT(res == 0, "PEN(0) succeeds");
    TEST_ASSERT(result == 0, "PEN(0) returns 0");
    
    res = oemev_rdpen(1, &result);  /* PEN(1) - X when activated */
    TEST_ASSERT(res == 0, "PEN(1) succeeds");
    TEST_ASSERT(result == 0, "PEN(1) returns 0");
    
    res = oemev_rdpen(2, &result);  /* PEN(2) - Y when activated */
    TEST_ASSERT(res == 0, "PEN(2) succeeds");
    TEST_ASSERT(result == 0, "PEN(2) returns 0");
    
    res = oemev_rdpen(3, &result);  /* PEN(3) - current switch state */
    TEST_ASSERT(res == 0, "PEN(3) succeeds");
    TEST_ASSERT(result == 0, "PEN(3) returns 0");
    
    res = oemev_rdpen(4, &result);  /* PEN(4) - last valid X */
    TEST_ASSERT(res == 0, "PEN(4) succeeds");
    TEST_ASSERT(result == 0, "PEN(4) returns 0");
    
    res = oemev_rdpen(5, &result);  /* PEN(5) - last valid Y */
    TEST_ASSERT(res == 0, "PEN(5) succeeds");
    TEST_ASSERT(result == 0, "PEN(5) returns 0");
    
    res = oemev_rdpen(6, &result);  /* PEN(6) - row - 1 */
    TEST_ASSERT(res == 0, "PEN(6) succeeds");
    TEST_ASSERT(result == -1, "PEN(6) returns -1");
    
    res = oemev_rdpen(7, &result);  /* PEN(7) - col - 1 */
    TEST_ASSERT(res == 0, "PEN(7) succeeds");
    TEST_ASSERT(result == -1, "PEN(7) returns -1");
    
    res = oemev_rdpen(8, &result);  /* PEN(8) - row - 1 */
    TEST_ASSERT(res == 0, "PEN(8) succeeds");
    TEST_ASSERT(result == -1, "PEN(8) returns -1");
    
    res = oemev_rdpen(9, &result);  /* PEN(9) - col - 1 */
    TEST_ASSERT(res == 0, "PEN(9) succeeds");
    TEST_ASSERT(result == -1, "PEN(9) returns -1");
    
    /* Disable pen */
    oemev_rdpen(0xFF, &result);
    
    /* Try to read when disabled */
    res = oemev_rdpen(0, &result);
    TEST_ASSERT(res == -1, "PEN returns -1 when disabled");
    
    TEST_PASS("oemev_rdpen_status");
}

/* Test oemev_rdpen - invalid function */
void test_oemev_rdpen_invalid(void) {
    oemev_init();
    int result;
    int res = oemev_rdpen(10, &result);  /* Invalid function > 9 */
    TEST_ASSERT(res == -1, "Invalid function returns -1");
    TEST_PASS("oemev_rdpen_invalid");
}

/* Test oemev_rdtrig - current status */
void test_oemev_rdtrig_current(void) {
    oemev_init();
    int result;
    
    int res = oemev_rdtrig(0, 0, &result);  /* Button 0, current status */
    TEST_ASSERT(res == 0, "RDTRIG button 0 succeeds");
    TEST_ASSERT(result == 0 || result == 1, "Result is 0 or 1");
    
    res = oemev_rdtrig(1, 0, &result);  /* Button 1 */
    TEST_ASSERT(res == 0, "RDTRIG button 1 succeeds");
    TEST_ASSERT(result == 0 || result == 1, "Result is 0 or 1");
    
    res = oemev_rdtrig(2, 0, &result);  /* Button 2 */
    TEST_ASSERT(res == 0, "RDTRIG button 2 succeeds");
    TEST_ASSERT(result == 0 || result == 1, "Result is 0 or 1");
    
    res = oemev_rdtrig(3, 0, &result);  /* Button 3 */
    TEST_ASSERT(res == 0, "RDTRIG button 3 succeeds");
    TEST_ASSERT(result == 0 || result == 1, "Result is 0 or 1");
    
    TEST_PASS("oemev_rdtrig_current");
}

/* Test oemev_rdtrig - latched status */
void test_oemev_rdtrig_latched(void) {
    oemev_init();
    int result;
    
    int res = oemev_rdtrig(0, 1, &result);  /* Button 0, latched */
    TEST_ASSERT(res == 0, "RDTRIG latched button 0 succeeds");
    TEST_ASSERT(result == 0 || result == 1, "Result is 0 or 1");
    
    res = oemev_rdtrig(1, 1, &result);  /* Button 1 */
    TEST_ASSERT(res == 0, "RDTRIG latched button 1 succeeds");
    
    res = oemev_rdtrig(2, 1, &result);  /* Button 2 */
    TEST_ASSERT(res == 0, "RDTRIG latched button 2 succeeds");
    
    res = oemev_rdtrig(3, 1, &result);  /* Button 3 */
    TEST_ASSERT(res == 0, "RDTRIG latched button 3 succeeds");
    
    TEST_PASS("oemev_rdtrig_latched");
}

/* Test oemev_rdtrig - invalid button */
void test_oemev_rdtrig_invalid(void) {
    oemev_init();
    int result;
    
    int res = oemev_rdtrig(4, 0, &result);  /* Invalid button > 3 */
    TEST_ASSERT(res == -1, "Invalid button returns -1");
    TEST_ASSERT(result == 0, "Result is 0 for invalid");
    
    TEST_PASS("oemev_rdtrig_invalid");
}

/* Test oemev_rdstik - STICK function */
void test_oemev_rdstik(void) {
    oemev_init();
    int result;
    
    int res = oemev_rdstik(0, &result);  /* STICK(0) - X of A */
    TEST_ASSERT(res == 0, "STICK(0) succeeds");
    TEST_ASSERT(result >= 0, "Result >= 0");
    
    res = oemev_rdstik(1, &result);  /* STICK(1) - Y of A */
    TEST_ASSERT(res == 0, "STICK(1) succeeds");
    TEST_ASSERT(result >= 0, "Result >= 0");
    
    res = oemev_rdstik(2, &result);  /* STICK(2) - X of B */
    TEST_ASSERT(res == 0, "STICK(2) succeeds");
    TEST_ASSERT(result >= 0, "Result >= 0");
    
    res = oemev_rdstik(3, &result);  /* STICK(3) - Y of B */
    TEST_ASSERT(res == 0, "STICK(3) succeeds");
    TEST_ASSERT(result >= 0, "Result >= 0");
    
    TEST_PASS("oemev_rdstik");
}

/* Test oemev_rdstik - invalid coord */
void test_oemev_rdstik_invalid(void) {
    oemev_init();
    int result;
    
    int res = oemev_rdstik(4, &result);  /* Invalid coord > 3 */
    TEST_ASSERT(res == -1, "Invalid coord returns -1");
    
    TEST_PASS("oemev_rdstik_invalid");
}

/* Test oemev_evtrm cleanup */
void test_oemev_evtrm_cleanup(void) {
    oemev_init();
    oemev_evsta();
    oemev_evtrm();
    oemev_evtrm();  /* Double cleanup should be safe */
    TEST_PASS("oemev_evtrm_cleanup");
}