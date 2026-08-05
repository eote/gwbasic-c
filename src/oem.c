/* OEM - OEM Abstraction Layer */
/* Based on OEM.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* External references from other modules */
extern void scndrv_init(void);
extern void scndrv_swi(int width, int height);
extern void scndrv_clr(void);
extern void scndrv_out(int ch);
extern int scndrv_pos(int *col, int *row);
extern void scndrv_brk(void);
extern int scndrv_gwi(void);

/* External references from giokyb */
extern void giokyb_init(void);

/* External references from giolpt, giocom */
extern void giolpt_init(void);
extern void giocom_init(void);

/* External references from advgrp */
extern void advgrp_init(void);

/* Global state variables from OEM.ASM DSEG */
static BYTE PIXBTS = 0;      /* Bits per pixel: 0=text, 1=2-color, 2=4-color */
static WORD PIXOFS = 0;      /* Byte offset of current pixel into video memory */
static BYTE ATRMSK = 0;      /* Mask for current pixel */
static BYTE ATRPAT = 0;      /* Bit pattern for current graphics attribute */
static BYTE BORPAT = 0;      /* Border pattern for PAINT */
static BYTE PGTSHF = 0;      /* Shift for PUT/GET alignment */
static WORD PGTARY = 0;      /* Pointer to byte array for PUT/GET */
static WORD PGTACT = 0;      /* Action function for PUT/GET */
static WORD CBFSEG = 0;      /* COM buffer segment */

/* Screen mode variables */
static BYTE CRTMOD = 3;      /* Current BIOS video mode */
static BYTE CRTINI = 3;      /* Initial video mode */
static WORD CSRINI = 0x0607; /* Initial cursor shape */
static BYTE VISPAG = 0;      /* Visible display page */
static BYTE ACTPAG = 0;      /* Active display page */

/* Text mode variables */
static BYTE TXTATR = 7;      /* Current text attribute */
static BYTE TXTFGC = 7;      /* Current foreground color for text */
static BYTE TXTBGC = 0;      /* Current background color for text */

/* Graphics colors */
static BYTE FORCLR = 1;      /* Current foreground color for graphics */
static BYTE BAKCLR = 0;      /* Current background color for graphics */

/* Cursor position */
static BYTE CSRX = 1;        /* Cursor X position (1-based) */
static BYTE CSRY = 1;        /* Cursor Y position (1-based) */

/* Window coordinates */
static BYTE WDOTOP = 1, WDOLFT = 1, WDOBOT = 25, WDORGT = 80;

/* Line count and CRT width */
static BYTE LINCNT = 25;
static BYTE CRTWID = 80;

/* Edit/Insert flags */
static BYTE F_EDIT = 0;
static BYTE F_INST = 0;

/* Cursor shape */
static WORD CSRSHP = 0x0607;

/* PG action functions */
static void pgt_or(void);
static void pgt_and(void);
static void pgt_xor(void);
static void pgt_set(void);
static void pgt_rst(void);

/* Function table for PG actions */
static void (*pgt_actions[5])(void) = {pgt_or, pgt_and, pgt_rst, pgt_set, pgt_xor};

/* Forward declarations */
static void txtmod(void);

/* Module initialization */
void oem_init(void) {
    /* Initialize OEM module */
    CRTMOD = 3;
    CRTINI = 3;
    CSRINI = 0x0607;
    VISPAG = 0;
    ACTPAG = 0;
    TXTATR = 7;
    TXTFGC = 7;
    TXTBGC = 0;
    FORCLR = 1;
    BAKCLR = 0;
    CSRX = 1;
    CSRY = 1;
    WDOTOP = 1; WDOLFT = 1; WDOBOT = 25; WDORGT = 80;
    LINCNT = 25;
    CRTWID = 80;
    F_EDIT = 0;
    F_INST = 0;
    CSRSHP = 0x0607;
    PIXBTS = 0;
    PIXOFS = 0;
    ATRMSK = 0;
    ATRPAT = 0;
    BORPAT = 0;
    PGTSHF = 0;
    PGTARY = 0;
    PGTACT = 0;
    CBFSEG = 0;
}

/* Check if current video mode is a text mode */
static void txtmod(void) {
    /* Text modes are 0, 1, 2, 3, 7 */
    /* Graphics modes are 4, 5, 6 */
}

