/* ADVGRP - Advanced Generalized Graphics */
/* Based on ADVGRP.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* External references from gengrp/gioscn */
extern void gengrp_init(void);
extern void gengrp_pset(int x, int y, int color);
extern void gengrp_preset(int x, int y);
extern int gengrp_point(int x, int y);
extern void gengrp_direct_pset(int x, int y, int color);
extern int g_fb_width;
extern int g_fb_height;
extern int g_screen_mode;
extern unsigned char *g_framebuffer;

/* External references from gweval */
extern float gweval_eval_numeric(const char *expr);

/* Graphics state */
static int g_grpacx = 0;
static int g_grpacy = 0;
static int g_gxpos = 0;
static int g_gypos = 0;
static int g_atrbyt = 0;
static int g_aspect = 256;  /* Default aspect ratio 1.0 = 256/256 */
static int g_cscale = 0;   /* CSCLXY - 0 = scale Y, 1 = scale X */
static int g_clinef = 0;   /* CLINEF - center line flags */
static int g_cplotf = 0;   /* CPLOTF - plot polarity flag */
static int g_cstcnt = 0;   /* CSTCNT - start count */
static int g_cencnt = 0;   /* CENCNT - end count */
static int g_cnpnts = 0;   /* CNPNTS - number of points per octant */
static int g_cpcnt = 0;    /* CPCNT - current point count */
static int g_cpcnt8 = 0;   /* CPCNT8 - point count for octant 8 */
static int g_croff = 0;    /* CXOFF - X offset */
static int g_cyoff = 0;    /* CYOFF - Y offset */
static int g_crcsum = 0;   /* CRCSUM - circle sum */
static int g_cxoff = 0;    /* CXOFF - X offset */
static int g_cyoff2 = 0;   /* CYOFF - Y offset (second) */
static int g_minx = 0;     /* MINDEL - min X delta */
static int g_maxx = 0;     /* MAXDEL - max X delta */
static int g_miny = 0;     /* MINDEL - min Y delta */
static int g_maxy = 0;     /* MAXDEL - max Y delta */

/* Paint queue (simplified) */
#define MAX_QUEUE 1000
static int queue_len = 0;
static int queue_in = 0;
static int queue_out = 0;
static int psnlen = 0;

/* Module initialization */
void advgrp_init(void) {
    /* Initialize default values */
    g_grpacx = g_fb_width / 2;
    g_grpacy = g_fb_height / 2;
    g_gxpos = g_grpacx;
    g_gypos = g_grpacy;
    g_atrbyt = 1;
    g_aspect = 256;
    g_cscale = 0;
    g_clinef = 0;
    g_cplotf = 0;
    g_cstcnt = 0;
    g_cencnt = 0;
    g_cnpnts = 0;
    g_cpcnt = 0;
    g_cpcnt8 = 0;
    g_croff = 0;
    g_cyoff = 0;
    g_crcsum = 0;
    g_cxoff = 0;
    g_cyoff2 = 0;
    g_minx = 0;
    g_maxx = 0;
    g_miny = 0;
    g_maxy = 0;
    
    /* Initialize queue */
    queue_len = MAX_QUEUE * 6;
    psnlen = 0;
    queue_in = 0;
    queue_out = 0;
}

/* PAINT - Flood fill algorithm */
/* Syntax: PAINT (x,y)[,fill_color[,border_color]] */
void advgrp_paint(int x, int y, int fill_color, int border_color) {
    if (g_screen_mode == 0) return;  /* Text mode - no graphics */
    
    /* Initialize graphics if needed */
    gengrp_init();
    
    /* Default fill color to current attribute */
    if (fill_color < 0) fill_color = g_atrbyt;
    
    /* Default border color to fill color */
    if (border_color < 0) border_color = fill_color;
    
    /* Check bounds */
    if (x < 0 || x >= g_fb_width || y < 0 || y >= g_fb_height) return;
    
    /* Check if starting point is already border color */
    if (g_framebuffer && g_framebuffer[y * MAX_WIDTH + x] == (unsigned char)border_color) return;
    
    /* Simple flood fill using stack */
    typedef struct { int x, y; } Point;
    Point *stack = (Point *)malloc(g_fb_width * g_fb_height * sizeof(Point));
    if (!stack) return;
    
    int stack_top = 0;
    stack[stack_top++] = (Point){x, y};
    
    unsigned char target_color = g_framebuffer[y * MAX_WIDTH + x];
    
    while (stack_top > 0) {
        Point p = stack[--stack_top];
        
        /* Check bounds */
        if (p.x < 0 || p.x >= g_fb_width || p.y < 0 || p.y >= g_fb_height) continue;
        
        unsigned char current = g_framebuffer[p.y * MAX_WIDTH + p.x];
        if (current != target_color || current == (unsigned char)border_color) continue;
        
        /* Fill */
        g_framebuffer[p.y * MAX_WIDTH + p.x] = (unsigned char)fill_color;
        
        /* Add neighbors */
        if (stack_top < g_fb_width * g_fb_height - 4) {
            stack[stack_top++] = (Point){p.x + 1, p.y};
            stack[stack_top++] = (Point){p.x - 1, p.y};
            stack[stack_top++] = (Point){p.x, p.y + 1};
            stack[stack_top++] = (Point){p.x, p.y - 1};
        }
    }
    
    free(stack);
}

