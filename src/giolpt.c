/* GIOLPT - Line Printer Device Driver */
/* Based on GIOLPT.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

/* LPT Device Control Block */
typedef struct {
    int width;      /* _LPWID - device width (columns per line) */
    int pos;        /* _LPPOS - current column device is in (0-based) */
    int flags;      /* _LPFLG - boolean attributes mask */
    int cr_flag;    /* _LPCRF - non-zero = last char sent was CR */
} lpt_dcb_t;

/* Support up to 3 printers (LPT1, LPT2, LPT3) */
#define MAX_LPT_DEVICES 3
static lpt_dcb_t g_lpt_dcb[MAX_LPT_DEVICES];
static bool g_lpt_initialized = false;

/* Printer handles */
static HANDLE g_lpt_handle[MAX_LPT_DEVICES] = {NULL, NULL, NULL};

/* Module initialization */
void giolpt_init(void) {
    if (g_lpt_initialized) return;
    
    for (int i = 0; i < MAX_LPT_DEVICES; i++) {
        g_lpt_dcb[i].width = 80;     /* Default width = 80 chars/line */
        g_lpt_dcb[i].pos = 0;        /* Initial position = 0 */
        g_lpt_dcb[i].flags = 0;      /* Reset device flags */
        g_lpt_dcb[i].cr_flag = 0;    /* Clear CR flag */
        
        /* Try to open printer port */
        char port_name[16];
        snprintf(port_name, sizeof(port_name), "LPT%d:", i + 1);
        g_lpt_handle[i] = CreateFileA(port_name, GENERIC_WRITE, 0, NULL, 
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_lpt_handle[i] == INVALID_HANDLE_VALUE) {
            g_lpt_handle[i] = NULL;  /* Port not available */
        }
    }
    
    g_lpt_initialized = true;
}

/* Get pointer to LPT DCB */
static lpt_dcb_t* get_lpt_dcb(int device_id) {
    if (device_id < 0 || device_id >= MAX_LPT_DEVICES) return NULL;
    return &g_lpt_dcb[device_id];
}

/* LPTOPN - Open printer */
void giolpt_open(int device_id, int mode) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return;
    
    /* Initialize DCB */
    dcb->width = 80;
    dcb->pos = 0;
    dcb->flags = 0;
    dcb->cr_flag = 0;
    
    /* Open printer port if not already open */
    if (!g_lpt_handle[device_id]) {
        char port_name[16];
        snprintf(port_name, sizeof(port_name), "LPT%d:", device_id + 1);
        g_lpt_handle[device_id] = CreateFileA(port_name, GENERIC_WRITE, 0, NULL,
                                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_lpt_handle[device_id] == INVALID_HANDLE_VALUE) {
            g_lpt_handle[device_id] = NULL;
        }
    }
    
    (void)mode;  /* Mode handling would go here */
}

/* LPTCLS - Close printer */
void giolpt_close(int device_id) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return;
    
    /* Close printer handle if open */
    if (g_lpt_handle[device_id]) {
        CloseHandle(g_lpt_handle[device_id]);
        g_lpt_handle[device_id] = NULL;
    }
    
    /* Reset DCB */
    dcb->width = 80;
    dcb->pos = 0;
    dcb->flags = 0;
    dcb->cr_flag = 0;
}

/* LPTSWD - Set device width */
void giolpt_swd(int device_id, int width) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return;
    
    if (width > 0 && width <= 255) {
        dcb->width = width;
    }
}

/* LPTSOT - Sequential Output */
void giolpt_sot(int device_id, int ch) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return;
    
    /* Handle CR/LF translation for non-binary mode */
    if (ch == '\n' && dcb->cr_flag) {
        /* Eat LF after CR */
        dcb->cr_flag = 0;
        return;
    }
    
    /* Reset CR flag if not CR */
    if (ch != '\r') {
        dcb->cr_flag = 0;
    }
    
    /* Output character to printer */
    if (g_lpt_handle[device_id]) {
        DWORD written;
        char c = (char)ch;
        WriteFile(g_lpt_handle[device_id], &c, 1, &written, NULL);
    }
    
    /* Update column position */
    if (ch == '\r') {
        dcb->pos = 0;
        dcb->cr_flag = 1;
        
        /* Output LF after CR unless width is 255 and random mode */
        if (dcb->width != 255) {
            char lf = '\n';
            DWORD written;
            if (g_lpt_handle[device_id]) {
                WriteFile(g_lpt_handle[device_id], &lf, 1, &written, NULL);
            }
        }
    } else if (ch == '\n') {
        dcb->pos = 0;
    } else if (ch == '\t') {
        /* Tab - advance to next tab stop (every 8 columns) */
        dcb->pos = ((dcb->pos / 8) + 1) * 8;
        if (dcb->pos >= dcb->width) {
            dcb->pos = 0;
            /* Would output CR/LF */
            if (g_lpt_handle[device_id]) {
                char cr = '\r', lf = '\n';
                DWORD written;
                WriteFile(g_lpt_handle[device_id], &cr, 1, &written, NULL);
                WriteFile(g_lpt_handle[device_id], &lf, 1, &written, NULL);
            }
        }
    } else {
        dcb->pos++;
        if (dcb->pos >= dcb->width) {
            dcb->pos = 0;
            /* Line overflow - output CR/LF */
            if (g_lpt_handle[device_id]) {
                char cr = '\r', lf = '\n';
                DWORD written;
                WriteFile(g_lpt_handle[device_id], &cr, 1, &written, NULL);
                WriteFile(g_lpt_handle[device_id], &lf, 1, &written, NULL);
            }
        }
    }
}

/* LPTGPS - Get current file position */
int giolpt_gps(int device_id) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return 0;
    return dcb->pos;
}

/* LPTGWD - Get device width */
int giolpt_gwd(int device_id) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return 80;
    return dcb->width;
}

/* LPOS function - returns current printer position (1-based) */
int giolpt_lpos(int device_id) {
    lpt_dcb_t *dcb = get_lpt_dcb(device_id);
    if (!dcb) return 1;
    return dcb->pos + 1;  /* Convert to 1-based */
}