/* Return in AL the screen attribute to use for text blanking */
static void filatr(void) {
    txtmod();
    /* For simplicity, we just use TXTATR */
    /* In graphics mode, we'd shift right by 4 */
}

/* Set cursor position for BIOS without moving hardware cursor */
static void csrhak(void) {
    /* In Windows console, we don't need to hack BIOS cursor position */
    /* This is handled by scndrv */
}

/* Convert text (column, row) to offset into video memory */
static void posoff(int *x, int *y, bool carry) {
    /* Adjust for 1-based coordinates */
    *x -= 1;
    *y -= 1;
    
    /* For text mode, each char is 2 bytes (char + attribute) */
    int offset = (*y * CRTWID + *x) * 2;
    
    if (carry) {
        /* Return offset of last byte */
        offset += 1;
    }
    
    *x = offset;
}

/* Initialize screen pointer for graphics operations */
static void scnptr(void) {
    /* In our framebuffer model, we use the framebuffer directly */
}

/* PG action functions */
static void pgt_or(void) {
    /* OR operation for PUT */
}

static void pgt_and(void) {
    /* AND operation for PUT */
}

static void pgt_xor(void) {
    /* XOR operation for PUT */
}

static void pgt_set(void) {
    /* PSET operation for PUT */
}

static void pgt_rst(void) {
    /* PRESET operation for PUT */
}

/* NRW beginning - common setup for NREAD/NWRITE */
static void nrwbeg(void) {
    /* Setup for horizontal pixel operations */
}

/* Write a byte to video memory for NWRITE */
static void nwrbyt(void) {
}

/* Read a byte from video memory for NREAD */
static void nrdbyt(void) {
}

/* Scan left for PAINT flood fill */
static void scan_left(void) {
}

/* Scan right for PAINT flood fill */
static void scan_right(void) {
}

/* SLRCHG - change pixel and check boundary */
static void slrchg(void) {
}

/* SLRBEG - initialize state for SCANL/SCANR */
static void slr_begin(void) {
}

/* OEM supplied Clear-Screen Routine
   al = 0: Clear everything
   al = 1: Clear graphics viewport
   al = 2: Clear text window */
void oem_clrscn(int al) {
    if (al == 2) {
        /* Clear text window */
        /* TODO: implement window clearing */
    } else {
        /* Clear all */
        scndrv_clr();
    }
}

/* OEM supplied screen clear to end of line */
void oem_clreol(int col, int row) {
    /* Clear from (col, row) to end of line */
    /* TODO: implement */
}

/* OEM supplied SCROLL routine */
void oem_scroll(int src_col, int src_row, int dst_col, int dst_row, int width, int height) {
    /* Scroll a region of the screen */
    /* TODO: implement */
}

/* OEM supplied screen input (read character) */
int oem_scrinp(int col, int row) {
    /* Read character at (col, row) */
    /* TODO: implement */
    return ' ';
}

/* OEM supplied character output */
void oem_scrout(int ch, int col, int row) {
    /* Output character at (col, row) */
    scndrv_loc(col, row);
    scndrv_out(ch);
}

/* FETCHC - Load current pixel's byte offset & mask */
void oem_fetchc(int *ofs, int *mask) {
    *ofs = PIXOFS;
    *mask = ATRMSK;
}

/* STOREC - Set current pixel's byte offset & mask */
void oem_storec(int ofs, int mask) {
    PIXOFS = ofs;
    ATRMSK = mask;
}

/* Set screen width */
bool oem_swidth(int width, int height) {
    if (height != 25) return true;  /* Error */
    if (width != 40 && width != 80) return true;  /* Error */
    
    CRTWID = width;
    /* Switch video mode based on width */
    return false;  /* Success */
}

/* NSETCX - Set a horizontal series of pixels */
void oem_nsetcx(int count) {
    /* Set count pixels to current color */
    /* TODO: implement */
}

/* SNDLPT - Send a byte to printer */
int oem_sndlpt(int printer, int ch) {
    /* Send character to printer port */
    /* 0 = success, 1 = not available, 2 = timeout, 3 = out of paper, 4+ = I/O error */
    return 0;
}

/* PEKFLT - Filter PEEK operation */
bool oem_pekflt(int seg, int ofs, int *value) {
    /* Allow all PEEKs by default, except special case for function keys */
    if (value && *value == 0 && ofs == 106 && seg == 0) {
        /* Special case from DONKEY.BAS */
        return true;  /* Filter out */
    }
    return false;
}

