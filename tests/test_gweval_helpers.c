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

/* Test gweval_chrgt2 - advances pointer only for chars < ':' (per ASM) */
void test_gweval_chrgt2_basic(void) {
    /* Test with char < ':' (space) - should skip spaces */
    char text1[] = "  ABC";
    char *p1 = text1;
    gweval_chrgt2(&p1);
    TEST_ASSERT(*p1 == 'A', "chrgt2 skips spaces for char < ':'");
    
    /* Test with char >= ':' ('A') - should NOT advance */
    char text2[] = "ABC";
    char *p2 = text2;
    gweval_chrgt2(&p2);
    TEST_ASSERT(*p2 == 'A', "chrgt2 does not advance for char >= ':'");
    
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

/* Test gweval_synchr - matches expected char (no whitespace skip, per ASM) */
void test_gweval_synchr_match(void) {
    /* Test with '(' which is < ':' - should skip spaces after match */
    const char text1[] = "( EXPR)";
    const char *p1 = text1;
    
    gweval_synchr(&p1, '(');
    
    /* After matching '(', should advance and skip spaces */
    TEST_ASSERT(*p1 == 'E', "synchr matches, advances, and skips spaces for char < ':'");
    TEST_PASS("gweval_synchr match with space skip");
}

/* Test gweval_synchr - no match sets syntax error */
void test_gweval_synchr_no_match(void) {
    const char text[] = "EXPR)";
    const char *p = text;
    int error_before = gwdata_get_error();
    
    gweval_synchr(&p, '(');
    
    int error_after = gwdata_get_error();
    TEST_ASSERT(error_after == 2, "synchr mismatch sets error 2 (Syntax Error)");
    TEST_ASSERT(*p == 'E', "synchr leaves pointer at mismatch");
    
    /* Reset error for next test */
    gwdata_clear_error();
    TEST_PASS("gweval_synchr no match sets error");
}

/* Test gweval_synchr - end of string */
void test_gweval_synchr_eos(void) {
    const char text[] = "";
    const char *p = text;
    
    gweval_synchr(&p, '(');
    
    TEST_ASSERT(*p == '\0', "synchr handles EOS");
    gwdata_clear_error();
    TEST_PASS("gweval_synchr end of string");
}

/* Test gweval_synchr - null pointer */
void test_gweval_synchr_null(void) {
    const char *p = NULL;
    
    gweval_synchr(&p, '(');
    
    TEST_PASS("gweval_synchr null pointer");
}

/* Test gweval_synchr - operator '=': ASM checks NEXT char after match.
 * For "= 42", next char is space (0x20) < ':', so it skips spaces. */
void test_gweval_synchr_operator_eq(void) {
    const char text[] = "= 42";
    const char *p = text;
    
    gweval_synchr(&p, '=');
    
    TEST_ASSERT(*p == '4', "synchr with '=' skips spaces when next char is space");
    TEST_PASS("gweval_synchr operator = skips spaces (ASM behavior)");
}
