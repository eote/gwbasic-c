/* IBMRES - IBM Compatible Reserved Words / Extended Token Handling */
/* Based on IBMRES.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Token types */
#define TK_STMT 0x00    /* 1-byte statement token */
#define TK_FUNC 0x80    /* 1-byte function token (high bit set) */
#define TK_STMT_EXT 0xFE /* 2-byte extended statement token (FE xx) */
#define TK_FUNC_EXT 0xFD /* 2-byte extended function token (FD xx) */

/* Maximum token lengths */
#define MAX_TOKEN_LEN 32
#define MAX_RESERVED_WORDS 512

/* Keyword entry structure */
struct keyword_entry {
    const char *name;
    WORD token;
    BYTE type;      /* TK_STMT, TK_FUNC, TK_STMT_EXT, TK_FUNC_EXT */
    BYTE category;  /* Statement, function, operator, etc. */
};

/* Core GW-BASIC reserved words (subset) */
static const struct keyword_entry g_core_keywords[] = {
    /* Statements */
    {"END",     0x00, TK_STMT, 0},
    {"FOR",     0x01, TK_STMT, 0},
    {"NEXT",    0x02, TK_STMT, 0},
    {"DATA",    0x03, TK_STMT, 0},
    {"INPUT",   0x04, TK_STMT, 0},
    {"DIM",     0x04, TK_STMT, 0},
    {"READ",    0x05, TK_STMT, 0},
    {"LET",     0x06, TK_STMT, 0},
    {"GOTO",    0x07, TK_STMT, 0},
    {"RUN",     0x08, TK_STMT, 0},
    {"IF",      0x09, TK_STMT, 0},
    {"RESTORE", 0x0A, TK_STMT, 0},
    {"GOSUB",   0x0B, TK_STMT, 0},
    {"RETURN",  0x0C, TK_STMT, 0},
    {"REM",     0x0D, TK_STMT, 0},
    {"STOP",    0x0E, TK_STMT, 0},
    {"PRINT",   0x0F, TK_STMT, 0},
    {"CLEAR",   0x10, TK_STMT, 0},
    {"LIST",    0x11, TK_STMT, 0},
    {"NEW",     0x12, TK_STMT, 0},
    {"ON",      0x13, TK_STMT, 0},
    {"WAIT",    0x14, TK_STMT, 0},
    {"DEF",     0x15, TK_STMT, 0},
    {"POKE",    0x16, TK_STMT, 0},
    {"CONT",    0x17, TK_STMT, 0},
    {"OUT",     0x18, TK_STMT, 0},
    {"LPRINT",  0x19, TK_STMT, 0},
    {"LLIST",   0x1A, TK_STMT, 0},
    {"WIDTH",   0x1B, TK_STMT, 0},
    {"ELSE",    0x1C, TK_STMT, 0},
    {"TRON",    0x1D, TK_STMT, 0},
    {"TROFF",   0x1E, TK_STMT, 0},
    {"SWAP",    0x1F, TK_STMT, 0},
    {"ERASE",   0x20, TK_STMT, 0},
    {"EDIT",    0x21, TK_STMT, 0},
    {"ERROR",   0x22, TK_STMT, 0},
    {"RESUME",  0x22, TK_STMT, 0},
    {"DELETE",  0x23, TK_STMT, 0},
    {"AUTO",    0x24, TK_STMT, 0},
    {"RENUM",   0x25, TK_STMT, 0},
    {"DEFSTR",  0x26, TK_STMT, 0},
    {"DEFINT",  0x27, TK_STMT, 0},
    {"DEFSNG",  0x28, TK_STMT, 0},
    {"DEFDBL",  0x29, TK_STMT, 0},
    {"LINE",    0x2A, TK_STMT, 0},
    {"WHILE",   0x2B, TK_STMT, 0},
    {"WEND",    0x2C, TK_STMT, 0},
    {"CALL",    0x2D, TK_STMT, 0},
    {"WRITE",   0x2E, TK_STMT, 0},
    {"OPTION",  0x2F, TK_STMT, 0},
    {"RANDOMIZE", 0x30, TK_STMT, 0},
    {"OPEN",    0x31, TK_STMT, 0},
    {"CLOSE",   0x32, TK_STMT, 0},
    {"LOAD",    0x33, TK_STMT, 0},
    {"MERGE",   0x34, TK_STMT, 0},
    {"SAVE",    0x35, TK_STMT, 0},
    {"COLOR",   0x36, TK_STMT, 0},
    {"CLS",     0x37, TK_STMT, 0},
    {"MOTOR",   0x38, TK_STMT, 0},
    {"BSAVE",   0x39, TK_STMT, 0},
    {"BLOAD",   0x3A, TK_STMT, 0},
    {"SOUND",   0x3B, TK_STMT, 0},
    {"BEEP",    0x3C, TK_STMT, 0},
    {"PSET",    0x3D, TK_STMT, 0},
    {"PRESET",  0x3E, TK_STMT, 0},
    {"SCREEN",  0x3F, TK_STMT, 0},
    {"KEY",     0x40, TK_STMT, 0},
    {"LOCATE",  0x41, TK_STMT, 0},
    {"TO",      0x80, TK_STMT, 0},
    {"THEN",    0x81, TK_STMT, 0},
    {"TAB(",    0x82, TK_STMT, 0},
    {"STEP",    0x83, TK_STMT, 0},
    {"USR",     0x84, TK_STMT, 0},
    {"FN",      0x85, TK_STMT, 0},
    {"SPC(",    0x86, TK_STMT, 0},
    {"NOT",     0x87, TK_STMT, 0},
    {"ERL",     0x88, TK_STMT, 0},
    {"ERR",     0x89, TK_STMT, 0},
    {"STRING$", 0x8A, TK_STMT, 0},
    {"USING",   0x8B, TK_STMT, 0},
    {"INSTRC",  0x8C, TK_STMT, 0},
    {"DUMMY",   0x8D, TK_STMT, 0},
    {"VARPTR",  0x8E, TK_STMT, 0},
    {"CSRLIN",  0x8F, TK_STMT, 0},
    {"POINT",   0x90, TK_STMT, 0},
    {"OFF",     0x91, TK_STMT, 0},
    {"INKEY$",  0x92, TK_STMT, 0},
    
    /* Operators (single byte, high bit set for some) */
    {">",       0xA0, TK_STMT, 0},
    {"=",       0xA1, TK_STMT, 0},
    {"<",       0xA2, TK_STMT, 0},
    {"+",       0xA2, TK_STMT, 0},
    {"-",       0xA3, TK_STMT, 0},
    {"*",       0xA4, TK_STMT, 0},
    {"/",       0xA5, TK_STMT, 0},
    {"^",       0xA6, TK_STMT, 0},
    {"AND",     0xA7, TK_STMT, 0},
    {"OR",      0xA8, TK_STMT, 0},
    {"XOR",     0xA9, TK_STMT, 0},
    {"EQV",     0xAA, TK_STMT, 0},
    {"IMP",     0xAB, TK_STMT, 0},
    {"MOD",     0xAC, TK_STMT, 0},
    {"\\",      0xAD, TK_STMT, 0},
    
    /* Functions */
    {"LEFT$",   0x80, TK_FUNC, 0},
    {"RIGHT$",  0x81, TK_FUNC, 0},
    {"MID$",    0x82, TK_FUNC, 0},
    {"SGN",     0x83, TK_FUNC, 0},
    {"INT",     0x84, TK_FUNC, 0},
    {"ABS",     0x85, TK_FUNC, 0},
    {"SQR",     0x86, TK_FUNC, 0},
    {"RND",     0x87, TK_FUNC, 0},
    {"SIN",     0x88, TK_FUNC, 0},
    {"LOG",     0x89, TK_FUNC, 0},
    {"EXP",     0x8A, TK_FUNC, 0},
    {"COS",     0x8B, TK_FUNC, 0},
    {"TAN",     0x8B, TK_FUNC, 0},
    {"ATN",     0x8C, TK_FUNC, 0},
    {"FRE",     0x8D, TK_FUNC, 0},
    {"INP",     0x8E, TK_FUNC, 0},
    {"POS",     0x8F, TK_FUNC, 0},
    {"LEN",     0x90, TK_FUNC, 0},
    {"STR$",    0x91, TK_FUNC, 0},
    {"VAL",     0x92, TK_FUNC, 0},
    {"ASC",     0x93, TK_FUNC, 0},
    {"CHR$",    0x94, TK_FUNC, 0},
    {"PEEK",    0x95, TK_FUNC, 0},
    {"SPACE$",  0x96, TK_FUNC, 0},
    {"UCASE$",  0x97, TK_FUNC, 0},
    {"LCASE$",  0x98, TK_FUNC, 0},
    {"OCT$",    0x99, TK_FUNC, 0},
    {"HEX$",    0x9A, TK_FUNC, 0},
    {"LPOS",    0x9B, TK_FUNC, 0},
    {"CINT",    0x9C, TK_FUNC, 0},
    {"CSNG",    0x9D, TK_FUNC, 0},
    {"CDBL",    0x9E, TK_FUNC, 0},
    {"FIX",     0x9F, TK_FUNC, 0},
    {"PEN",     0xA0, TK_FUNC, 0},
    {"STICK",   0xA1, TK_FUNC, 0},
    {"STRIG",   0xA2, TK_FUNC, 0},
    {"EOF",     0xA3, TK_FUNC, 0},
    {"LOC",     0xA4, TK_FUNC, 0},
    {"LOF",     0xA5, TK_FUNC, 0},
    
    /* Extended statement tokens (FE prefix) */
    {"FILES",   0xFE00, TK_STMT_EXT, 0},
    {"FIELD",   0xFE01, TK_STMT_EXT, 0},
    {"SYSTEM",  0xFE02, TK_STMT_EXT, 0},
    {"NAME",    0xFE03, TK_STMT_EXT, 0},
    {"LSET",    0xFE04, TK_STMT_EXT, 0},
    {"RSET",    0xFE05, TK_STMT_EXT, 0},
    {"KILL",    0xFE06, TK_STMT_EXT, 0},
    {"PUT",     0xFE07, TK_STMT_EXT, 0},
    {"GET",     0xFE08, TK_STMT_EXT, 0},
    {"RESET",   0xFE09, TK_STMT_EXT, 0},
    {"COMMON",  0xFE0A, TK_STMT_EXT, 0},
    {"CHAIN",   0xFE0A, TK_STMT_EXT, 0},
    {"DATE$",   0xFE0B, TK_STMT_EXT, 0},
    {"TIME$",   0xFE0C, TK_STMT_EXT, 0},
    {"PAINT",   0xFE0D, TK_STMT_EXT, 0},
    {"COM",     0xFE0E, TK_STMT_EXT, 0},
    {"CIRCLE",  0xFE0F, TK_STMT_EXT, 0},
    {"DRAW",    0xFE10, TK_STMT_EXT, 0},
    {"PLAY",    0xFE11, TK_STMT_EXT, 0},
    {"TIMER",   0xFE12, TK_STMT_EXT, 0},
    {"ERDEV",   0xFE13, TK_STMT_EXT, 0},
    {"IOCTL",   0xFE14, TK_STMT_EXT, 0},
    {"CHDIR",   0xFE15, TK_STMT_EXT, 0},
    {"MKDIR",   0xFE16, TK_STMT_EXT, 0},
    {"RMDIR",   0xFE17, TK_STMT_EXT, 0},
    {"SHELL",   0xFE18, TK_STMT_EXT, 0},
    {"ENVIRON", 0xFE19, TK_STMT_EXT, 0},
    {"VIEW",    0xFE1A, TK_STMT_EXT, 0},
    {"WINDOW",  0xFE1B, TK_STMT_EXT, 0},
    {"PMAP",    0xFE1C, TK_STMT_EXT, 0},
    {"PALETTE", 0xFE1D, TK_STMT_EXT, 0},
    {"LCOPY",   0xFE1E, TK_STMT_EXT, 0},
    {"CALLS",   0xFE1F, TK_STMT_EXT, 0},
    
    /* Extended functions (FD prefix) */
    {"CVI",     0xFD00, TK_FUNC_EXT, 0},
    {"CVS",     0xFD01, TK_FUNC_EXT, 0},
    {"CVD",     0xFD02, TK_FUNC_EXT, 0},
    {"MKI$",    0xFD03, TK_FUNC_EXT, 0},
    {"MKS$",    0xFD04, TK_FUNC_EXT, 0},
    {"MKD$",    0xFD05, TK_FUNC_EXT, 0},
    {"KTN",     0xFD06, TK_FUNC_EXT, 0},
    {"JIS",     0xFD07, TK_FUNC_EXT, 0},
    {"KPOS",    0xFD08, TK_FUNC_EXT, 0},
    {"KLEN",    0xFD09, TK_FUNC_EXT, 0},
};

