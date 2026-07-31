/* BISTRS - String Functions */
/* Based on BISTRS.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* MID$ - return substring from start position (1-based) of length len */
char *bistrs_mid(const char *str, int start, int len) {
    if (!str || start <= 0 || len <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    int str_len = strlen(str);
    if (start > str_len) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    /* Adjust to 0-based index */
    start--;
    if (start + len > str_len) len = str_len - start;
    
    char *result = (char *)gwram_alloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, str + start, len);
    result[len] = '\0';
    return result;
}

/* LEN - return length of string */
int bistrs_len(const char *str) {
    if (!str) return 0;
    return (int)strlen(str);
}

/* ASC - return ASCII code of first character */
int bistrs_asc(const char *str) {
    if (!str || str[0] == '\0') return 0;
    return (unsigned char)str[0];
}

/* CHR$ - return string of single character with given ASCII code */
char *bistrs_chr(int code) {
    char *result = (char *)gwram_alloc(2);
    if (!result) return NULL;
    
    result[0] = (char)(code & 0xFF);
    result[1] = '\0';
    return result;
}

/* STR$ - convert number to string */
char *bistrs_str(double val) {
    char *result = (char *)gwram_alloc(32);
    if (!result) return NULL;
    
    sprintf(result, "%g", val);
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