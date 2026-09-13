/* SCNEDT - Screen Oriented Editor for GW-BASIC */
/* Based on SCNEDT.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* External references */
extern void scndrv_init(void);
extern void scndrv_loc(int col, int row);
extern void scndrv_out(int ch);
extern int scndrv_pos(int *col, int *row);
extern int scndrv_rdl(int line, char *buf, int maxlen);
extern void scndrv_mrk(bool is_input);
extern void scndrv_brk(void);

extern void giokyb_init(void);
extern char *giokyb_inkey(void);
extern char *giokyb_input(int n);

/* Global state from SCNEDT.ASM */
static BYTE F_EDPG = 0;      /* Program statement edit flag */
static BYTE SEMFLG = 0;      /* Semicolon flag for INPUT; */
static BYTE F_INST = 0;      /* Insert mode flag */
static BYTE F_EDIT = 0;      /* Edit active flag */
static WORD AUTFLG = 0;      /* AUTO mode flag */
static BYTE CSRX = 1;        /* Cursor column (1-based) */
static BYTE CSRY = 1;        /* Cursor row (1-based) */

/* Line buffer */
#define BUFLEN 255
static char g_buf[BUFLEN + 2];  /* +1 for sentinel, +1 for null */
static int g_buf_len = 0;

/* Control character definitions */
#define CHREDT    1   /* EDIT */
#define CHRBKW    2   /* Back Word */
#define CHRCAN    3   /* Cancel */
#define CHRHCN    4   /* Hard Cancel */
#define CHRCLE    5   /* Clear to End of line */
#define CHRFDW    6   /* Forward Word */
#define CHRBEL    7   /* Bell */
#define CHRBAK    8   /* Backspace */
#define CHRTAB    9   /* Tab */
#define CHRLNF    10  /* Line Feed */
#define CHRHOM    11  /* Home */
#define CHRERA    12  /* Erase */
#define CHRRET    13  /* Return */
#define CHRAPP    14  /* Append */
#define CHRINS    18  /* Insert */
#define CHRLDL    21  /* Line Delete */
#define CHRADV    28  /* Cursor Right */
#define CHRREG    29  /* Cursor Left */
#define CHRUP     30  /* Cursor Up */
#define CHRDWN    31  /* Cursor Down */
#define CHRDEL    127 /* Delete */

/* Forward declarations */
void scnedt_clrflg(void);
void scnedt_edtbrk(void);
static int scnedt_inlin0(int mode);
static void scnedt_inlret(void);
static void scnedt_out(int ch);

/* Module initialization */
void scnedt_init(void) {
    F_EDPG = 0;
    SEMFLG = 0;
    F_INST = 0;
    F_EDIT = 0;
    AUTFLG = 0;
    CSRX = 1;
    CSRY = 1;
    g_buf_len = 0;
    memset(g_buf, 0, sizeof(g_buf));
    
    /* Initialize screen driver */
    scndrv_init();
    
    /* Initialize keyboard */
    giokyb_init();
}

/* Clear editor flags */
void scnedt_clrflg(void) {
    F_EDIT = 0;
    F_EDPG = 0;
    F_INST = 0;
    SEMFLG = 0;
}

/* EDTBRK - Initialize when break detected */
void scnedt_edtbrk(void) {
    /* Turn off cursor */
    scndrv_loc(CSRX, CSRY);
    
    /* Clear editor flags */
    scnedt_clrflg();
    
    /* Move cursor to end of logical line */
    F_EDIT = 1;
    scnedt_out(CHRAPP);
    F_EDIT = 0;
    
    /* Move to next physical line */
    scnedt_out(CHRRET);
    scndrv_loc(1, CSRY + 1);
    
    AUTFLG = 0;
}

/* Output a character through screen driver */
static void scnedt_out(int ch) {
    scndrv_out(ch);
    
    /* Update cursor position based on character */
    switch (ch) {
        case CHRRET:
            CSRX = 1;
            CSRY++;
            break;
        case CHRLNF:
            CSRY++;
            break;
        case CHRBAK:
            if (CSRX > 1) CSRX--;
            else if (CSRY > 1) { CSRY--; CSRX = 80; }
            break;
        case CHRADV:
            if (CSRX < 80) CSRX++;
            else { CSRX = 1; CSRY++; }
            break;
        case CHRREG:
            if (CSRX > 1) CSRX--;
            else if (CSRY > 1) { CSRY--; CSRX = 80; }
            break;
        case CHRUP:
            if (CSRY > 1) CSRY--;
            break;
        case CHRDWN:
            CSRY++;
            break;
        case CHRHOM:
            CSRX = 1;
            CSRY = 1;
            break;
        default:
            if (ch >= 32 && ch < 127) {
                if (CSRX < 80) CSRX++;
                else { CSRX = 1; CSRY++; }
            }
            break;
    }
    
    scndrv_loc(CSRX, CSRY);
}

