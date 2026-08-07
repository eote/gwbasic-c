/* TEST_BISTRS - Unit tests for BISTRS (String Functions) */
/* Based on BISTRS.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

static void setup(void) {
    gwdata_init();
    gwram_init();
    gwinit_init();
    bistrs_init();
}

/* Test LEFT$ basic */
void test_bistrs_left_basic(void) {
    setup();
    char *result = bistrs_left("Hello World", 5);
    TEST_ASSERT(result != NULL, "LEFT$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "Hello", "LEFT$ 5 chars");
    }
    TEST_PASS("LEFT$ basic");
}

/* Test LEFT$ more than string length */
void test_bistrs_left_overflow(void) {
    setup();
    char *result = bistrs_left("Hi", 10);
    TEST_ASSERT(result != NULL, "LEFT$ overflow result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "Hi", "LEFT$ overflow returns full string");
    }
    TEST_PASS("LEFT$ overflow");
}

/* Test LEFT$ with zero */
void test_bistrs_left_zero(void) {
    setup();
    char *result = bistrs_left("Hello", 0);
    TEST_ASSERT(result != NULL, "LEFT$ zero result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "LEFT$ 0 chars = empty");
    }
    TEST_PASS("LEFT$ zero");
}

/* Test LEFT$ NULL input */
void test_bistrs_left_null(void) {
    setup();
    char *result = bistrs_left(NULL, 5);
    TEST_ASSERT(result != NULL, "LEFT$ NULL returns non-NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "LEFT$ NULL = empty");
    }
    TEST_PASS("LEFT$ NULL input");
}

/* Test RIGHT$ basic */
void test_bistrs_right_basic(void) {
    setup();
    char *result = bistrs_right("Hello World", 5);
    TEST_ASSERT(result != NULL, "RIGHT$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "World", "RIGHT$ 5 chars");
    }
    TEST_PASS("RIGHT$ basic");
}

/* Test RIGHT$ more than string length */
void test_bistrs_right_overflow(void) {
    setup();
    char *result = bistrs_right("Hi", 10);
    TEST_ASSERT(result != NULL, "RIGHT$ overflow result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "Hi", "RIGHT$ overflow returns full string");
    }
    TEST_PASS("RIGHT$ overflow");
}

/* Test RIGHT$ zero */
void test_bistrs_right_zero(void) {
    setup();
    char *result = bistrs_right("Hello", 0);
    TEST_ASSERT(result != NULL, "RIGHT$ zero result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "RIGHT$ 0 chars = empty");
    }
    TEST_PASS("RIGHT$ zero");
}

/* Test MID$ basic */
void test_bistrs_mid_basic(void) {
    setup();
    char *result = bistrs_mid("Hello World", 1, 5);
    TEST_ASSERT(result != NULL, "MID$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "Hello", "MID$ start=1 len=5");
    }
    TEST_PASS("MID$ basic");
}

/* Test MID$ middle */
void test_bistrs_mid_middle(void) {
    setup();
    char *result = bistrs_mid("Hello World", 7, 5);
    TEST_ASSERT(result != NULL, "MID$ middle result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "World", "MID$ start=7 len=5");
    }
    TEST_PASS("MID$ middle");
}

/* Test MID$ beyond string end */
void test_bistrs_mid_beyond(void) {
    setup();
    char *result = bistrs_mid("Hello", 3, 10);
    TEST_ASSERT(result != NULL, "MID$ beyond result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "llo", "MID$ beyond returns remaining chars");
    }
    TEST_PASS("MID$ beyond string end");
}

/* Test MID$ start past end */
void test_bistrs_mid_past_end(void) {
    setup();
    char *result = bistrs_mid("Hello", 10, 3);
    TEST_ASSERT(result != NULL, "MID$ past end result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "MID$ past end = empty");
    }
    TEST_PASS("MID$ start past end");
}

/* Test LEN basic */
void test_bistrs_len_basic(void) {
    setup();
    int len = bistrs_len("Hello");
    TEST_ASSERT_EQ_INT(len, 5, "LEN('Hello') = 5");
    TEST_PASS("LEN basic");
}

/* Test LEN empty */
void test_bistrs_len_empty(void) {
    setup();
    int len = bistrs_len("");
    TEST_ASSERT_EQ_INT(len, 0, "LEN('') = 0");
    TEST_PASS("LEN empty");
}