/* POKFLT - Filter POKE operation */
bool oem_pokflt(int seg, int ofs, int value) {
    /* Allow all POKEs by default, except special case for function keys */
    if (value == 0 && ofs == 106 && seg == 0) {
        /* Special case from DONKEY.BAS */
        return true;  /* Filter out */
    }
    return false;
}

/* SETCBF - Set COM buffer location and size */
bool oem_setcbf(int segment, int size) {
    CBFSEG = segment;
    return false;  /* Success */
}

/* LEFTC - Move current pixel left */
void oem_leftc(void) {
    if (ATRMSK == 0) return;
    
    if (PIXBTS == 1) {
        /* 1 bit per pixel */
        ATRMSK = (ATRMSK << 7) | (ATRMSK >> 1);
        if (ATRMSK & 0x80) {
            PIXOFS--;
        }
    } else if (PIXBTS == 2) {
        /* 2 bits per pixel */
        ATRMSK = (ATRMSK << 6) | (ATRMSK >> 2);
        if (ATRMSK & 0xC0) {
            PIXOFS--;
        }
    }
}

/* RIGHTC - Move current pixel right */
void oem_rightc(void) {
    if (ATRMSK == 0) return;
    
    if (PIXBTS == 1) {
        /* 1 bit per pixel */
        if (ATRMSK & 0x80) {
            PIXOFS++;
        }
        ATRMSK = (ATRMSK << 1) | (ATRMSK >> 7);
    } else if (PIXBTS == 2) {
        /* 2 bits per pixel */
        if (ATRMSK & 0xC0) {
            PIXOFS++;
        }
        ATRMSK = (ATRMSK << 2) | (ATRMSK >> 6);
    }
}

/* MAPXYC - Map (X,Y) coordinates to pixel offset and mask */
void oem_mapxyc(int x, int y) {
    /* Convert X,Y to video memory offset and bit mask */
    /* This is a simplified version for our framebuffer */
    PIXOFS = y * 320 + x;  /* For 320x200 mode */
    
    if (PIXBTS == 1) {
        ATRMSK = 0x80 >> (x & 7);
    } else if (PIXBTS == 2) {
        int bit = (x & 3) * 2;
        ATRMSK = 0xC0 >> bit;
    }
}

/* INKMAP - Map keystrokes for INKEY$ */
bool oem_inkmap(int *ax, int *dx, bool carry) {
    /* Map special keys for INKEY$ */
    /* Return true to discard keystroke */
    return false;
}

/* INFMAP - Map keystrokes for other purposes */
bool oem_infmap(int *ax, int *dx, bool carry) {
    return oem_inkmap(ax, dx, carry);
}

/* SETFBC - Set foreground/background colors */
void oem_setfbc(int fg, int bg, bool graphics) {
    if (graphics) {
        FORCLR = fg;
        BAKCLR = bg;
    } else {
        TXTFGC = fg;
        TXTBGC = bg;
        /* Combine into attribute byte */
        TXTATR = (bg << 4) | fg;
    }
}

/* INICOM - Initialize COM port (dummy) */
void oem_inicom(void) {
    /* Dummy function */
}

/* GETHED - Get OEM header */
const char* oem_gethed(void) {
    static char header[] = "GW-BASIC 2024 version, MIT License\n(C) Copyright 2024";
    return header;
}

/* TUPC - Move pixel up (with boundary check) */
bool oem_tupc(void) {
    if (PIXOFS < 80) return true;  /* At top, can't move up */
    oem_upc();
    return false;
}

/* UPC - Move pixel up */
void oem_upc(void) {
    if (PIXBTS == 1) {
        if ((PIXOFS & 0xFF00) >= 0x2000) {
            PIXOFS -= 0x2000;
        } else {
            PIXOFS -= 80;
        }
    } else if (PIXBTS == 2) {
        PIXOFS -= 160;
    }
}

/* SCALXY - Clip graphics coordinates */
void oem_scalxy(int *x, int *y) {
    if (PIXBTS == 0) return;  /* Not graphics mode */
    
    int max_x = (PIXBTS == 1) ? 640 : 320;
    int max_y = 200;
    
    if (*x < 0) *x = 0;
    if (*x >= max_x) *x = max_x - 1;
    if (*y < 0) *y = 0;
    if (*y >= max_y) *y = max_y - 1;
}

