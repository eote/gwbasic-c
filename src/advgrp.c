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
extern void gengrp_preset(int x, int y, int color);
extern int gengrp_point(int x, int y);
extern void gengrp_direct_pset(int x, int y, int color);
extern int g_fb_width;
extern int g_fb_height;
extern int g_screen_mode;
extern unsigned char *g_framebuffer;

/* External references from gweval */
extern double gweval_eval_numeric(const char *expr);

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
    
    /* Check bounds (g_fb_width synced by gengrp_set_screen_mode) */
    int fb_w = gengrp_get_fb_width();
    int fb_h = gengrp_get_fb_height();
    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) return;
    
    /* Check if starting point is already border color
     * (v143: all fb access via gengrp API — single framebuffer source,
     * ASM ADVGRP shares GENGRP's MAPXYC/SETC) */
    unsigned char start_px = (unsigned char)gengrp_point(x, y);
    if (start_px == (unsigned char)border_color) return;
    
    /* Simple flood fill using stack */
    typedef struct { int x, y; } Point;
    Point *stack = (Point *)malloc((size_t)fb_w * (size_t)fb_h * sizeof(Point));
    if (!stack) return;
    
    int stack_top = 0;
    stack[stack_top++] = (Point){x, y};
    
    unsigned char target_color = start_px;
    
    while (stack_top > 0) {
        Point p = stack[--stack_top];
        
        /* Check bounds */
        if (p.x < 0 || p.x >= fb_w || p.y < 0 || p.y >= fb_h) continue;
        
        unsigned char current = (unsigned char)gengrp_point(p.x, p.y);
        if (current != target_color || current == (unsigned char)border_color) continue;
        
        /* Fill */
        gengrp_direct_pset(p.x, p.y, fill_color);
        
        /* Add neighbors */
        if (stack_top < fb_w * fb_h - 4) {
            stack[stack_top++] = (Point){p.x + 1, p.y};
            stack[stack_top++] = (Point){p.x - 1, p.y};
            stack[stack_top++] = (Point){p.x, p.y + 1};
            stack[stack_top++] = (Point){p.x, p.y - 1};
        }
    }
    
    free(stack);
}

/* CIRCLE - Draw circle with aspect ratio and angles */
/* Syntax: CIRCLE (x,y),radius[,color[,start_angle[,end_angle[,aspect]]]]
 * ASM ADVGRP CIRCLE L460-800: angles in radians, 0 = east, CCW; negative
 * angle draws a radius line to center; end < start wraps past 360;
 * aspect < 1 scales Y, aspect > 1 scales X by 1/aspect. Arc selection by
 * point count: total = 8 * (radius*SQR(2)/2) points around the circle. */
