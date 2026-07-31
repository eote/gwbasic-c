/* GIOCON - Console I/O */
/* Based on GIOCON.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <windows.h>

/* Console state */
static HANDLE g_console_out = NULL;
static HANDLE g_console_in = NULL;
static CONSOLE_SCREEN_BUFFER_INFO g_orig_console_info;
static bool g_console_initialized = false;

void giocon_init(void) {
    if (g_console_initialized) return;
    
    g_console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_console_in = GetStdHandle(STD_INPUT_HANDLE);
    
    if (g_console_out != INVALID_HANDLE_VALUE) {
        GetConsoleScreenBufferInfo(g_console_out, &g_orig_console_info);
    }
    
    g_console_initialized = true;
}

void giocon_print_num(float val) {
    if (!g_console_initialized) giocon_init();
    printf("%g", val);
}

void giocon_print_str(const char *str) {
    if (!g_console_initialized) giocon_init();
    if (str) {
        fputs(str, stdout);
    }
}

void giocon_locate(int row, int col) {
    if (!g_console_initialized) giocon_init();
    
    if (g_console_out != INVALID_HANDLE_VALUE) {
        COORD pos;
        pos.X = (short)(col - 1);  /* 0-based */
        pos.Y = (short)(row - 1);  /* 0-based */
        SetConsoleCursorPosition(g_console_out, pos);
    }
}

void giocon_color(int fg, int bg) {
    if (!g_console_initialized) giocon_init();
    
    if (g_console_out != INVALID_HANDLE_VALUE) {
        WORD attr = 0;
        
        /* Foreground colors */
        switch (fg & 0x0F) {
            case 0: attr |= 0; break;           /* Black */
            case 1: attr |= FOREGROUND_BLUE; break;
            case 2: attr |= FOREGROUND_GREEN; break;
            case 3: attr |= FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            case 4: attr |= FOREGROUND_RED; break;
            case 5: attr |= FOREGROUND_RED | FOREGROUND_BLUE; break;
            case 6: attr |= FOREGROUND_RED | FOREGROUND_GREEN; break;
            case 7: attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            case 8: attr |= FOREGROUND_INTENSITY; break;           /* Bright Black */
            case 9: attr |= FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case 10: attr |= FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case 11: attr |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case 12: attr |= FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case 13: attr |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case 14: attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case 15: attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        }
        
        /* Background colors */
        switch (bg & 0x0F) {
            case 0: break;  /* Black */
            case 1: attr |= BACKGROUND_BLUE; break;
            case 2: attr |= BACKGROUND_GREEN; break;
            case 3: attr |= BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case 4: attr |= BACKGROUND_RED; break;
            case 5: attr |= BACKGROUND_RED | BACKGROUND_BLUE; break;
            case 6: attr |= BACKGROUND_RED | BACKGROUND_GREEN; break;
            case 7: attr |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
        }
        
        SetConsoleTextAttribute(g_console_out, attr);
    }
}

void giocon_cls(void) {
    if (!g_console_initialized) giocon_init();
    
    if (g_console_out != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(g_console_out, &csbi);
        
        DWORD count;
        COORD home = {0, 0};
        DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
        
        FillConsoleOutputCharacter(g_console_out, ' ', size, home, &count);
        FillConsoleOutputAttribute(g_console_out, csbi.wAttributes, size, home, &count);
        SetConsoleCursorPosition(g_console_out, home);
    }
}