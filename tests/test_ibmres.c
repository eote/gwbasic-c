/* TEST_IBMRES - Unit tests for IBMRES (IBM Compatible Reserved Words) */
/* Based on IBMRES.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Token types (mirrored from ibmres.c) */
#define TK_STMT 0x00    /* 1-byte statement token */
#define TK_FUNC 0x80    /* 1-byte function token (high bit set) */
#define TK_STMT_EXT 0xFE /* 2-byte extended statement token (FE xx) */
#define TK_FUNC_EXT 0xFD /* 2-byte extended function token (FD xx) */

/* Keyword entry structure (mirrored from ibmres.c) */
struct keyword_entry {
    const char *name;
    WORD token;
    BYTE type;      /* TK_STMT, TK_FUNC, TK_STMT_EXT, TK_FUNC_EXT */
    BYTE category;  /* Statement, function, operator, etc. */
};

/* Test ibmres_init */
void test_ibmres_init(void) {
    ibmres_init();
    TEST_PASS("ibmres_init");
}

/* Test ibmres_lookup_keyword - core statement */
void test_ibmres_lookup_statement(void) {
    const struct keyword_entry *kw = ibmres_lookup_keyword("FOR");
    TEST_ASSERT(kw != NULL, "FOR found");
    TEST_ASSERT_EQ_STR(kw->name, "FOR", "FOR name");
    TEST_ASSERT_EQ_INT(kw->token, 0x01, "FOR token");
    TEST_ASSERT_EQ_INT(kw->type, TK_STMT, "FOR type");
    
    kw = ibmres_lookup_keyword("PRINT");
    TEST_ASSERT(kw != NULL, "PRINT found");
    TEST_ASSERT_EQ_INT(kw->token, 0x0F, "PRINT token");
    
    kw = ibmres_lookup_keyword("IF");
    TEST_ASSERT(kw != NULL, "IF found");
    TEST_ASSERT_EQ_INT(kw->token, 0x09, "IF token");
    
    TEST_PASS("ibmres_lookup_statement");
}

/* Test ibmres_lookup_keyword - core function */
void test_ibmres_lookup_function(void) {
    const struct keyword_entry *kw = ibmres_lookup_keyword("LEFT$");
    TEST_ASSERT(kw != NULL, "LEFT$ found");
    TEST_ASSERT_EQ_INT(kw->token, 0x80, "LEFT$ token");
    TEST_ASSERT_EQ_INT(kw->type, TK_FUNC, "LEFT$ type");
    
    kw = ibmres_lookup_keyword("SIN");
    TEST_ASSERT(kw != NULL, "SIN found");
    TEST_ASSERT_EQ_INT(kw->token, 0x88, "SIN token");
    
    kw = ibmres_lookup_keyword("RND");
    TEST_ASSERT(kw != NULL, "RND found");
    TEST_ASSERT_EQ_INT(kw->token, 0x87, "RND token");
    
    TEST_PASS("ibmres_lookup_function");
}

/* Test ibmres_lookup_keyword - extended statement */
void test_ibmres_lookup_extended_stmt(void) {
    const struct keyword_entry *kw = ibmres_lookup_keyword("FILES");
    TEST_ASSERT(kw != NULL, "FILES found");
    TEST_ASSERT_EQ_INT(kw->token, 0xFE00, "FILES token");
    TEST_ASSERT_EQ_INT(kw->type, TK_STMT_EXT, "FILES type");
    
    kw = ibmres_lookup_keyword("PAINT");
    TEST_ASSERT(kw != NULL, "PAINT found");
    TEST_ASSERT_EQ_INT(kw->token, 0xFE0D, "PAINT token");
    
    kw = ibmres_lookup_keyword("CIRCLE");
    TEST_ASSERT(kw != NULL, "CIRCLE found");
    TEST_ASSERT_EQ_INT(kw->token, 0xFE0F, "CIRCLE token");
    
    TEST_PASS("ibmres_lookup_extended_stmt");
}

/* Test ibmres_lookup_keyword - extended function */
void test_ibmres_lookup_extended_func(void) {
    const struct keyword_entry *kw = ibmres_lookup_keyword("CVI");
    TEST_ASSERT(kw != NULL, "CVI found");
    TEST_ASSERT_EQ_INT(kw->token, 0xFD00, "CVI token");
    TEST_ASSERT_EQ_INT(kw->type, TK_FUNC_EXT, "CVI type");
    
    kw = ibmres_lookup_keyword("KTN");
    TEST_ASSERT(kw != NULL, "KTN found");
    TEST_ASSERT_EQ_INT(kw->token, 0xFD06, "KTN token");
    
    TEST_PASS("ibmres_lookup_extended_func");
}

