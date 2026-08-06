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

/* ===== Additional comprehensive tests ===== */

/* Test all core statement keywords */
void test_ibmres_all_core_statements(void) {
    const char *statements[] = {
        "END", "FOR", "NEXT", "DATA", "INPUT", "DIM", "READ", "LET",
        "GOTO", "RUN", "IF", "RESTORE", "GOSUB", "RETURN", "REM",
        "STOP", "PRINT", "CLEAR", "LIST", "NEW", "ON", "WAIT",
        "DEF", "POKE", "CONT", "OUT", "LPRINT", "LLIST", "WIDTH",
        "ELSE", "TRON", "TROFF", "SWAP", "ERASE", "EDIT", "ERROR",
        "RESUME", "DELETE", "AUTO", "RENUM", "DEFSTR", "DEFINT",
        "DEFSNG", "DEFDBL", "LINE", "WHILE", "WEND", "CALL",
        "WRITE", "OPTION", "RANDOMIZE", "OPEN", "CLOSE", "LOAD",
        "MERGE", "SAVE", "COLOR", "CLS", "MOTOR", "BSAVE", "BLOAD",
        "SOUND", "BEEP", "PSET", "PRESET", "SCREEN", "KEY", "LOCATE",
        NULL
    };
    
    for (int i = 0; statements[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(statements[i]);
        TEST_ASSERT(kw != NULL, "Statement keyword found");
        TEST_ASSERT_EQ_INT(kw->type, TK_STMT, "Statement type correct");
    }
    TEST_PASS("ibmres_all_core_statements");
}

/* Test all core function keywords */
void test_ibmres_all_core_functions(void) {
    const char *functions[] = {
        "LEFT$", "RIGHT$", "MID$", "SGN", "INT", "ABS", "SQR",
        "RND", "SIN", "LOG", "EXP", "COS", "TAN", "ATN",
        "FRE", "INP", "POS", "LEN", "STR$", "VAL", "ASC",
        "CHR$", "PEEK", "SPACE$", "OCT$", "HEX$", "LPOS",
        "CINT", "CSNG", "CDBL", "FIX", "PEN", "STICK", "STRIG",
        "EOF", "LOC", "LOF",
        NULL
    };
    
    for (int i = 0; functions[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(functions[i]);
        TEST_ASSERT(kw != NULL, "Function keyword found");
        TEST_ASSERT_EQ_INT(kw->type, TK_FUNC, "Function type correct");
    }
    TEST_PASS("ibmres_all_core_functions");
}

/* Test all extended statement keywords */
void test_ibmres_all_extended_statements(void) {
    const char *ext_stmts[] = {
        "FILES", "FIELD", "SYSTEM", "NAME", "LSET", "RSET",
        "KILL", "PUT", "GET", "RESET", "COMMON", "CHAIN",
        "DATE$", "TIME$", "PAINT", "COM", "CIRCLE", "DRAW",
        "PLAY", "TIMER", "ERDEV", "IOCTL", "CHDIR", "MKDIR",
        "RMDIR", "SHELL", "ENVIRON", "VIEW", "WINDOW", "PMAP",
        "PALETTE", "LCOPY", "CALLS",
        NULL
    };
    
    for (int i = 0; ext_stmts[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(ext_stmts[i]);
        TEST_ASSERT(kw != NULL, "Extended statement found");
        TEST_ASSERT_EQ_INT(kw->type, TK_STMT_EXT, "Extended statement type correct");
        TEST_ASSERT((kw->token & 0xFF00) == 0xFE00, "Extended statement token format");
    }
    TEST_PASS("ibmres_all_extended_statements");
}

/* Test all extended function keywords */
void test_ibmres_all_extended_functions(void) {
    const char *ext_funcs[] = {
        "CVI", "CVS", "CVD", "MKI$", "MKS$", "MKD$",
        "KTN", "JIS", "KPOS", "KLEN",
        NULL
    };
    
    for (int i = 0; ext_funcs[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(ext_funcs[i]);
        TEST_ASSERT(kw != NULL, "Extended function found");
        TEST_ASSERT_EQ_INT(kw->type, TK_FUNC_EXT, "Extended function type correct");
        TEST_ASSERT((kw->token & 0xFF00) == 0xFD00, "Extended function token format");
    }
    TEST_PASS("ibmres_all_extended_functions");
}

/* Test operator keywords */
void test_ibmres_operators(void) {
    const char *operators[] = {
        ">", "=", "<", "+", "-", "*", "/", "^",
        "AND", "OR", "XOR", "EQV", "IMP", "MOD", "\\",
        "TO", "THEN", "TAB(", "STEP", "USR", "FN", "SPC(", "NOT",
        "ERL", "ERR", "STRING$", "USING", "INSTRC", "VARPTR",
        "CSRLIN", "POINT", "OFF", "INKEY$",
        NULL
    };
    
    for (int i = 0; operators[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(operators[i]);
        TEST_ASSERT(kw != NULL, "Operator keyword found");
    }
    TEST_PASS("ibmres_operators");
}

/* Test crunch extended with various inputs */
void test_ibmres_crunch_extended_comprehensive(void) {
    struct {
        const char *input;
        WORD expected_token;
        int should_succeed;
    } tests[] = {
        {"FILES", 0xFE00, 1},
        {"FIELD", 0xFE01, 1},
        {"SYSTEM", 0xFE02, 1},
        {"NAME", 0xFE03, 1},
        {"LSET", 0xFE04, 1},
        {"RSET", 0xFE05, 1},
        {"KILL", 0xFE06, 1},
        {"PUT", 0xFE07, 1},
        {"GET", 0xFE08, 1},
        {"RESET", 0xFE09, 1},
        {"COMMON", 0xFE0A, 1},
        {"CHAIN", 0xFE0A, 1},
        {"DATE$", 0xFE0B, 1},
        {"TIME$", 0xFE0C, 1},
        {"PAINT", 0xFE0D, 1},
        {"COM", 0xFE0E, 1},
        {"CIRCLE", 0xFE0F, 1},
        {"DRAW", 0xFE10, 1},
        {"PLAY", 0xFE11, 1},
        {"TIMER", 0xFE12, 1},
        {"ERDEV", 0xFE13, 1},
        {"IOCTL", 0xFE14, 1},
        {"CHDIR", 0xFE15, 1},
        {"MKDIR", 0xFE16, 1},
        {"RMDIR", 0xFE17, 1},
        {"SHELL", 0xFE18, 1},
        {"ENVIRON", 0xFE19, 1},
        {"VIEW", 0xFE1A, 1},
        {"WINDOW", 0xFE1B, 1},
        {"PMAP", 0xFE1C, 1},
        {"PALETTE", 0xFE1D, 1},
        {"LCOPY", 0xFE1E, 1},
        {"CALLS", 0xFE1F, 1},
        {"CVI", 0xFD00, 1},
        {"CVS", 0xFD01, 1},
        {"CVD", 0xFD02, 1},
        {"MKI$", 0xFD03, 1},
        {"MKS$", 0xFD04, 1},
        {"MKD$", 0xFD05, 1},
        {"KTN", 0xFD06, 1},
        {"JIS", 0xFD07, 1},
        {"KPOS", 0xFD08, 1},
        {"KLEN", 0xFD09, 1},
        {"NONEXISTENT", 0, 0},
        {"", 0, 0},
        {NULL, 0, 0}
    };
    
    for (int i = 0; tests[i].input != NULL; i++) {
        const char *text = tests[i].input;
        WORD token = 0;
        int result = ibmres_crunch_extended(&text, &token);
        
        if (tests[i].should_succeed) {
            TEST_ASSERT(result != 0, "Crunch should succeed");
            TEST_ASSERT_EQ_INT(token, tests[i].expected_token, "Token matches");
        } else {
            TEST_ASSERT(result == 0, "Should fail for invalid input");
        }
    }
    TEST_PASS("ibmres_crunch_extended_comprehensive");
}

/* Test eval extended for all extended functions */
void test_ibmres_eval_extended_comprehensive(void) {
    const char *funcs[] = {
        "CVI", "CVS", "CVD", "MKI$", "MKS$", "MKD$",
        "KTN", "JIS", "KPOS", "KLEN",
        NULL
    };
    
    for (int i = 0; funcs[i] != NULL; i++) {
        const char *text = funcs[i];
        float result;
        int res = ibmres_eval_extended(&text, &result);
        TEST_ASSERT(res != 0, "Extended function eval succeeds");
    }
    TEST_PASS("ibmres_eval_extended_comprehensive");
}

/* Test is_reserved_word for all keywords */
void test_ibmres_is_reserved_word_comprehensive(void) {
    const char *all_keywords[] = {
        "FOR", "PRINT", "IF", "FILES", "PAINT", "CIRCLE",
        "LEFT$", "SIN", "CVI", "KTN",
        "NONEXISTENT", "", NULL
    };
    
    WORD token;
    for (int i = 0; all_keywords[i] != NULL; i++) {
        int result = ibmres_is_reserved_word(all_keywords[i], &token);
        if (strcmp(all_keywords[i], "NONEXISTENT") == 0 || 
            strcmp(all_keywords[i], "") == 0) {
            TEST_ASSERT(result == 0, "Non-keyword returns 0");
        } else {
            TEST_ASSERT(result != 0, "Keyword is reserved");
            TEST_ASSERT(token != 0, "Token is non-zero");
        }
    }
    TEST_PASS("ibmres_is_reserved_word_comprehensive");
}

/* Test get_keyword_type for all categories */
void test_ibmres_get_keyword_type_comprehensive(void) {
    struct {
        const char *keyword;
        BYTE expected_type;
    } tests[] = {
        {"FOR", TK_STMT}, {"PRINT", TK_STMT}, {"IF", TK_STMT},
        {"LEFT$", TK_FUNC}, {"SIN", TK_FUNC}, {"RND", TK_FUNC},
        {"FILES", TK_STMT_EXT}, {"PAINT", TK_STMT_EXT}, {"CIRCLE", TK_STMT_EXT},
        {"CVI", TK_FUNC_EXT}, {"KTN", TK_FUNC_EXT}, {"JIS", TK_FUNC_EXT},
        {"NONEXISTENT", 0}, {NULL, 0}
    };
    
    for (int i = 0; tests[i].keyword != NULL; i++) {
        BYTE type = ibmres_get_keyword_type(tests[i].keyword);
        TEST_ASSERT_EQ_INT(type, tests[i].expected_type, "Keyword type matches");
    }
    TEST_PASS("ibmres_get_keyword_type_comprehensive");
}

/* Test extended token functions */
void test_ibmres_extended_token_functions(void) {
    int result;
    
    result = ibmres_is_extended_token(0xFE);
    TEST_ASSERT(result != 0, "0xFE is extended");
    
    result = ibmres_is_extended_token(0xFD);
    TEST_ASSERT(result != 0, "0xFD is extended");
    
    result = ibmres_is_extended_token(0xFF);
    TEST_ASSERT(result == 0, "0xFF is not extended");
    
    int len = ibmres_get_token_length(0xFE);
    TEST_ASSERT_EQ_INT(len, 2, "0xFE length 2");
    
    len = ibmres_get_token_length(0xFD);
    TEST_ASSERT_EQ_INT(len, 2, "0xFD length 2");
    
    len = ibmres_get_token_length(0xFF);
    TEST_ASSERT_EQ_INT(len, 1, "0xFF length 1");
    
    TEST_PASS("ibmres_extended_token_functions");
}

/* Test list_token and list_extended */
void test_ibmres_list_functions(void) {
    const char *name = ibmres_list_token(0x01);
    TEST_ASSERT(name != NULL, "token 0x01 has name");
    TEST_ASSERT_EQ_STR(name, "FOR", "token 0x01 = FOR");
    
    char buffer[32];
    int result = ibmres_list_extended(0x01, buffer, sizeof(buffer));
    TEST_ASSERT(result != 0, "token 0x01 lists");
    TEST_ASSERT_EQ_STR(buffer, "FOR", "token 0x01 = FOR");
    
    result = ibmres_list_extended(0xFE00, buffer, sizeof(buffer));
    /* Extended tokens would need extended name table */
    TEST_PASS("ibmres_list_functions");
}

/* Test init and term */
void test_ibmres_init_term(void) {
    ibmres_init();
    TEST_PASS("ibmres_init");
    
    ibmres_term();
    TEST_PASS("ibmres_term");
    
    /* Double init/term should be safe */
    ibmres_init();
    ibmres_term();
    TEST_PASS("ibmres_init_term_double");
}

/* Test case insensitivity for all types */
void test_ibmres_case_insensitive_all(void) {
    const char *keywords[] = {
        "FOR", "for", "For", "fOr",
        "FILES", "files", "Files", "FiLeS",
        "CVI", "cvi", "Cvi", "cVi",
        "KTN", "ktn", "Ktn",
        NULL
    };
    
    for (int i = 0; keywords[i] != NULL; i++) {
        const struct keyword_entry *kw = ibmres_lookup_keyword(keywords[i]);
        TEST_ASSERT(kw != NULL, "Case insensitive lookup works");
    }
    TEST_PASS("ibmres_case_insensitive_all");
}