#include <math.h>
void advgrp_circle(int x, int y, int radius, int color, float start_angle, float end_angle, float aspect) {
    if (g_screen_mode == 0) return;  /* Text mode */
    if (radius <= 0) return;
    
    gengrp_init();
    
    if (color < 0) color = g_atrbyt;
    
    /* negative angles: draw radius line to center, use absolute value */
    bool line_start = (start_angle < 0.0f);
    bool line_end = (end_angle < 0.0f);
    if (line_start) start_angle = -start_angle;
    if (line_end) end_angle = -end_angle;
    
    /* point counts around circumference (ASM CNPNTS = R*SQR(2)/2) */
    int cnpnts = (int)(radius * 0.70710678f + 0.5f);
    if (cnpnts < 1) cnpnts = 1;
    int total = cnpnts * 8;
    int start_cnt = (int)(start_angle / 6.2831853f * total + 0.5f);
    int end_cnt = (int)(end_angle / 6.2831853f * total + 0.5f);
    
    bool plot_outside = false;
    if (end_cnt < start_cnt) {
        /* ASM CSTPLT: end < start → plot points ABOVE & BELOW (outside) */
        plot_outside = true;
        int t = start_cnt; start_cnt = end_cnt; end_cnt = t;
    }
    
    /* aspect handling (ASM GTASPC/CIRC11): <1 scales Y, >1 scales X */
    float ax = 1.0f, ay = 1.0f;
    if (aspect > 0.0f && aspect != 1.0f) {
        if (aspect < 1.0f) {
            ay = aspect;            /* y compressed */
        } else {
            ax = 1.0f / aspect;     /* x compressed */
        }
    }
    
    int fb_w = gengrp_get_fb_width();
    int fb_h = gengrp_get_fb_height();
    
    /* helper: point-count for offset (dx right, dy up) on octant k */
    #define CIRCLE_PLOT(cx_, cy_) do { \
        int px_ = (cx_), py_ = (cy_); \
        if (px_ >= 0 && px_ < fb_w && py_ >= 0 && py_ < fb_h) \
            gengrp_pset(px_, py_, color); \
    } while (0)
    
    /* midpoint octant walk: 0..45° (dx from r down to dy up) */
    int dx0 = radius, dy0 = 0;
    int err = 1 - radius;
    while (dx0 >= dy0) {
        /* this point's count within the first (east→north) octant */
        int cnt0 = dy0;  /* ASM CPCNT = Y */
        
        /* 8 reflections; octant counts: even octants count up (dy0),
         * odd octants count down from 2*CNPNTS boundary (ASM CPCNT8
         * reversal). Approximate ASM mapping:
         *   oct0 (east→north):   cnt = cnt0
         *   oct1 (north→west):   cnt = 2*CNPNTS - cnt0
         *   oct2 (west→south):   cnt = 2*CNPNTS + cnt0
         *   oct3 (south→east):   cnt = 4*CNPNTS - cnt0
         *   oct4 (east→south):   cnt = 4*CNPNTS + cnt0
         *   oct5 (south→west):   cnt = 6*CNPNTS - cnt0
         *   oct6 (west→north):   cnt = 6*CNPNTS + cnt0
         *   oct7 (north→east):   cnt = 8*CNPNTS - cnt0 */
        int cnt[8] = {
            cnt0,
            2*cnpnts - cnt0,
            2*cnpnts + cnt0,
            4*cnpnts - cnt0,
            4*cnpnts + cnt0,
            6*cnpnts - cnt0,
            6*cnpnts + cnt0,
            8*cnpnts - cnt0
        };
        
        /* offsets with aspect applied (dy up = -y on screen) */
        int ox = (int)(dx0 * ax + 0.5f);
        int oy = (int)(dy0 * ay + 0.5f);
        
        /* reflect 8 ways: (+x,+y_up) (+y_up,+x) (-y_up,+x) (-x,+y_up)
         * (-x,-y_up) (-y_up,-x) (+y_up,-x) (+x,-y_up) */
        int pxs[8] = { x+ox, x+oy, x-oy, x-ox, x-ox, x-oy, x+oy, x+ox };
        int pys[8] = { y-oy, y-ox, y-ox, y-oy, y+oy, y+ox, y+ox, y+oy };
        
        for (int k = 0; k < 8; k++) {
            bool in_range = (cnt[k] >= start_cnt && cnt[k] <= end_cnt);
            if (plot_outside) in_range = !in_range;
            if (in_range) {
                CIRCLE_PLOT(pxs[k], pys[k]);
            }
        }
        
        dy0++;
        if (err < 0) {
            err += 2 * dy0 + 1;
        } else {
            dx0--;
            err += 2 * (dy0 - dx0 + 1);
        }
    }
    #undef CIRCLE_PLOT
    
    /* radius lines for negative angles (ASM CLINE → DOGRPH) */
    if (line_start || line_end) {
        float ang = line_start ? start_angle : end_angle;
        float rad = ang;  /* radians */
        int ex = x + (int)(radius * cosf(rad) * ax + 0.5f);
        int ey = y - (int)(radius * sinf(rad) * ay + 0.5f);
        extern void gengrp_line_coords(int x1, int y1, int x2, int y2, int color, int box, int box_fill);
        gengrp_line_coords(x, y, ex, ey, color, 0, 0);
    }
}

/* ===== v144 CRITICAL-1 fix: graphics GET/PUT storage =====
 * ASM ADVGRP GPUTG L864: the block is stored in a standard NUMERIC array —
 * first 2 elements = x-bits (word) + y-count (word) header, the bitmap
 * follows packed MSB-first across rows (mode 1: 2 bits/pixel, mode 2:
 * 1 bit/pixel). Array elements are 32-bit ints in our port (ASM used
 * whatever element size the array had; we normalize to 4 bytes/element,
 * matching DIM default single-precision storage). Capacity shortfall →
 * FCERR (Illegal function call). String variables/arrays → FCERR. */

extern int gwdata_set_error_ext(int code);

static int advgrp_gfx_fcerr(void) {
    /* FCERR = Illegal function call, code 5 */
    extern uint16_t g_curlin;
    gwdata_set_error(5, g_curlin, 0);
    return ERROR;
}

/* element size in bytes for the array's declared type */
static int advgrp_elem_size(var_type_t t) {
    return (t == VT_INTEGER) ? 4 : (t == VT_DOUBLE) ? 8 : 4;
}

