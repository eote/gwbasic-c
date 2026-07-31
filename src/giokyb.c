/* GIOKYB - Keyboard Input */
/* Based on GIOKYB.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <conio.h>

/* Keyboard state */
static bool g_kbhit_checked = false;
static int g_last_key = 0;

void giokyb_init(void) {
    g_kbhit_checked = false;
    g_last_key = 0;
}

/* INKEY$ - returns one character if available, empty string otherwise */
char *giokyb_inkey(void) {
    if (_kbhit()) {
        int ch = _getch();
        char *result = (char *)gwram_alloc(2);
        if (result) {
            result[0] = (char)ch;
            result[1] = '\0';
        }
        return result;
    }
    
    /* Return empty string */
    char *result = (char *)gwram_alloc(1);
    if (result) result[0] = '\0';
    return result;
}

/* INPUT$ - reads n characters from keyboard */
char *giokyb_input(int n) {
    if (n <= 0) {
        char *result = (char *)gwram_alloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    char *result = (char *)gwram_alloc(n + 1);
    if (!result) return NULL;
    
    for (int i = 0; i < n; i++) {
        result[i] = (char)_getch();
    }
    result[n] = '\0';
    return result;
}