/* Test LEN NULL */
void test_bistrs_len_null(void) {
    setup();
    int len = bistrs_len(NULL);
    TEST_ASSERT_EQ_INT(len, 0, "LEN(NULL) = 0");
    TEST_PASS("LEN NULL");
}

/* Test ASC basic */
void test_bistrs_asc_basic(void) {
    setup();
    int code = bistrs_asc("A");
    TEST_ASSERT_EQ_INT(code, 65, "ASC('A') = 65");
    TEST_PASS("ASC basic");
}

/* Test ASC lowercase */
void test_bistrs_asc_lower(void) {
    setup();
    int code = bistrs_asc("a");
    TEST_ASSERT_EQ_INT(code, 97, "ASC('a') = 97");
    TEST_PASS("ASC lowercase");
}

/* Test ASC empty */
void test_bistrs_asc_empty(void) {
    setup();
    int code = bistrs_asc("");
    TEST_ASSERT_EQ_INT(code, 0, "ASC('') = 0");
    TEST_PASS("ASC empty");
}

/* Test CHR$ basic */
void test_bistrs_chr_basic(void) {
    setup();
    char *result = bistrs_chr(65);
    TEST_ASSERT(result != NULL, "CHR$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "A", "CHR$(65) = 'A'");
    }
    TEST_PASS("CHR$ basic");
}

/* Test CHR$ special */
void test_bistrs_chr_special(void) {
    setup();
    char *result = bistrs_chr(48);
    TEST_ASSERT(result != NULL, "CHR$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "0", "CHR$(48) = '0'");
    }
    TEST_PASS("CHR$ special char");
}

/* Test STR$ integer */
void test_bistrs_str_int(void) {
    setup();
    char *result = bistrs_str(42.0);
    TEST_ASSERT(result != NULL, "STR$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "42", "STR$(42) = '42'");
    }
    TEST_PASS("STR$ integer");
}

/* Test STR$ float */
void test_bistrs_str_float(void) {
    setup();
    char *result = bistrs_str(3.14);
    TEST_ASSERT(result != NULL, "STR$ float result not NULL");
    /* %g format, so 3.14 */
    TEST_PASS("STR$ float");
}

/* Test STR$ negative */
void test_bistrs_str_negative(void) {
    setup();
    char *result = bistrs_str(-5.0);
    TEST_ASSERT(result != NULL, "STR$ negative result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "-5", "STR$(-5) = '-5'");
    }
    TEST_PASS("STR$ negative");
}

/* Test VAL basic */
void test_bistrs_val_basic(void) {
    setup();
    double val = bistrs_val("123");
    TEST_ASSERT_EQ_INT((int)val, 123, "VAL('123') = 123");
    TEST_PASS("VAL basic");
}

/* Test VAL float */
void test_bistrs_val_float(void) {
    setup();
    double val = bistrs_val("3.14");
    TEST_ASSERT(val > 3.13 && val < 3.15, "VAL('3.14') = 3.14");
    TEST_PASS("VAL float");
}

/* Test VAL invalid */
void test_bistrs_val_invalid(void) {
    setup();
    double val = bistrs_val("abc");
    TEST_ASSERT_EQ_INT((int)val, 0, "VAL('abc') = 0");
    TEST_PASS("VAL invalid");
}

/* Test STRING$ basic */
void test_bistrs_string_basic(void) {
    setup();
    char *result = bistrs_string(5, '*');
    TEST_ASSERT(result != NULL, "STRING$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "*****", "STRING$(5, '*') = '*****'");
    }
    TEST_PASS("STRING$ basic");
}

/* Test STRING$ zero */
void test_bistrs_string_zero(void) {
    setup();
    char *result = bistrs_string(0, '*');
    TEST_ASSERT(result != NULL, "STRING$ zero result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "STRING$(0) = empty");
    }
    TEST_PASS("STRING$ zero");
}

/* Test SPACE$ basic */
void test_bistrs_space_basic(void) {
    setup();
    char *result = bistrs_space(5);
    TEST_ASSERT(result != NULL, "SPACE$ result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "     ", "SPACE$(5) = 5 spaces");
    }
    TEST_PASS("SPACE$ basic");
}

/* Test SPACE$ zero */
void test_bistrs_space_zero(void) {
    setup();
    char *result = bistrs_space(0);
    TEST_ASSERT(result != NULL, "SPACE$ zero result not NULL");
    if (result) {
        TEST_ASSERT_EQ_STR(result, "", "SPACE$(0) = empty");
    }
    TEST_PASS("SPACE$ zero");
}
