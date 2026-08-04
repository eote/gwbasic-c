/* TEST_GWEVAL_HELPERS - Unit tests for GWEVAL helper functions */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test gweval_scan_var - basic variable scanning */
void test_gweval_scan_var_basic(void) {
    char text[] = "VARIABLE";
    char *p = text;
    int var_ptr = 0;
    
    gweval_scan_var(&p, &var_ptr);
    
    /* Should advance pointer and set var_ptr */
    TEST_PASS("gweval_scan_var basic");
}

/* Test gweval_scan_var - with array reference */
void test_gweval_scan_var_array(void) {
    char text[] = "ARRAY(5)";
    char *p = text;
    int var_ptr = 0;
    
    gweval_scan_var(&p, &var_ptr);
    
    TEST_PASS("gweval_scan_var array reference");
}

/* Test gweval_scan_var - empty */
void test_gweval_scan_var_empty(void) {
    char text[] = "";
    char *p = text;
    int var_ptr = 0;
    
    gweval_scan_var(&p, &var_ptr);
    
    TEST_PASS("gweval_scan_var empty");
}

/* Test gweval_frqint - returns 0 as placeholder */
void test_gweval_frqint_basic(void) {
    int result = gweval_frqint();
    
    TEST_ASSERT_EQ_INT(result, 0, "gweval_frqint returns 0");
    TEST_PASS("gweval_frqint basic");
}

/* Test gweval_chrgt2 - advances pointer */
void test_gweval_chrgt2_basic(void) {
    char text[] = "ABC";
    char *p = text;
    
    gweval_chrgt2(&p);
    
    TEST_ASSERT(*p == 'B', "chrgt2 advances pointer");
    TEST_PASS("gweval_chrgt2 basic");
}

/* Test gweval_chrgt2 - empty string */
void test_gweval_chrgt2_empty(void) {
    char text[] = "";
    char *p = text;
    
    gweval_chrgt2(&p);
    
    TEST_ASSERT(*p == '\0', "chrgt2 handles empty");
    TEST_PASS("gweval_chrgt2 empty");
}

/* Test gweval_synchr - matches expected char */
void test_gweval_synchr_match(void) {
    char text[] = "(EXPR)";
    char *p = text;
    
    gweval_synchr(&p, '(');
    
    TEST_ASSERT(*p == 'E', "synchr matches and advances");
    TEST_PASS("gweval_synchr match");
}

/* Test gweval_synchr - no match */
void test_gweval_synchr_no_match(void) {
    char text[] = "EXPR)";
    char *p = text;
    
    gweval_synchr(&p, '(');
    
    TEST_ASSERT(*p == 'E', "synchr doesn't advance on mismatch");
    TEST_PASS("gweval_synchr no match");
}

/* Test gweval_synchr - end of string */
void test_gweval_synchr_eos(void) {
    char text[] = "";
    char *p = text;
    
    gweval_synchr(&p, '(');
    
    TEST_ASSERT(*p == '\0', "synchr handles EOS");
    TEST_PASS("gweval_synchr end of string");
}

/* Test gweval_synchr - null pointer */
void test_gweval_synchr_null(void) {
    char *p = NULL;
    
    gweval_synchr(&p, '(');
    
    TEST_PASS("gweval_synchr null pointer");
}