/* PINLIN - Program line input */
void scnedt_pinlin(void) {
    scnedt_inlin0(-1);  /* STC -> AL = -1 */
}

/* QINLIN - Input with "?" prompt */
void scnedt_qinlin(void) {
    scnedt_out('?');
    scnedt_out(' ');
    scnedt_inlin0(0);   /* AL = 0 */
}

/* INLIN - BASIC INPUT statement line input */
void scnedt_inlin(void) {
    scnedt_inlin0(0);   /* AL = 0 */
}

/* SINLIN - BASIC INPUT statement redo line input */
void scnedt_sinlin(void) {
    scnedt_inlin0(0);   /* OR AL,AL -> AL = 0 */
}

/* Main editor loop entry */
static int scnedt_inlin0(int mode) {
    F_EDPG = (mode < 0) ? 0xFF : 0;  /* Program statement edit flag */
    
    BYTE old_F_EDIT = F_EDIT;
    F_EDIT = 0xFF;  /* Indicate executing INLIN */
    
    /* Set up for edit */
    scndrv_mrk(mode < 0);  /* AL=1 if program edit, else 0 */
    
    g_buf_len = 0;
    memset(g_buf, 0, sizeof(g_buf));
    
    /* Main editor loop */
    while (1) {
        /* Set cursor type */
        scndrv_loc(CSRX, CSRY);
        
        /* Get a key */
        char *key = giokyb_inkey();
        if (!key || key[0] == 0) continue;
        
        int ch = (unsigned char)key[0];
        
        /* Handle 2-byte sequences (extended keys) */
        if (key[1] != 0) {
            int ch2 = (unsigned char)key[1];
            
            /* Editor control functions (AH=0xFF in ASM) */
            if (ch2 == 0xFF) {
                switch (ch) {
                    case CHREDT:    /* EDIT - help key during program input */
                        if (F_EDPG) {
                            /* Do edit on KBUF */
                            scnedt_inlin0(0);
                        }
                        continue;
                    case CHRBKW:    /* Back Word */
                        /* Move back one word */
                        while (CSRX > 1 && g_buf[CSRX-2] != ' ') CSRX--;
                        while (CSRX > 1 && g_buf[CSRX-2] == ' ') CSRX--;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRCAN:    /* Cancel */
                    case CHRHCN:    /* Hard Cancel */
                        scnedt_edtbrk();
                        return 0;
                    case CHRCLE:    /* Clear to End of line */
                        g_buf_len = CSRX - 1;
                        g_buf[g_buf_len] = 0;
                        continue;
                    case CHRFDW:    /* Forward Word */
                        while (CSRX <= g_buf_len && g_buf[CSRX-1] != ' ') CSRX++;
                        while (CSRX <= g_buf_len && g_buf[CSRX-1] == ' ') CSRX++;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRBEL:    /* Bell */
                        Beep(800, 100);
                        continue;
                    case CHRBAK:    /* Backspace */
                        if (CSRX > 1) {
                            CSRX--;
                            g_buf_len--;
                            memmove(&g_buf[CSRX-1], &g_buf[CSRX], g_buf_len - CSRX + 1);
                            g_buf[g_buf_len] = 0;
                            scndrv_loc(CSRX, CSRY);
                        }
                        continue;
                    case CHRTAB:    /* Tab */
                        /* Move to next tab stop (every 8 columns) */
                        CSRX = ((CSRX - 1) / 8 + 1) * 8 + 1;
                        if (CSRX > 80) CSRX = 80;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRLNF:    /* Line Feed */
                        /* In editor, LF acts like down arrow */
                        if (CSRY < 25) CSRY++;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRHOM:    /* Home */
                        CSRX = 1;
                        CSRY = 1;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRERA:    /* Erase (clear to end) */
                        g_buf_len = CSRX - 1;
                        g_buf[g_buf_len] = 0;
                        continue;
                    case CHRAPP:    /* Append (move to end) */
                        CSRX = g_buf_len + 1;
                        if (CSRX > 80) CSRX = 80;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRINS:    /* Insert mode toggle */
                        F_INST = !F_INST;
                        continue;
                    case CHRLDL:    /* Line Delete */
                        g_buf_len = 0;
                        CSRX = 1;
                        memset(g_buf, 0, sizeof(g_buf));
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRADV:    /* Cursor Right */
                        if (CSRX <= g_buf_len && CSRX < 80) CSRX++;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRREG:    /* Cursor Left */
                        if (CSRX > 1) CSRX--;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRUP:     /* Cursor Up */
                        if (CSRY > 1) CSRY--;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRDWN:    /* Cursor Down */
                        if (CSRY < 25) CSRY++;
                        scndrv_loc(CSRX, CSRY);
                        continue;
                    case CHRDEL:    /* Delete */
                        if (CSRX <= g_buf_len) {
                            memmove(&g_buf[CSRX-1], &g_buf[CSRX], g_buf_len - CSRX + 1);
                            g_buf_len--;
                            g_buf[g_buf_len] = 0;
                        }
                        continue;
                }
            }
            
            /* Single byte character or CF=0 with AL!=0xFF */
            if (ch == CHRRET) {  /* Carriage Return - terminate line input */
                scnedt_inlret();
                break;
            }
            
            if (ch == 0xFE) {  /* 3-byte character, ignore */
                continue;
            }
            
            /* Echo character */
            if (ch >= 32 && ch < 127) {
                /* Insert or overwrite */
                if (F_INST && CSRX <= g_buf_len) {
                    memmove(&g_buf[CSRX], &g_buf[CSRX-1], g_buf_len - CSRX + 2);
                    g_buf_len++;
                } else if (!F_INST && CSRX > g_buf_len + 1) {
                    /* Pad with spaces */
                    while (g_buf_len < CSRX - 1) {
                        g_buf[g_buf_len++] = ' ';
                    }
                }
                
                /* In C, BYTE is 0-255, BUFLEN=255 so CSRX<=BUFLEN is always true for valid CSRX.
 * The logic handles overflow via CSRX++ wrapping. Keep comparison but suppress warning. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
                if (CSRX <= (BYTE)BUFLEN) {
#pragma GCC diagnostic pop
                    g_buf[CSRX-1] = ch;
                    if (CSRX > g_buf_len) g_buf_len = CSRX;
                    CSRX++;
                    scnedt_out(ch);
                }
            }
        }
    }
    
    F_EDIT = old_F_EDIT;
    return 0;
}

/* INLRET - Exit, return current logical line */
static void scnedt_inlret(void) {
    /* Read logical line into buffer */
    scndrv_rdl(CSRY, g_buf, BUFLEN);
    g_buf_len = strlen(g_buf);
    
    /* Trim trailing blanks and linefeeds for program statements */
    if (F_EDPG) {
        while (g_buf_len > 0 && (g_buf[g_buf_len-1] == ' ' || g_buf[g_buf_len-1] == '\n')) {
            g_buf_len--;
        }
        g_buf[g_buf_len] = 0;
    }
    
    /* Handle semicolon flag for INPUT; */
    if (SEMFLG) {
        scnedt_out(CHRAPP);  /* Move to end of logical line */
    }
    
    F_EDIT = 0;
    scnedt_clrflg();
    
    /* Move to next physical line */
    scnedt_out(CHRAPP);
    scnedt_out(CHRRET);
    scndrv_loc(1, CSRY + 1);
    
    /* BUF is now available at global g_buf */
}

/* SCNSEM - Scan for semicolon */
void scnedt_scnsem(void) {
    /* Check if current character is semicolon */
    if (g_buf_len > 0 && g_buf[g_buf_len-1] == ';') {
        SEMFLG = ';';
        g_buf_len--;  /* Skip semicolon */
        g_buf[g_buf_len] = 0;
    }
}

/* ERREDT - Automatic edit for errors */
void scnedt_erredt(int errlin) {
    (void)errlin;
    if (errlin == 0xFFFF || errlin == 0) return;
    
    /* Find and edit the error line */
    /* This would integrate with program storage (gwlist) */
    /* For now, just mark for edit */
    F_EDIT = 1;
}

/* EDIT - EDIT statement */
void scnedt_edit(int line_num) {
    (void)line_num;
    /* Find line and put in buffer for editing */
    /* This would integrate with program storage (gwlist) */
    F_EDIT = 1;
    F_EDPG = 0xFF;
}

/* Get current buffer */
const char* scnedt_get_buf(void) {
    return g_buf;
}

int scnedt_get_buf_len(void) {
    return g_buf_len;
}

/* Set buffer for editing */
void scnedt_set_buf(const char *buf) {
    strncpy(g_buf, buf, BUFLEN);
    g_buf_len = strlen(g_buf);
    g_buf[g_buf_len] = 0;
}