/* Test ibmres_lookup_keyword - case insensitive */
void test_ibmres_lookup_case_insensitive(void) {
    const struct keyword_entry *kw;
    
    kw = ibmres_lookup_keyword("for");
    TEST_ASSERT(kw != NULL, "lowercase for found");
    TEST_ASSERT_EQ_INT(kw->token, 0x01, "for token");
    
    kw = ibmres_lookup_keyword("For");
    TEST_ASSERT(kw != NULL, "mixed case For found");
    
    kw = ibmres_lookup_keyword("LEFT$");
    TEST_ASSERT(kw != NULL, "LEFT$ found");
    
    kw = ibmres_lookup_keyword("left$");
    TEST_ASSERT(kw != NULL, "lowercase left$ found");
    
    TEST_PASS("ibmres_lookup_case_insensitive");
}

/* Test ibmres_lookup_keyword - not found */
void test_ibmres_lookup_not_found(void) {
    const struct keyword_entry *kw = ibmres_lookup_keyword("XYZNOTEXIST");
    TEST_ASSERT(kw == NULL, "nonexistent keyword returns NULL");
    
    kw = ibmres_lookup_keyword("");
    TEST_ASSERT(kw == NULL, "empty string returns NULL");
    
    kw = ibmres_lookup_keyword(NULL);
    TEST_ASSERT(kw == NULL, "NULL returns NULL");
    
    TEST_PASS("ibmres_lookup_not_found");
}

/* Test ibmres_get_token */
void test_ibmres_get_token(void) {
    WORD token = ibmres_get_token("FOR");
    TEST_ASSERT_EQ_INT(token, 0x01, "FOR token");
    
    token = ibmres_get_token("LEFT$");
    TEST_ASSERT_EQ_INT(token, 0x80, "LEFT$ token");
    
    token = ibmres_get_token("FILES");
    TEST_ASSERT_EQ_INT(token, 0xFE00, "FILES token");
    
    token = ibmres_get_token("CVI");
    TEST_ASSERT_EQ_INT(token, 0xFD00, "CVI token");
    
    token = ibmres_get_token("NONEXISTENT");
    TEST_ASSERT_EQ_INT(token, 0, "nonexistent returns 0");
    
    TEST_PASS("ibmres_get_token");
}

/* Test ibmres_get_token_name */
void test_ibmres_get_token_name(void) {
    const char *name = ibmres_get_token_name(0x01);
    TEST_ASSERT(name != NULL, "token 0x01 has name");
    TEST_ASSERT_EQ_STR(name, "FOR", "token 0x01 = FOR");
    
    name = ibmres_get_token_name(0x80);
    TEST_ASSERT(name != NULL, "token 0x80 has name");
    TEST_ASSERT_EQ_STR(name, "LEFT$", "token 0x80 = LEFT$");
    
    name = ibmres_get_token_name(0xFF);
    TEST_ASSERT(name == NULL, "unknown token returns NULL");
    
    TEST_PASS("ibmres_get_token_name");
}

/* Test ibmres_is_reserved_word */
void test_ibmres_is_reserved_word(void) {
    WORD token;
    int result = ibmres_is_reserved_word("FOR", &token);
    TEST_ASSERT(result != 0, "FOR is reserved");
    TEST_ASSERT_EQ_INT(token, 0x01, "FOR token");
    
    result = ibmres_is_reserved_word("PRINT", &token);
    TEST_ASSERT(result != 0, "PRINT is reserved");
    TEST_ASSERT_EQ_INT(token, 0x0F, "PRINT token");
    
    result = ibmres_is_reserved_word("XYZNOTEXIST", &token);
    TEST_ASSERT(result == 0, "XYZNOTEXIST not reserved");
    
    result = ibmres_is_reserved_word("", &token);
    TEST_ASSERT(result == 0, "empty string not reserved");
    
    result = ibmres_is_reserved_word(NULL, &token);
    TEST_ASSERT(result == 0, "NULL not reserved");
    
    TEST_PASS("ibmres_is_reserved_word");
}

/* Test ibmres_get_keyword_type */
void test_ibmres_get_keyword_type(void) {
    BYTE type = ibmres_get_keyword_type("FOR");
    TEST_ASSERT_EQ_INT(type, TK_STMT, "FOR is statement");
    
    type = ibmres_get_keyword_type("LEFT$");
    TEST_ASSERT_EQ_INT(type, TK_FUNC, "LEFT$ is function");
    
    type = ibmres_get_keyword_type("FILES");
    TEST_ASSERT_EQ_INT(type, TK_STMT_EXT, "FILES is extended statement");
    
    type = ibmres_get_keyword_type("CVI");
    TEST_ASSERT_EQ_INT(type, TK_FUNC_EXT, "CVI is extended function");
    
    type = ibmres_get_keyword_type("NONEXISTENT");
    TEST_ASSERT_EQ_INT(type, 0, "nonexistent type 0");
    
    TEST_PASS("ibmres_get_keyword_type");
}

