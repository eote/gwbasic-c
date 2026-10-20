/* GENGRP - Generalized Graphics */
/* Based on GENGRP.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* Graphics state */
static int g_gxpos = 0;
static int g_gypos = 0;
static int g_grpacx = 0;
static int g_grpacy = 0;
static int g_maxdel = 0;
static int g_mindel = 0;
static int g_minupd = 0;
static int g_maxupd = 0;
static int g_atrbyt = 0;   /* Current attribute */
static int g_forclr = 1;   /* Foreground color (default) */
static int g_bakclr = 0;   /* Background color (default) */
static bool g_graphics_initialized = false;

/* Screen dimensions (from gioscn) */
extern int g_fb_width;
extern int g_fb_height;
extern int g_screen_mode;
extern unsigned char *g_framebuffer;

/* MAX_WIDTH must match gioiscn.c - used as framebuffer stride */
#define MAX_WIDTH 640

/* Local framebuffer for testing when external not available */
static unsigned char *g_local_framebuffer = NULL;
static int g_local_fb_width = 640;
static int g_local_fb_height = 480;

/* Forward declarations */
static void draw_line_bresenham(int x0, int y0, int x1, int y1, int color);
static int scale_coord(int *x, int *y);
static void map_xyc(void);
static unsigned char* get_framebuffer(void);

/* Module initialization */
void gengrp_init(void) {
    /* Initialize default colors */
    g_forclr = 1;
    g_bakclr = 0;
    g_atrbyt = g_forclr;
    
    /* Set default screen mode for testing (320x200 - 4 colors) */
    g_screen_mode = 1;
    
    /* Allocate local framebuffer for testing */
    if (!g_local_framebuffer) {
        g_local_framebuffer = (unsigned char*)calloc(g_local_fb_width * g_local_fb_height, 1);
    }
    
    /* Reset graphics state */
    gengrp_grprst();
}

/* Initialize graphics - center cursor */
void gengrp_grpini(void) {
    if (!g_graphics_initialized) {
        /* In full implementation, would call OEM GRPSIZ to get screen dimensions */
        /* For now, use framebuffer dimensions from gioscn */
    }
    
    /* Center the graphics cursor */
    g_grpacx = g_fb_width / 2;
    g_grpacy = g_fb_height / 2;
    g_gxpos = g_grpacx;
    g_gypos = g_grpacy;
    
    /* Initialize draw scale and angle */
    g_maxdel = 0;
    g_mindel = 0;
    g_minupd = 0;
    g_maxupd = 0;
    
    /* Get foreground/background colors and set default attribute */
    gengrp_set_atr(g_forclr);
}

/* Reset graphics - called from CLEAR, NEW, etc. */
void gengrp_grprst(void) {
    g_maxdel = 0;
    g_mindel = 0;
    g_minupd = 0;
    g_maxupd = 0;
    gengrp_grpini();
}

/* Scan a coordinate pair - handles (x,y) or STEP(x,y) */
void gengrp_scan_coord(int *x, int *y, bool relative) {
    /* This is a simplified version - in full implementation would parse from text pointer */
    /* For now, just update the graphics accumulator */
    if (relative) {
        g_grpacx += *x;
        g_grpacy += *y;
    } else {
        g_grpacx = *x;
        g_grpacy = *y;
    }
    
    *x = g_grpacx;
    *y = g_grpacy;
    g_gxpos = g_grpacx;
    g_gypos = g_grpacy;
}