/* CIRCLE - Draw circle with aspect ratio and angles */
/* Syntax: CIRCLE (x,y),radius[,color[,start_angle[,end_angle[,aspect]]]] */
void advgrp_circle(int x, int y, int radius, int color, float start_angle, float end_angle, float aspect) {
    (void)start_angle;
    (void)end_angle;
    (void)aspect;
    if (g_screen_mode == 0) return;  /* Text mode */
    if (radius <= 0) return;
    
    gengrp_init();
    
    if (color < 0) color = g_atrbyt;
    
    /* Simple circle drawing using midpoint algorithm */
    int x0 = x;
    int y0 = y;
    int r = radius;
    
    int draw_x = r;
    int draw_y = 0;
    int err = 1 - r;
    
    while (draw_x >= draw_y) {
        /* 8-way symmetry - draw all 8 points */
        int points[8][2] = {
            {x0 + draw_x, y0 + draw_y},
            {x0 + draw_y, y0 + draw_x},
            {x0 - draw_y, y0 + draw_x},
            {x0 - draw_x, y0 + draw_y},
            {x0 - draw_x, y0 - draw_y},
            {x0 - draw_y, y0 - draw_x},
            {x0 + draw_y, y0 - draw_x},
            {x0 + draw_x, y0 - draw_y}
        };
        
        for (int i = 0; i < 8; i++) {
            if (points[i][0] >= 0 && points[i][0] < g_fb_width && 
                points[i][1] >= 0 && points[i][1] < g_fb_height) {
                gengrp_pset(points[i][0], points[i][1], color);
            }
        }
        
        draw_y++;
        if (err < 0) {
            err += 2 * draw_y + 1;
        } else {
            draw_x--;
            err += 2 * (draw_y - draw_x + 1);
        }
    }
}

/* GET - Read graphics area into array */
/* Syntax: GET (x1,y1)-(x2,y2), array_name */
int advgrp_get(int x1, int y1, int x2, int y2, const char *array_name) {
    if (g_screen_mode == 0) return ERROR;
    if (!array_name || array_name[0] == 0) {
        return ERROR;
    }
    
    /* Ensure x1 <= x2, y1 <= y2 */
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    
    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    
    if (width <= 0 || height <= 0) return ERROR;
    
    /* Calculate array size */
    int bits_per_pixel = (g_screen_mode == 2) ? 1 : 2;
    int total_pixels = width * height;
    int data_bits = total_pixels * bits_per_pixel;
    int data_bytes = (data_bits + 7) / 8;
    int total_bytes = data_bytes + 4;  /* +4 for header */
    
    /* Find or create array variable */
    struct variable *v = gwdata_find_variable(array_name);
    if (!v) {
        v = gwdata_alloc_variable(array_name, VT_STRING);
        if (!v) return ERROR;
    }
    
    if (!v->is_array) {
        /* Allocate string buffer */
        if (v->type == VT_STRING && v->value.s.ptr) {
            gwram_free(v->value.s.ptr);
        }
        v->value.s.ptr = (char *)gwram_alloc(total_bytes + 2);  /* +2 for safety + null */
        if (!v->value.s.ptr) return ERROR;
        v->value.s.len = total_bytes + 2;
        v->type = VT_STRING;
        v->is_array = false;  /* Using string for binary data */
    }
    
    /* Store header: width, height, data_bytes */
    char *buf = v->value.s.ptr;
    buf[0] = (width >> 8) & 0xFF;
    buf[1] = width & 0xFF;
    buf[2] = (height >> 8) & 0xFF;
    buf[3] = height & 0xFF;
    
    /* Read pixel data */
    int pos = 4;
    int bit = 7;
    unsigned char byte = 0;
    int bit2 = 6;
    unsigned char byte2 = 0;

    for (int y = y1; y <= y2 && pos < total_bytes; y++) {
        for (int x = x1; x <= x2 && pos < total_bytes; x++) {
            int pixel = gengrp_point(x, y);
            if (g_screen_mode == 2) {
                /* 1 bit per pixel */
                if (pixel) byte |= (1 << bit);
                bit--;
                if (bit < 0) {
                    if (pos < total_bytes) buf[pos++] = byte;
                    byte = 0;
                    bit = 7;
                }
            } else {
                /* 2 bits per pixel - pack continuously across rows */
                byte2 |= ((pixel & 3) << bit2);
                bit2 -= 2;
                if (bit2 < 0) {
                    if (pos < total_bytes) buf[pos++] = byte2;
                    byte2 = 0;
                    bit2 = 6;
                }
            }
        }
    }
    /* Flush remaining bits at the very end */
    if (g_screen_mode == 2) {
        if (bit != 7 && pos < total_bytes) buf[pos++] = byte;
    } else {
        if (bit2 != 6 && pos < total_bytes) buf[pos++] = byte2;
    }
    
    return SUCCESS;
}

