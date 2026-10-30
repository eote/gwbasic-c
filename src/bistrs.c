/* BISTRS - String Functions */
/* Based on BISTRS.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

void bistrs_init(void) {
    /* No initialization needed */
}

/* LEFT$ - return leftmost n characters */
char *bistrs_left(const char *str, int n) {
    if (!str || n <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int len = strlen(str);
    if (n > len) n = len;
    
    char *result = (char *)gwram_alloc(n + 1);
    if (!result) return NULL;
    
    memcpy(result, str, n);
    result[n] = '\0';
    return result;
}

/* RIGHT$ - return rightmost n characters */
char *bistrs_right(const char *str, int n) {
    if (!str || n <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int len = strlen(str);
    if (n > len) n = len;
    
    char *result = (char *)gwram_alloc(n + 1);
    if (!result) return NULL;
    
    memcpy(result, str + len - n, n);
    result[n] = '\0';
    return result;
}

/* MID$ - return substring from start position for len characters */
char *bistrs_mid(const char *str, int start, int len) {
    if (!str || start <= 0 || len <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int str_len = strlen(str);
    /* GW-BASIC: start is 1-based */
    start--;
    if (start >= str_len) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    if (start + len > str_len) len = str_len - start;
    
    char *result = (char *)gwram_alloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, str + start, len);
    result[len] = '\0';
    return result;
}

/* UCASE$ - return uppercase version of string */
char *bistrs_ucase(const char *str) {
    if (!str) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int len = strlen(str);
    char *result = (char *)gwram_alloc(len + 1);
    if (!result) return NULL;
    
    for (int i = 0; i < len; i++) {
        result[i] = toupper((unsigned char)str[i]);
    }
    result[len] = '\0';
    return result;
}

/* LCASE$ - return lowercase version of string */
char *bistrs_lcase(const char *str) {
    if (!str) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int len = strlen(str);
    char *result = (char *)gwram_alloc(len + 1);
    if (!result) return NULL;
    
    for (int i = 0; i < len; i++) {
        result[i] = tolower((unsigned char)str[i]);
    }
    result[len] = '\0';
    return result;
}

/* STR$ - convert number to string */
char *bistrs_str(double val) {
    char *result = (char *)gwram_alloc(32);
    if (!result) return NULL;
    
    /* GW-BASIC: STR$ of positive numbers has a leading space */
    sprintf(result, "%s%g", (val >= 0) ? " " : "", val);
    return result;
}

/* VAL - convert string to number */
double bistrs_val(const char *str) {
    if (!str) return 0.0;
    return atof(str);
}

/* STRING$ - return string of n repetitions of character ch */
char *bistrs_string(int n, char ch) {
    if (n <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    char *result = (char *)gwram_alloc(n + 1);
    if (!result) return NULL;
    
    memset(result, ch, n);
    result[n] = '\0';
    return result;
}

/* SPACE$ - return string of n spaces */
char *bistrs_space(int n) {
    return bistrs_string(n, ' ');
}

/* CHR$ - return character for ASCII code */
char *bistrs_chr(int code) {
    if (code < 0 || code > 255) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    char *result = (char *)gwram_alloc(2);
    if (!result) return NULL;
    
    result[0] = (char)code;
    result[1] = '\0';
    return result;
}

/* MKI$ - convert integer to 2-byte string */
char *bistrs_mki(int val) {
    char *result = (char *)gwram_alloc(3);  /* 2 bytes + null */
    if (!result) return NULL;
    
    result[0] = (char)(val & 0xFF);
    result[1] = (char)((val >> 8) & 0xFF);
    result[2] = '\0';
    return result;
}

/* MKS$ - convert single precision float to 4-byte string */
char *bistrs_mks(float val) {
    char *result = (char *)gwram_alloc(5);  /* 4 bytes + null */
    if (!result) return NULL;
    
    uint32_t bits;
    memcpy(&bits, &val, 4);
    result[0] = (char)(bits & 0xFF);
    result[1] = (char)((bits >> 8) & 0xFF);
    result[2] = (char)((bits >> 16) & 0xFF);
    result[3] = (char)((bits >> 24) & 0xFF);
    result[4] = '\0';
    return result;
}

/* CVI - convert 2-byte string to integer */
int bistrs_cvi(const char *str) {
    if (!str) return 0;
    /* MKI$ produces 2-byte binary string; check actual data not strlen */
    /* We can't use strlen because of embedded NUL */
    /* Assume valid MKI$ has at least 2 bytes allocated */
    int val = (unsigned char)str[0] | ((unsigned char)str[1] << 8);
    return val;
}

/* CVS - convert 4-byte string to single precision float */
float bistrs_cvs(const char *str) {
    if (!str) return 0.0f;
    uint32_t bits = (unsigned char)str[0] | 
                    ((unsigned char)str[1] << 8) | 
                    ((unsigned char)str[2] << 16) | 
                    ((unsigned char)str[3] << 24);
    float val;
    memcpy(&val, &bits, 4);
    return val;
}

/* ASC - return ASCII code of first character */
int bistrs_asc(const char *str) {
    if (!str || !*str) return 0;
    return (unsigned char)str[0];
}

/* LEN - return string length */
int bistrs_len(const char *str) {
    if (!str) return 0;
    return strlen(str);
}

/* INSTR — substring search (ASM INSTRC BISTRS.ASM L841-949).
 * Forms: INSTR([start,] S1$, S2$) — caller resolves to (s1, s2, start).
 * Semantics per ASM:
 * - start = 0            → FCERR (??L009: "DONT ALLOW ZERO OFFSET")
 * - start > LEN(S1$)     → 0 (offset beyond length → return zero)
 * - S2$ empty            → return start (S2 NULL → return offset)
 * - S1$ empty            → 0
 * - found                → 1-based position of FIRST occurrence (GOTSTR)
 * - not found            → 0 (RETZR1)
 * ERROR (-1) maps FCERR for the C caller. */
int bistrs_instr(const char *s1, const char *s2, int start) {
    /* v133 MEDIUM-1: GW-BASIC manual — start must be 1..255, else FCERR.
     * (ASM INSTRC holds start in AL: byte semantics; start=0 → FCERR, and
     * start>255 cannot occur for ASM's ≤255-byte strings. The port permits
     * longer strings via WORD length, so the 255 ceiling is enforced here
     * to keep manual-consistent FCERR semantics.) */
    if (start < 1 || start > 255) return -1;  /* FCERR */
    if (!s1 || !*s1) return 0;            /* S1$ NULL → 0 */
    size_t len1 = strlen(s1);
    if ((size_t)start > len1) return 0;   /* offset beyond length → 0 */
    if (!s2 || !*s2) return start;        /* S2$ NULL → return offset */

    /* Search from 1-based start (ASM: DEC AL to make offset 0-based) */
    const char *from = s1 + (start - 1);
    const char *hit = strstr(from, s2);
    if (!hit) return 0;                   /* RETZR1 */
    return (int)(hit - s1) + 1;           /* GOTSTR: 1-based position */
}

/* FRE - return free memory (arg: 0 for numeric, non-zero for string space) */
double bistrs_fre(int arg) {
    if (!g_state) {
        return 0.0;
    }
    
    size_t free_mem = 0;
    if (arg == 0) {
        /* FRE(0) - return free numeric memory (approximate) */
        /* Use string heap free space as approximation */
        free_mem = STRING_HEAP_SIZE - g_state->str_heap.used;
    } else {
        /* FRE(string) - return free string space */
        free_mem = STRING_HEAP_SIZE - g_state->str_heap.used;
    }
    
    return (double)free_mem;
}

/* ===== v158: HEX$ / OCT$ (ASM: BIPRTU? — 16-bit unsigned range) =====
 * GW-BASIC: HEX$(n)/OCT$(n) take n in -32768..65535; negative values
 * are the two's-complement 16-bit pattern (HEX$(-1) = "FFFF"). */
/* v159 HIGH-4: ASM FRQINT/FRCINT semantics — round to nearest
 * integer (FRCINT ADC rounding), valid range -32768..65535,
 * out of range → Overflow (error 6). */
static int bistrs_conv_int(double val) {
    if (val > 65535.0 || val < -32768.0 || val != val) {
        gwdata_set_error(6, g_state ? (uint16_t)g_state->current_line : 0, 0);
        return 0;
    }
    /* round half away from zero (ASM ADC rounding on magnitude) */
    return (int)(val >= 0.0 ? floor(val + 0.5) : ceil(val - 0.5));
}

char *bistrs_hex(double val) {
    int n = bistrs_conv_int(val);
    if (g_state && g_state->error_code != 0) return NULL;
    unsigned short u = (unsigned short)(n & 0xFFFF);
    char buf[8];
    sprintf(buf, "%X", u);
    char *s = gwdata_alloc_string(strlen(buf) + 1);
    if (s) strcpy(s, buf);
    return s;
}

char *bistrs_oct(double val) {
    int n = bistrs_conv_int(val);
    if (g_state && g_state->error_code != 0) return NULL;
    unsigned short u = (unsigned short)(n & 0xFFFF);
    char buf[8];
    sprintf(buf, "%o", u);
    char *s = gwdata_alloc_string(strlen(buf) + 1);
    if (s) strcpy(s, buf);
    return s;
}