/* Number of core keywords */
#define NUM_CORE_KEYWORDS (sizeof(g_core_keywords) / sizeof(g_core_keywords[0]))

/* Token lookup tables */
static const char *g_token_names[256] = {0};

/* Forward declaration */
static void ibmres_build_tables(void);

/* Module initialization */
void ibmres_init(void) {
    ibmres_build_tables();
}

/* Build token lookup tables */
void ibmres_build_tables(void) {
    int i;
    
    /* Initialize token name table */
    for (i = 0; i < 256; i++) {
        g_token_names[i] = NULL;
    }
    
    /* Populate from core keywords */
    for (i = 0; i < (int)NUM_CORE_KEYWORDS; i++) {
        const struct keyword_entry *kw = &g_core_keywords[i];
        
        if (kw->type == TK_STMT && kw->token < 256) {
            g_token_names[kw->token] = kw->name;
        } else if (kw->type == TK_FUNC && kw->token < 256) {
            g_token_names[kw->token] = kw->name;
        }
    }
}

/* Check if token is an extended (2-byte) token */
int ibmres_is_extended_token(BYTE token) {
    return (token == 0xFE || token == 0xFD);
}

/* Get token length (1 or 2 bytes) */
int ibmres_get_token_length(BYTE first_byte) {
    if (ibmres_is_extended_token(first_byte)) {
        return 2;
    }
    return 1;
}