/* Test ibmres_is_extended_token */
void test_ibmres_is_extended_token(void) {
    int result = ibmres_is_extended_token(0xFE);
    TEST_ASSERT(result != 0, "0xFE is extended");
    
    result = ibmres_is_extended_token(0xFD);
    TEST_ASSERT(result != 0, "0xFD is extended");
    
    result = ibmres_is_extended_token(0x01);
    TEST_ASSERT(result == 0, "0x01 is not extended");
    
    result = ibmres_is_extended_token(0x80);
    TEST_ASSERT(result == 0, "0x80 is not extended");
    
    TEST_PASS("ibmres_is_extended_token");
}

/* Test ibmres_get_token_length */
void test_ibmres_get_token_length(void) {
    int len = ibmres_get_token_length(0xFE);
    TEST_ASSERT_EQ_INT(len, 2, "0xFE length 2");
    
    len = ibmres_get_token_length(0xFD);
    TEST_ASSERT_EQ_INT(len, 2, "0xFD length 2");
    
    len = ibmres_get_token_length(0x01);
    TEST_ASSERT_EQ_INT(len, 1, "0x01 length 1");
    
    len = ibmres_get_token_length(0x80);
    TEST_ASSERT_EQ_INT(len, 1, "0x80 length 1");
    
    TEST_PASS("ibmres_get_token_length");
}

/* Test ibmres_crunch_extended */
void test_ibmres_crunch_extended(void) {
    const char *text = "FILES";
    WORD token;
    int result = ibmres_crunch_extended(&text, &token);
    TEST_ASSERT(result != 0, "FILES crunch succeeds");
    TEST_ASSERT_EQ_INT(token, 0xFE00, "FILES token");
    
    text = "PAINT";
    result = ibmres_crunch_extended(&text, &token);
    TEST_ASSERT(result != 0, "PAINT crunch succeeds");
    TEST_ASSERT_EQ_INT(token, 0xFE0D, "PAINT token");
    
    text = "NONEXISTENT";
    result = ibmres_crunch_extended(&text, &token);
    TEST_ASSERT(result == 0, "nonexistent crunch fails");
    
    text = "";
    result = ibmres_crunch_extended(&text, &token);
    TEST_ASSERT(result == 0, "empty string crunch fails");
    
    TEST_PASS("ibmres_crunch_extended");
}

/* Test ibmres_eval_extended */
void test_ibmres_eval_extended(void) {
    const char *text = "CVI";
    float result;
    int res = ibmres_eval_extended(&text, &result);
    TEST_ASSERT(res != 0, "CVI eval succeeds");
    TEST_ASSERT_EQ_FLOAT(result, 0.0f, "CVI returns 0");
    
    text = "KTN";
    res = ibmres_eval_extended(&text, &result);
    TEST_ASSERT(res != 0, "KTN eval succeeds");
    
    text = "NONEXISTENT";
    res = ibmres_eval_extended(&text, &result);
    TEST_ASSERT(res == 0, "nonexistent eval fails");
    
    TEST_PASS("ibmres_eval_extended");
}

/* Test ibmres_list_token */
void test_ibmres_list_token(void) {
    const char *name = ibmres_list_token(0x01);
    TEST_ASSERT(name != NULL, "token 0x01 has name");
    TEST_ASSERT_EQ_STR(name, "FOR", "token 0x01 = FOR");
    
    name = ibmres_list_token(0xFE00);
    /* Extended tokens would need extended name table */
    TEST_PASS("ibmres_list_token");
}

/* Test ibmres_list_extended */
void test_ibmres_list_extended(void) {
    char buffer[32];
    int result = ibmres_list_extended(0x01, buffer, sizeof(buffer));
    TEST_ASSERT(result != 0, "token 0x01 lists");
    TEST_ASSERT_EQ_STR(buffer, "FOR", "token 0x01 = FOR");
    
    result = ibmres_list_extended(0xFE00, buffer, sizeof(buffer));
    /* Extended tokens would need extended name table */
    TEST_PASS("ibmres_list_extended");
}

/* Test ibmres_term */
void test_ibmres_term(void) {
    ibmres_term();  /* Should not crash */
    TEST_PASS("ibmres_term");
}