int advgrp_get(int x1, int y1, int x2, int y2, const char *array_name) {
    if (g_screen_mode == 0) return ERROR;
    if (!array_name || array_name[0] == 0) return ERROR;
    
    /* Ensure x1 <= x2, y1 <= y2 */
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    
    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if (width <= 0 || height <= 0) return ERROR;
    
    /* target must be a numeric ARRAY (string var/array → FCERR).
     * Unknown name → implicit DIM(10) (GW-BASIC: arrays under 11 elements
     * need no DIM) — 11 elements * 4B = 44B capacity check still applies */
    struct variable *v = biptrg_ensure_array(array_name);
    if (!v || !v->is_array || v->type == VT_STRING) {
        return advgrp_gfx_fcerr();
    }
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc || desc->magic != ARRAY_MAGIC || !desc->data) {
        return advgrp_gfx_fcerr();
    }
    
    /* required bytes: 4 header + bitmap (MSB-first, rows continuous) */
    int bits_per_pixel = (g_screen_mode == 2) ? 1 : 2;
    int data_bits = width * height * bits_per_pixel;
    int data_bytes = (data_bits + 7) / 8;
    int need_bytes = 4 + data_bytes;
    int elem_size = advgrp_elem_size(desc->type);
    int cap_bytes = desc->size * elem_size;
    if (need_bytes > cap_bytes) {
        return advgrp_gfx_fcerr();  /* ASM: array too small → FCERR */
    }
    
    unsigned char *buf = (unsigned char *)desc->data;
    
    /* header: x-bits (word) + y-count (word) — ASM stores CL/CH then
     * MINDEL word at the first 4 bytes of the array data */
    int xbits = width * bits_per_pixel;
    buf[0] = (unsigned char)(xbits & 0xFF);
    buf[1] = (unsigned char)((xbits >> 8) & 0xFF);
    buf[2] = (unsigned char)(height & 0xFF);
    buf[3] = (unsigned char)((height >> 8) & 0xFF);
    
    /* bitmap: MSB-first, packed continuously across rows.
     * ASM GET overwrites (not OR) — clear the bitmap region first so
     * stale array values don't leak into the packed data. */
    memset(buf + 4, 0, (size_t)data_bytes);
    int bitpos = 0;
    for (int yy = y1; yy <= y2; yy++) {
        for (int xx = x1; xx <= x2; xx++) {
            int pixel = gengrp_point(xx, yy) & ((1 << bits_per_pixel) - 1);
            int byte_idx = 4 + (bitpos >> 3);
            int shift = 8 - bits_per_pixel - (bitpos & 7);
            /* for 2bpp, pixel spans may straddle: handle bit-level */
            buf[byte_idx] |= (unsigned char)(pixel << shift);
            bitpos += bits_per_pixel;
        }
    }
    
    return SUCCESS;
}

int advgrp_put(int x, int y, const char *array_name, int action) {
    if (g_screen_mode == 0) return ERROR;
    if (!array_name || array_name[0] == 0) return ERROR;
    
    struct variable *v = biptrg_ensure_array(array_name);
    if (!v || !v->is_array || v->type == VT_STRING) {
        return advgrp_gfx_fcerr();
    }
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc || desc->magic != ARRAY_MAGIC || !desc->data) {
        return advgrp_gfx_fcerr();
    }
    
    unsigned char *buf = (unsigned char *)desc->data;
    int xbits = buf[0] | (buf[1] << 8);
    int height = buf[2] | (buf[3] << 8);
    int bits_per_pixel = (g_screen_mode == 2) ? 1 : 2;
    int width = bits_per_pixel > 0 ? xbits / bits_per_pixel : 0;
    if (width <= 0 || height <= 0) return advgrp_gfx_fcerr();
    
    /* Default action is XOR (ASM GFUNTB default) */
    if (action < 0) action = 5;
    
    int fb_w = gengrp_get_fb_width();
    int fb_h = gengrp_get_fb_height();
    
    int bitpos = 0;
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int byte_idx = 4 + (bitpos >> 3);
            int shift = 8 - bits_per_pixel - (bitpos & 7);
            int pixel = (buf[byte_idx] >> shift) & ((1 << bits_per_pixel) - 1);
            bitpos += bits_per_pixel;
            
            int sx = x + dx;
            int sy = y + dy;
            if (sx < 0 || sx >= fb_w || sy < 0 || sy >= fb_h) continue;
            
            switch (action) {
                case 1:  /* PSET */
                    gengrp_direct_pset(sx, sy, pixel);
                    break;
                case 2:  /* PRESET */
                    gengrp_direct_pset(sx, sy, (1 << bits_per_pixel) - 1 - pixel);
                    break;
                case 3: { /* AND */
                    int cur = gengrp_point(sx, sy);
                    gengrp_direct_pset(sx, sy, cur & pixel);
                    break;
                }
                case 4: { /* OR */
                    int cur = gengrp_point(sx, sy);
                    gengrp_direct_pset(sx, sy, cur | pixel);
                    break;
                }
                case 5: { /* XOR */
                    int cur = gengrp_point(sx, sy);
                    gengrp_direct_pset(sx, sy, cur ^ pixel);
                    break;
                }
                default:
                    gengrp_direct_pset(sx, sy, pixel);
                    break;
            }
        }
    }
    
    return SUCCESS;
}