/* Lookup keyword by name (case-insensitive) */
const struct keyword_entry* ibmres_lookup_keyword(const char *name) {
    if (!name) return NULL;
    
    for (int i = 0; i < (int)NUM_CORE_KEYWORDS; i++) {
        if (strcasecmp(g_core_keywords[i].name, name) == 0) {
            return &g_core_keywords[i];
        }
    }
    return NULL;
}

/* Get token for keyword name */
WORD ibmres_get_token(const char *name) {
    const struct keyword_entry *kw = ibmres_lookup_keyword(name);
    if (kw) {
        return kw->token;
    }
    return 0;
}

/* Get keyword name from token */
const char* ibmres_get_token_name(WORD token) {
    if (token < 256) {
        return g_token_names[token];
    }
    return NULL;
}

/* Check if string is a reserved word */
int ibmres_is_reserved_word(const char *text, WORD *out_token) {
    if (!text || !*text) return 0;
    
    const struct keyword_entry *kw = ibmres_lookup_keyword(text);
    if (kw && out_token) {
        *out_token = kw->token;
    }
    return kw != NULL;
}

/* Get keyword type */
BYTE ibmres_get_keyword_type(const char *name) {
    const struct keyword_entry *kw = ibmres_lookup_keyword(name);
    return kw ? kw->type : 0;
}

