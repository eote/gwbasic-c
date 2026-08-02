/* GIOSCN - Screen/Graphics Handling */
/* Based on GIOSCN.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <windows.h>
#include <string.h>

/* Screen state */
int g_screen_mode = 0;  /* 0 = text, 1 = 320x200, 2 = 640x200 */
static HANDLE g_console_out = NULL;
static bool g_graphics_initialized = false;

/* Simple framebuffer for graphics modes (simulated) */
#define MAX_WIDTH  640
#define MAX_HEIGHT 480
unsigned char *g_framebuffer = NULL;
int g_fb_width = 0;
int g_fb_height = 0;

static void ensure_graphics_init(void) {
    if (g_graphics_initialized) return;
    
    g_console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_framebuffer = (unsigned char *)gwram_calloc(MAX_WIDTH * MAX_HEIGHT, sizeof(unsigned char));
    g_graphics_initialized = true;
}

void gioscn_init(void) {
    ensure_graphics_init();
    g_screen_mode = 0;
}

void gioscn_screen(int mode) {
    ensure_graphics_init();
    
    if (mode < 0 || mode > 2) mode = 0;
    
    g_screen_mode = mode;
    
    switch (mode) {
        case 0:  /* Text mode 80x25 */
            g_fb_width = 80;
            g_fb_height = 25;
            /* Reset console to text mode */
            system("cls");
            break;
        case 1:  /* 320x200 graphics (4 colors) */
            g_fb_width = 320;
            g_fb_height = 200;
            break;
        case 2:  /* 640x200 graphics (2 colors) */
            g_fb_width = 640;
            g_fb_height = 200;
            break;
    }
    
    /* Clear framebuffer */
    if (g_framebuffer) {
        memset(g_framebuffer, 0, MAX_WIDTH * MAX_HEIGHT);
    }
}

void gioscn_pset(int x, int y, int color) {
    ensure_graphics_init();
    
    if (x < 0 || x >= g_fb_width || y < 0 || y >= g_fb_height) return;
    if (!g_framebuffer) return;
    
    /* Clamp color based on mode */
    if (g_screen_mode == 2) {
        color = (color != 0) ? 1 : 0;  /* 2 colors */
    } else if (g_screen_mode == 1) {
        color = color & 0x03;  /* 4 colors */
    }
    
    g_framebuffer[y * MAX_WIDTH + x] = (unsigned char)color;
}

void gioscn_preset(int x, int y) {
    gioscn_pset(x, y, 0);  /* Reset to background color (0) */
}

int gioscn_point(int x, int y) {
    ensure_graphics_init();
    
    if (x < 0 || x >= g_fb_width || y < 0 || y >= g_fb_height) return -1;
    if (!g_framebuffer) return -1;
    
    return (int)g_framebuffer[y * MAX_WIDTH + x];
}