/* GTASPC - Get pixel aspect ratio */
void oem_gtasp(int *bx, int *dx) {
    if (PIXBTS == 1) {
        *bx = 0xD5;  /* Width/height ratio */
        *dx = 0x133; /* Height/width ratio */
    } else {
        *bx = 0xD5 / 2;
        *dx = 0x133 * 2;
    }
}

/* PNTINI - Initialize PAINT operation */
void oem_pntini(int border_color) {
    BORPAT = border_color;
    /* Set ATRPAT to fill color (current) */
}

/* READC - Read pixel color */
int oem_readc(void) {
    if (ATRMSK == 0) return 0;
    
    int bit = 0;
    BYTE mask = ATRMSK;
    while (!(mask & 1)) {
        mask >>= 1;
        bit++;
    }
    
    /* Read from framebuffer */
    extern unsigned char *g_framebuffer;
    if (g_framebuffer && PIXOFS < MAX_WIDTH * MAX_HEIGHT) {
        int val = g_framebuffer[PIXOFS];
        val >>= bit;
        val &= ATRMSK;
        return val;
    }
    return 0;
}

/* SETC - Set pixel color */
void oem_setc(void) {
    if (ATRMSK == 0) return;
    
    extern unsigned char *g_framebuffer;
    if (g_framebuffer && PIXOFS < MAX_WIDTH * MAX_HEIGHT) {
        int val = g_framebuffer[PIXOFS];
        val ^= ATRPAT;
        val &= ATRMSK;
        val ^= g_framebuffer[PIXOFS];
        g_framebuffer[PIXOFS] = val;
    }
}

/* PIXSIZ - Get bits per pixel */
int oem_pixsiz(void) {
    return PIXBTS;
}

/* PGINIT - Initialize PUT/GET operation */
void oem_pginit(int action, int *array_ptr, int bit_count) {
    PGTARY = (int)array_ptr;
    PGTSHF = 0;
    PGTACT = (int)pgt_actions[action];
}

/* NREAD - Read horizontal pixels into array */
void oem_nread(int *array_ptr, int bit_count) {
    /* Read pixels into array */
    /* TODO: implement */
}

/* NWRITE - Write horizontal pixels from array */
void oem_nwrite(int *array_ptr, int bit_count) {
    /* Write pixels from array */
    /* TODO: implement */
}

/* TDOWNC - Move pixel down (with boundary check) */
bool oem_tdownc(void) {
    int max_ofs = (PIXBTS == 1) ? 0x2000 : 0x1000;
    if (PIXOFS >= max_ofs - 80) return true;  /* At bottom */
    oem_downc();
    return false;
}

/* DOWNC - Move pixel down */
void oem_downc(void) {
    if (PIXBTS == 1) {
        if ((PIXOFS & 0xFF00) >= 0x2000) {
            PIXOFS -= 0x2000;
        } else {
            PIXOFS += 80;
        }
    } else if (PIXBTS == 2) {
        PIXOFS += 160;
    }
}

/* LCPY - LCOPY command (no-op) */
bool oem_lcpy(int param) {
    return true;  /* Failure/no-op */
}

/* FKYFMT - Get function key display format */
void oem_fkyfmt(int *count, int *chars, int *start) {
    if (CRTWID == 40) {
        *count = 6;
        *chars = 5;
        *start = 1;
    } else {
        *count = 6;
        *chars = 10;
        *start = 1;
    }
}

/* FKYADV - Advance function key display */
bool oem_fkyadv(bool displaying) {
    if (displaying) {
        return false;  /* Keep displaying */
    }
    if (CRTWID >= 80) {
        return true;  /* Don't display */
    }
    return false;
}

/* KEYINP - Get keystroke from keyboard */
bool oem_keyinp(int *ax, int *dx, bool *carry, bool *zero) {
    /* Get keystroke from keyboard */
    *zero = true;  /* No key available */
    return true;
}

/* EDTMAP - Map control character for editing */
bool oem_edtmap(int *ax, bool *zero) {
    /* Map control characters for editor */
    if (*ax <= 0x1F) {
        *zero = false;
        return false;
    }
    *zero = true;
    return true;
}