/* PSET (x,y)[,color] - set pixel to foreground color (or specified color) */
void gengrp_pset(int x, int y, int color) {
    if (g_screen_mode == 0) return;  /* Text mode - no graphics */
    
    /* Scan coordinate if needed */
    int px = x;
    int py = y;
    gengrp_scan_coord(&px, &py, false);
    
    /* Scale into bounds */
    if (scale_coord(&px, &py) != 0) return;  /* Out of bounds */
    
    /* Map to framebuffer coordinates */
    map_xyc();
    
    /* Set attribute if provided */
    if (color >= 0) {
        gengrp_set_atr(color);
    } else {
        gengrp_set_atr(g_forclr);
    }
    
    /* Actually set the pixel - use MAX_WIDTH stride for consistency with gioiscn */
    unsigned char *fb = get_framebuffer();
    if (fb && px >= 0 && py >= 0) {
        fb[py * MAX_WIDTH + px] = (unsigned char)g_atrbyt;
    }
}

/* PRESET (x,y)[,color] - set pixel to background color (or specified color)
 * v138 M2: color < 0 = default BAKCLR (ASM PRESET 默认 BAKCLR) */
void gengrp_preset(int x, int y, int color) {
    gengrp_pset(x, y, (color < 0) ? g_bakclr : color);
}

/* POINT (x,y) - return attribute at point, or -1 if out of bounds */
int gengrp_point(int x, int y) {
    if (g_screen_mode == 0) return -1;  /* Text mode */
    
    /* Direct coordinate - do NOT call scan_coord (which modifies graphics cursor) */
    int px = x;
    int py = y;
    
    /* Scale into bounds */
    if (scale_coord(&px, &py) != 0) return -1;  /* Out of bounds */
    
    /* Read attribute - use MAX_WIDTH stride for consistency with gioiscn */
    unsigned char *fb = get_framebuffer();
    if (fb && px >= 0 && py >= 0) {
        return (int)fb[py * MAX_WIDTH + px];
    }
    
    return -1;
}

/* Set current attribute (color) */
void gengrp_set_atr(int color) {
    if (g_screen_mode == 1) {  /* 320x200 - 4 colors */
        color = color & 0x03;
    } else if (g_screen_mode == 2) {  /* 640x200 - 2 colors */
        color = (color != 0) ? 1 : 0;
    }
    g_atrbyt = color;
}

/* Direct pixel write - bypasses coordinate scanning for PUT */
void gengrp_direct_pset(int x, int y, int color) {
    if (g_screen_mode == 0) return;  /* Text mode - no graphics */
    
    /* Scale into bounds */
    if (x < 0 || y < 0) return;
    
    int fb_w = g_fb_width > 0 ? g_fb_width : g_local_fb_width;
    int fb_h = g_fb_height > 0 ? g_fb_height : g_local_fb_height;
    
    if (x >= fb_w || y >= fb_h) return;
    
    /* Set attribute */
    gengrp_set_atr(color);
    
    /* Actually set the pixel - use MAX_WIDTH stride */
    unsigned char *fb = get_framebuffer();
    if (fb) {
        fb[y * MAX_WIDTH + x] = (unsigned char)g_atrbyt;
    }
}

/* Foreground/background color accessors (v138 M2) */
int gengrp_get_forclr(void) { return g_forclr; }
int gengrp_get_bakclr(void) { return g_bakclr; }

/* Get current attribute */
int gengrp_get_atr(void) {
    return g_atrbyt;
}

