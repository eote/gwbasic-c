/* SCNDRV - Screen Driver (text mode) */
/* Based on SCNDRV.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

/* Forward declarations for static helper functions */
static void scndrv_out_cr(void);
static void scndrv_out_lf(void);
static void scndrv_out_bs(void);
static void scndrv_out_tab(void);
static void scndrv_wrap_lf(void);

/* Text mode screen state */
static int g_crtwid = 80;      /* CRTWID - physical screen width */
static int g_lincnt = 25;      /* LINCNT - physical screen height */
static int g_linlen = 80;      /* LINLEN - logical line length */
static int g_wdotop = 1;       /* WDOTOP - window top line */
static int g_wdobot = 24;      /* WDOBOT - window bottom line */
static int g_wdolft = 1;       /* WDOLFT - window left column */
static int g_wdorgt = 80;      /* WDORGT - window right column */

static int g_csrx = 1;         /* CSRX - current column (1-based) */
static int g_csry = 1;         /* CSRY - current line (1-based) */
static int g_fstlin = 0;       /* FSTLIN - first logical line */
static int g_fstcol = 0;       /* FSTCOL - first logical column */
static int g_lstlin = 0xFF;    /* LSTLIN - last logical line (255 = none) */
static int g_lstcol = 0;       /* LSTCOL - last logical column */

static int g_f_cret = 0xFF;    /* F_CRET - last char was CR */
static bool g_f_edit = false;  /* F_EDIT - editing mode */
static bool g_f_inst = false;  /* F_INST - insert mode */
static bool g_f_esc = false;   /* ESCFLG - escape sequence flag */
static int g_keysw = 0;        /* KEYSW - function key display */

/* Terminator table */
#define MAX_LINES 25
static unsigned char g_linttb[MAX_LINES * 2];  /* [terminator_pos, terminator_val] */
static int g_trmcur = 0;       /* TRMCUR - current terminator index */

/* Terminator types */
#define TRMLNF  0x0A   /* Linefeed terminator */
#define TRMEOL  0xFF   /* EOL terminator (177 octal) */
#define TRMWRP  0x00   /* Wrap terminator */
#define TRMNWP  0x01   /* Null wrap terminator */
#define TRMNUL  0x80   /* Sign bit indicates initialize line */

/* Module initialization */
void scndrv_init(void) {
    /* Initialize terminator table */
    for (int i = 0; i < MAX_LINES; i++) {
        g_linttb[i * 2] = 0;
        g_linttb[i * 2 + 1] = TRMEOL;
    }
    g_trmcur = 0;
    
    /* Reset cursor and logical positions */
    g_csrx = 1;
    g_csry = 1;
    g_fstlin = 0;
    g_fstcol = 0;
    g_lstlin = 0xFF;
    g_lstcol = 0;
    g_f_cret = 0xFF;
    g_f_edit = false;
    g_f_inst = false;
    g_f_esc = false;
    g_keysw = 0;
}

/* SCNSWI - Set screen width and height */
void scndrv_swi(int width, int height) {
    g_crtwid = width;
    g_linlen = width;
    g_lincnt = height;
    g_wdotop = 1;
    g_wdobot = height - 1;  /* Reserve status line */
    g_wdolft = 1;
    g_wdorgt = width;
}

/* SCNLOC - Locate cursor on physical screen */
void scndrv_loc(int col, int row) {
    if (col < 1) col = 1;
    if (col > g_linlen) col = g_linlen;
    if (row < 1) row = 1;
    if (row > g_lincnt) row = g_lincnt;
    
    g_csrx = col;
    g_csry = row;
}

/* SCNCLR - Clear screen, home cursors */
void scndrv_clr(void) {
    /* Clear terminator table */
    for (int i = 0; i < MAX_LINES; i++) {
        g_linttb[i * 2] = 0;
        g_linttb[i * 2 + 1] = TRMEOL;
    }
    g_trmcur = 0;
    
    /* Home graphics cursor (would call GRPINI) */
    
    /* Home text cursor */
    g_csrx = g_wdolft;
    g_csry = g_wdotop;
    
    /* Update cursor position via console (giocon) */
    giocon_locate(g_csry, g_csrx);
    
    /* Refresh function key display */
    /* Would call KEYDSP */
}

/* SCNOUT - Output character at current position */
void scndrv_out(int ch) {
    /* Handle control characters */
    if (ch < 32 || ch == 27) {
        /* Control character or ESC - would call CTLDSP */
        /* For now, just handle basic ones */
        switch (ch) {
            case '\r':  /* Carriage return */
                scndrv_out_cr();
                return;
            case '\n':  /* Linefeed */
                scndrv_out_lf();
                return;
            case '\b':  /* Backspace */
                scndrv_out_bs();
                return;
            case '\t':  /* Tab */
                scndrv_out_tab();
                return;
            case 7:     /* Bell */
                Beep(800, 200);
                return;
        }
    }
    
    /* Regular character - do wrap logic */
    if (g_csrx > g_linlen) {
        /* Wrap to next line */
        scndrv_out_lf();
    }
    
    /* Output character at current position */
    giocon_locate(g_csry, g_csrx);
    putchar(ch);
    fflush(stdout);
    
    g_csrx++;
    g_f_cret = 0xFF;  /* Clear last char was CR flag */
    
    /* Update terminator table */
    int idx = (g_csry - 1) * 2;
    if (idx >= 0 && idx < MAX_LINES * 2) {
        g_linttb[idx] = g_csrx - 1;
        g_linttb[idx + 1] = TRMEOL;
    }
}