/* Extended token handling for CRUNCH/LIST/EVAL */
int ibmres_crunch_extended(const char **text, WORD *out_token) {
    /* Handle extended keywords (FE/FD prefix) */
    if (!text || !*text) return 0;
    
    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    
    /* Extract identifier */
    while (*text && (isalnum((unsigned char)**text) || **text == '$' || **text == '%' || **text == '#' || **text == '!' || **text == '@') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = (char)toupper((unsigned char)**text);
        (*text)++;
    }
    buffer[i] = '\0';
    
    if (i == 0) return 0;
    
    const struct keyword_entry *kw = ibmres_lookup_keyword(buffer);
    if (kw && (kw->type == TK_STMT_EXT || kw->type == TK_FUNC_EXT)) {
        if (out_token) *out_token = kw->token;
        return 1;
    }
    
    return 0;
}

/* List extended token */
const char* ibmres_list_token(WORD token) {
    if (token < 256) {
        return g_token_names[token];
    }
    
    /* Extended tokens - would need extended name table */
    return NULL;
}

/* Extended function evaluation dispatch */
int ibmres_eval_extended(const char **text, float *result) {
    if (!text || !*text) return 0;
    
    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    
    /* Extract function name */
    while (*text && (isalnum((unsigned char)**text) || **text == '$' || **text == '%') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = (char)toupper((unsigned char)**text);
        (*text)++;
    }
    buffer[i] = '\0';
    
    if (i == 0) return 0;
    
    const struct keyword_entry *kw = ibmres_lookup_keyword(buffer);
    if (kw && kw->type == TK_FUNC_EXT) {
        /* Extended function evaluation would dispatch here */
        /* For now, return 0 as placeholder */
        if (result) *result = 0.0f;
        return 1;
    }
    
    return 0;
}

/* List extended statement */
int ibmres_list_extended(WORD token, char *buffer, int buflen) {
    const char *name = ibmres_list_token(token);
    if (name) {
        strncpy(buffer, name, buflen - 1);
        buffer[buflen - 1] = '\0';
        return 1;
    }
    return 0;
}

/* Module termination */
void ibmres_term(void) {
    /* Nothing to clean up */
}