/* Accessor functions for testing */
int gengrp_get_gxpos(void) { return g_gxpos; }
int gengrp_get_gypos(void) { return g_gypos; }
int gengrp_get_grpacx(void) { return g_grpacx; }
int gengrp_get_grpacy(void) { return g_grpacy; }
int gengrp_get_screen_mode(void) { return g_screen_mode; }
int gengrp_get_fb_width(void) { return g_fb_width > 0 ? g_fb_width : g_local_fb_width; }
int gengrp_get_fb_height(void) { return g_fb_height > 0 ? g_fb_height : g_local_fb_height; }
void gengrp_set_screen_mode(int mode) {
    g_screen_mode = mode;
    /* v143: keep framebuffer dimensions in sync (same semantics as
     * gioscn_screen; ASM SCRN is the single entry for mode changes —
     * advgrp boundary checks read g_fb_width/g_fb_height) */
    switch (mode) {
        case 0: g_fb_width = 80;  g_fb_height = 25;  break;
        case 1: g_fb_width = 320; g_fb_height = 200; break;
        case 2: g_fb_width = 640; g_fb_height = 200; break;
    }
}
void gengrp_set_grpacx(int x) { g_grpacx = x; }
void gengrp_set_grpacy(int y) { g_grpacy = y; }
void gengrp_set_gxpos(int x) { g_gxpos = x; }
void gengrp_set_gypos(int y) { g_gypos = y; }
/* v142-gfx: clear framebuffer (test isolation; screen-fill equivalent) */
void gengrp_fb_clear(void) {
    unsigned char *fb = get_framebuffer();
    int w = (g_fb_width > 0 ? g_fb_width : g_local_fb_width);
    int h = (g_fb_height > 0 ? g_fb_height : g_local_fb_height);
    if (fb && w > 0 && h > 0) memset(fb, 0, (size_t)w * (size_t)h);
}

/* Get framebuffer (external or local) */
static unsigned char* get_framebuffer(void) {
    if (g_framebuffer) return g_framebuffer;
    if (g_local_framebuffer) return g_local_framebuffer;
    return NULL;
}

/* Scale coordinate into screen bounds */
static int scale_coord(int *x, int *y) {
    int fb_w = g_fb_width > 0 ? g_fb_width : g_local_fb_width;
    int fb_h = g_fb_height > 0 ? g_fb_height : g_local_fb_height;
    
    if (*x < 0 || *x >= fb_w || *y < 0 || *y >= fb_h) {
        return -1;  /* Out of bounds */
    }
    return 0;
}

/* Map logical coordinates to framebuffer */
static void map_xyc(void) {
    /* In full implementation, would handle viewport/window mapping */
    /* For now, direct mapping */
}