/* PUT - Write graphics array to screen */
/* Syntax: PUT (x,y), array_name[, action] */
/* action: PSET, PRESET, AND, OR, XOR */
int advgrp_put(int x, int y, const char *array_name, int action) {
    if (g_screen_mode == 0) return ERROR;
    if (!array_name || array_name[0] == 0) return ERROR;
    
    /* Default action is XOR */
    if (action < 0) action = 5;  /* XOR */
    
    struct variable *v = gwdata_find_variable(array_name);
    if (!v || v->type != VT_STRING || !v->value.s.ptr) return ERROR;
    
    char *buf = v->value.s.ptr;
    
    /* Read header */
    int width = (buf[0] << 8) | buf[1];
    int height = (buf[2] << 8) | buf[3];
    
    if (width <= 0 || height <= 0) return ERROR;
    
    /* Calculate proper buffer length from header info */
    int bits_per_pixel = (g_screen_mode == 2) ? 1 : 2;
    int total_pixels = width * height;
    int data_bits = total_pixels * bits_per_pixel;
    int data_bytes = (data_bits + 7) / 8;
    int max_data_len = 4 + data_bytes;
    
    /* Use the calculated data length instead of strlen (binary data may contain null bytes) */
    int buf_len = max_data_len;
    
    int pos = 4;
    int bit = 7;
    unsigned char byte = 0;
    int bit2 = 6;
    unsigned char byte2 = 0;
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int pixel = 0;
            if (g_screen_mode == 2) {
                /* 1 bit per pixel */
                if (bit == 7) {
                    if (pos < buf_len) byte = buf[pos++];
                    else byte = 0;
                }
                pixel = (byte >> bit) & 1;
                bit--;
            } else {
                /* 2 bits per pixel - continuous packing */
                if (bit2 == 6) {
                    if (pos < buf_len) byte2 = buf[pos++];
                    else byte2 = 0;
                }
                pixel = (byte2 >> bit2) & 3;
                bit2 -= 2;
                if (bit2 < 0) {
                    bit2 = 6;
                }
            }
            
            /* Apply action */
            int sx = x + dx;
            int sy = y + dy;
            
            switch (action) {
                case 1:  /* PSET */
                    gengrp_direct_pset(sx, sy, pixel);
                    break;
                case 2:  /* PRESET */
                    gengrp_preset(sx, sy);
                    break;
                case 3:  /* AND */
                    {
                        int cur = gengrp_point(sx, sy);
                        gengrp_direct_pset(sx, sy, cur & pixel);
                    }
                    break;
                case 4:  /* OR */
                    {
                        int cur = gengrp_point(sx, sy);
                        gengrp_direct_pset(sx, sy, cur | pixel);
                    }
                    break;
                case 5:  /* XOR */
                    {
                        int cur = gengrp_point(sx, sy);
                        gengrp_direct_pset(sx, sy, cur ^ pixel);
                    }
                    break;
                default:
                    gengrp_direct_pset(sx, sy, pixel);  /* Default to PSET */
                    break;
            }
        }
    }
    
    return SUCCESS;
}