/* PRTMAP - Map control character for printing */
bool oem_prtmap(int *ax, bool *zero) {
    /* Map control characters for printer */
    if (*ax <= 0x1F) {
        *zero = false;
        return false;
    }
    *zero = true;
    return true;
}

/* CSRATR - Set cursor attributes */
bool oem_csratr(int ah, int al, int bh, int bl, int ch, int cl) {
    /* Set cursor shape and visibility */
    if (ah != 0) {
        /* 3rd parameter given */
        if (al == 1) {
            CSRSHP = (CSRSHP & 0x9F) | 0x20;  /* Visible */
        } else {
            CSRSHP = (CSRSHP & 0x9F);  /* Invisible */
        }
    }
    if (bh != 0) {
        /* 4th parameter given - start scan line */
        if (bl <= 0x1F) {
            CSRSHP = (CSRSHP & 0xE0) | bl;
        } else {
            return true;  /* Error */
        }
    }
    if (ch != 0) {
        /* 5th parameter given - stop scan line */
        if (cl <= 0x1F) {
            CSRSHP = (CSRSHP & 0x1F) | (cl << 5);
        } else {
            return true;  /* Error */
        }
    }
    return false;  /* Success */
}

/* GWINI - GW-BASIC initialization */
void oem_gwini(void) {
    /* Initialize printer ports */
    for (int i = 0; i < 4; i++) {
        /* Initialize LPT1-4 */
    }
}

/* SCNRST - Reset screen mode */
void oem_scnrst(void) {
    if (CRTMOD == 3) return;  /* Already in mode 3 */
    CRTMOD = 3;
    /* Reset screen */
    oem_clrscn(0);
}

/* SCRSTT - SCREEN command */
bool oem_scrstt(int *params, int param_count) {
    if (param_count < 1 || param_count > 4) return true;  /* Error */
    
    int mode = params[0];
    if (mode > 2) return true;  /* Error */
    
    /* Additional screen setup would go here */
    return false;
}

/* SETCLR - Set color */
bool oem_setclr(int *params, int param_count, int mode) {
    /* Set color palette */
    return false;
}

/* SCRATR - Get screen attribute */
int oem_scratr(int col, int row) {
    return 0;
}

/* TRMCOM - Terminate COM */
void oem_trmcom(void) {
}

/* MAPSUP - Map key */
int oem_mapsup(int key) {
    return key;
}

/* SEGIN - Segment initialization */
void oem_segin(void) {
}

/* GWTERM - GW-BASIC termination */
void oem_gwterm(void) {
}

/* GRPSIZ - Get graphics size */
void oem_grpsiz(int *width, int *height) {
    *width = 640;
    *height = 200;
}

/* CSRDSP - Set cursor display type */
void oem_csrdsp(int type) {
}

/* SETATR - Set attribute */
bool oem_setatr(int color) {
    return false;
}

/* Accessor functions for testing */
BYTE oem_get_pixbts(void) { return PIXBTS; }
void oem_set_pixbts(BYTE val) { PIXBTS = val; }

WORD oem_get_pixofs(void) { return PIXOFS; }
void oem_set_pixofs(WORD val) { PIXOFS = val; }

BYTE oem_get_atrmsk(void) { return ATRMSK; }
void oem_set_atrmsk(BYTE val) { ATRMSK = val; }

BYTE oem_get_atrpat(void) { return ATRPAT; }
void oem_set_atrpat(BYTE val) { ATRPAT = val; }

BYTE oem_get_borpat(void) { return BORPAT; }
void oem_set_borpat(BYTE val) { BORPAT = val; }

WORD oem_get_pgtary(void) { return PGTARY; }
void oem_set_pgtary(WORD val) { PGTARY = val; }

WORD oem_get_pgtshf(void) { return PGTSHF; }
void oem_set_pgtshf(WORD val) { PGTSHF = val; }

WORD oem_get_pgtact(void) { return PGTACT; }
void oem_set_pgtact(WORD val) { PGTACT = val; }

BYTE oem_get_crtmod(void) { return CRTMOD; }
void oem_set_crtmod(BYTE val) { CRTMOD = val; }

BYTE oem_get_crtwid(void) { return CRTWID; }
void oem_set_crtwid(BYTE val) { CRTWID = val; }

BYTE oem_get_lincnt(void) { return LINCNT; }
void oem_set_lincnt(BYTE val) { LINCNT = val; }