/* Draw line using Bresenham's algorithm */
static void draw_line_bresenham(int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        if (x0 >= 0 && y0 >= 0) {
            int fb_h = g_fb_height > 0 ? g_fb_height : g_local_fb_height;
            if (x0 < MAX_WIDTH && y0 < fb_h) {
                unsigned char *fb = get_framebuffer();
                if (fb) {
                    fb[y0 * MAX_WIDTH + x0] = (unsigned char)color;
                }
            }
        }
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* LINE core execution — coordinate/attr already resolved by caller
 * (v138 M2: gwsts 表达式求值后调用; box=画框, box_fill=填充)
 * ASM GENGRP: 省略首坐标用当前 graphics cursor; 执行后 cursor=(x2,y2) */
void gengrp_line_coords(int x1, int y1, int x2, int y2, int color, int box, int box_fill) {
    if (g_screen_mode == 0) return;
    /* 首坐标成为当前 graphics cursor (ASM: SCAND 设置 GXPOS/GYPOS) */
    g_gxpos = x1;
    g_gypos = y1;
    g_grpacx = x1;
    g_grpacy = y1;
    if (box) {
        if (box_fill) {
            int x_min = (x1 < x2) ? x1 : x2;
            int x_max = (x1 > x2) ? x1 : x2;
            int y_min = (y1 < y2) ? y1 : y2;
            int y_max = (y1 > y2) ? y1 : y2;
            for (int y = y_min; y <= y_max; y++) {
                for (int x = x_min; x <= x_max; x++) {
                    gengrp_pset(x, y, color);
                }
            }
        } else {
            draw_line_bresenham(x1, y1, x2, y1, color);
            draw_line_bresenham(x2, y1, x2, y2, color);
            draw_line_bresenham(x2, y2, x1, y2, color);
            draw_line_bresenham(x1, y2, x1, y1, color);
        }
    } else {
        draw_line_bresenham(g_gxpos, g_gypos, x2, y2, color);
    }
    /* ASM: 终点成为新 graphics cursor */
    g_gxpos = x2;
    g_gypos = y2;
    g_grpacx = x2;
    g_grpacy = y2;
}

/* LINE command - parse and execute LINE statement */
void gengrp_line(const char *params) {
    if (!params || g_screen_mode == 0) return;
    
    /* This is a simplified parser for LINE command */
    /* Full syntax: LINE [(x1,y1)]-(x2,y2) [,color[,B[F]]] */
    
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int color = g_forclr;
    bool box = false;
    bool box_fill = false;
    bool has_first_coord = false;
    
    /* Parse parameters - simplified implementation */
    const char *p = params;
    
    /* Skip leading whitespace */
    while (*p && isspace((unsigned char)*p)) p++;
    
    /* Check for first coordinate (x1,y1) */
    if (*p == '(') {
        has_first_coord = true;
        p++; /* skip '(' */
        
        /* Parse x1 */
        while (*p && isspace((unsigned char)*p)) p++;
        x1 = (int)strtol(p, (char **)&p, 10);
        
        /* Skip comma */
        while (*p && (*p == ',' || isspace((unsigned char)*p))) p++;
        
        /* Parse y1 */
        y1 = (int)strtol(p, (char **)&p, 10);
        
        /* Skip ')' */
        while (*p && *p != ')') p++;
        if (*p == ')') p++;
        
        /* Skip '-' */
        while (*p && (*p == '-' || isspace((unsigned char)*p))) p++;
    }
    
    /* Parse second coordinate (x2,y2) */
    if (*p == '(') {
        p++; /* skip '(' */
        
        /* Parse x2 */
        while (*p && isspace((unsigned char)*p)) p++;
        x2 = (int)strtol(p, (char **)&p, 10);
        
        /* Skip comma */
        while (*p && (*p == ',' || isspace((unsigned char)*p))) p++;
        
        /* Parse y2 */
        y2 = (int)strtol(p, (char **)&p, 10);
        
        /* Skip ')' */
        while (*p && *p != ')') p++;
        if (*p == ')') p++;
    }
    
    /* Check for color attribute */
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ',') {
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        color = (int)strtol(p, (char **)&p, 10);
    }
    
    /* Check for B or BF */
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ',') {
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == 'B' || *p == 'b') {
            box = true;
            p++;
            if (*p == 'F' || *p == 'f') {
                box_fill = true;
            }
        }
    }
    
    /* Execute LINE command */
    if (has_first_coord) {
        /* Use provided first coordinate */
        g_grpacx = x1;
        g_grpacy = y1;
        g_gxpos = x1;
        g_gypos = y1;
    }
    /* Otherwise use current graphics cursor position */
    
    if (box) {
        /* Draw rectangle: (x1,y1) and (x2,y2) are opposite corners */
        if (box_fill) {
            /* Filled rectangle */
            int x_min = (x1 < x2) ? x1 : x2;
            int x_max = (x1 > x2) ? x1 : x2;
            int y_min = (y1 < y2) ? y1 : y2;
            int y_max = (y1 > y2) ? y1 : y2;
            
            for (int y = y_min; y <= y_max; y++) {
                for (int x = x_min; x <= x_max; x++) {
                    gengrp_pset(x, y, color);
                }
            }
        } else {
            /* Rectangle outline - draw 4 lines */
            gengrp_pset(x1, y1, color);
            draw_line_bresenham(x1, y1, x2, y1, color);  /* Top */
            draw_line_bresenham(x2, y1, x2, y2, color);  /* Right */
            draw_line_bresenham(x2, y2, x1, y2, color);  /* Bottom */
            draw_line_bresenham(x1, y2, x1, y1, color);  /* Left */
        }
    } else {
        /* Simple line from (x1,y1) (cursor) to (x2,y2) */
        draw_line_bresenham(x1, y1, x2, y2, color);
    }
    
    /* Update graphics cursor to end point */
    g_gxpos = x2;
    g_gypos = y2;
    g_grpacx = x2;
    g_grpacy = y2;
}