/* SCNPOS - Get current cursor position */
int scndrv_pos(int *col, int *row) {
    if (col) *col = g_csrx;
    if (row) *row = g_csry;
    
    /* Ensure position is within logical width */
    if (g_csrx > g_linlen) {
        if (col) *col = g_linlen;
    }
    return 0;
}

/* SCNWDO - Set text window */
int scndrv_wdo(int col, int row, int width, int height) {
    /* Validate position within physical screen */
    if (row < 1 || row > g_lincnt || col < 1 || col > g_crtwid) {
        return -1;  /* CF = error */
    }
    
    /* Truncate width/height to fit */
    if (row + height - 1 > g_lincnt) height = g_lincnt - row + 1;
    if (col + width - 1 > g_crtwid) width = g_crtwid - col + 1;
    
    g_wdotop = row;
    g_wdobot = row + height - 1;
    g_wdolft = col;
    g_wdorgt = col + width - 1;
    
    return 0;
}

/* SCNIPL - Initialize */
void scndrv_ipi(void) {
    /* Reset ESC flag */
    g_f_esc = false;
}

/* SCNBRK - Break handler */
void scndrv_brk(void) {
    g_fstlin = 0;
    g_lstlin = 0xFF;
    g_f_cret = 0xFF;
}

/* SCNGWI - Get logical width */
int scndrv_gwi(void) {
    return g_linlen;
}

/* SCNRDL - Read logical line */
int scndrv_rdl(int line, char *buf, int maxlen) {
    /* Simplified implementation - would need full terminator table logic */
    (void)line;
    (void)buf;
    (void)maxlen;
    return 0;
}

/* SCNMRK - Mark position */
void scndrv_mrk(bool is_input) {
    if (is_input) {
        g_lstlin = g_csry;
        g_lstcol = g_csrx - 1;
        g_fstlin = g_csry;
        g_fstcol = g_csrx;
    } else {
        g_fstlin = 0;
        g_fstcol = 0;
        g_lstlin = 0xFF;
    }
}

/* Helper: Carriage return */
static void scndrv_out_cr(void) {
    /* Read current terminator */
    int idx = (g_csry - 1) * 2;
    if (idx >= 0 && idx < MAX_LINES * 2) {
        unsigned char term_val = g_linttb[idx + 1];
        
        if (term_val == TRMLNF || term_val == TRMNWP) {
            /* Don't include LF or NULL_WRAP in line */
            g_linttb[idx] = g_csrx - 2;
        }
        
        /* Make this line EOL terminated */
        g_linttb[idx + 1] = TRMEOL;
    }
    
    /* Move to left margin */
    g_csrx = g_wdolft;
    giocon_locate(g_csry, g_csrx);
}

/* Helper: Linefeed */
static void scndrv_out_lf(void) {
    bool after_cr = (g_f_cret == 0);
    bool insert_mode = g_f_inst;
    
    if (after_cr && insert_mode) {
        /* Insert mode - move characters from cursor to EOL onto next line */
        /* Simplified - just append LF to end of physical line */
    }
    
    /* Try to add LF to current line */
    int idx = (g_csry - 1) * 2;
    if (idx >= 0 && idx < MAX_LINES * 2) {
        unsigned char term_pos = g_linttb[idx];
        
        if (term_pos < g_linlen) {
            /* Have room on this line */
            g_linttb[idx] = g_csrx;
            g_linttb[idx + 1] = TRMLNF;
        } else {
            /* Wrap linefeed to next line */
            scndrv_wrap_lf();
        }
    }
    
    /* Move to next line */
    if (g_csry < g_wdobot) {
        g_csry++;
        g_csrx = g_wdolft;
        giocon_locate(g_csry, g_csrx);
    }
    
    g_f_cret = 0xFF;
}

/* Helper: Backspace */
static void scndrv_out_bs(void) {
    if (g_csrx > g_wdolft) {
        g_csrx--;
        giocon_locate(g_csry, g_csrx);
        putchar(' ');
        giocon_locate(g_csry, g_csrx);
    }
}

/* Helper: Tab */
static void scndrv_out_tab(void) {
    if (g_f_inst) {
        /* Insert tabs until at tab stop or end of line */
        while (g_csrx <= g_linlen && (g_csrx % 8) != 0) {
            scndrv_out(' ');
        }
    } else {
        /* Non-insert tab - move over characters */
        while (g_csrx <= g_linlen && (g_csrx % 8) != 0) {
            g_csrx++;
            giocon_locate(g_csry, g_csrx);
        }
    }
}

/* Helper: Wrap linefeed to next line */
static void scndrv_wrap_lf(void) {
    /* Open next line */
    if (g_csry < g_wdobot) {
        /* Clear next line */
        g_csry++;
        g_csrx = g_wdolft;
        giocon_locate(g_csry, g_csrx);
        /* Would clear to end of line */
    }
}