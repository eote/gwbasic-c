/* GIOCOM - Serial Communications Device Driver */
/* Based on GIOCOM.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

/* COM Device Control Block */
typedef struct {
    int dev_id;          /* _DEVID - COM channel ID (0..n) */
    int baud_rate;       /* _BAUDR - baud rate */
    int byte_size;       /* _BYTSZ - bits per byte (4..8) */
    int parity;          /* _PARIT - parity (0=none, 1=odd, 2=even, 3=mark, 4=space) */
    int stop_bits;       /* _STOPB - stop bits (0=1, 1=1.5, 2=2) */
    int rlsd_timeout;    /* _RLSTO - RLSD timeout (ms) */
    int cts_timeout;     /* _CTSTO - CTS timeout (ms) */
    int dsr_timeout;     /* _DSRTO - DSR timeout (ms) */
    int flags;           /* _CMFLG - boolean attributes mask */
    int bin_mode;        /* _CMBIN - 0=ASCII, 1=BINARY */
    int rts_suppress;    /* _CMRTS - suppress RTS */
    int user_cod;        /* _CMCOD - user specified ASC/BIN */
    int cts_not_default; /* _CMCTS - CTS parm not defaulted */
    int ascii_eof_mode;  /* _CMCLF - ASCII mode: send CTL-Z on close */
    int send_lf_after_cr; /* send LF after CR */
    int cr_flag;         /* _CMCRF - last char sent was CR */
    int width;           /* _CMWID - device width (columns per line) */
    int pos;             /* _CMPOS - current column (0-based) */
    int fdb_ptr;         /* _CMFDB - FDB pointer (0 = not opened) */
} com_dcb_t;

/* Support up to 4 COM ports (COM1..COM4) */
#define MAX_COM_DEVICES 4
static com_dcb_t g_com_dcb[MAX_COM_DEVICES];
static bool g_com_initialized = false;

/* COM port handles */
static HANDLE g_com_handle[MAX_COM_DEVICES] = {NULL, NULL, NULL, NULL};

/* DCB size (matches ASM CDCBSZ = 24 bytes) */
#define COM_DCB_SIZE 24

/* Module initialization */
void giocom_init(void) {
    if (g_com_initialized) return;
    
    for (int i = 0; i < MAX_COM_DEVICES; i++) {
        g_com_dcb[i].dev_id = i;
        g_com_dcb[i].baud_rate = 300;      /* Default 300 baud */
        g_com_dcb[i].byte_size = 7;        /* Default 7 data bits */
        g_com_dcb[i].parity = 2;           /* Default even parity */
        g_com_dcb[i].stop_bits = 0xFF;     /* Mark as default (will be set on open) */
        g_com_dcb[i].rlsd_timeout = 0;     /* Default carrier detect timeout = 0 (infinite) */
        g_com_dcb[i].cts_timeout = 1000;   /* Default CTS timeout = 1 sec */
        g_com_dcb[i].dsr_timeout = 1000;   /* Default DSR timeout = 1 sec */
        g_com_dcb[i].flags = 1;            /* Default binary mode, CRLF off */
        g_com_dcb[i].bin_mode = 1;
        g_com_dcb[i].rts_suppress = 0;
        g_com_dcb[i].user_cod = 0;
        g_com_dcb[i].cts_not_default = 0;
        g_com_dcb[i].ascii_eof_mode = 0;
        g_com_dcb[i].send_lf_after_cr = 0;
        g_com_dcb[i].cr_flag = 0;
        g_com_dcb[i].width = 255;          /* Default infinite width */
        g_com_dcb[i].pos = 0;
        g_com_dcb[i].fdb_ptr = 0;          /* Not opened */
        
        g_com_handle[i] = NULL;
    }
    
    g_com_initialized = true;
}

/* Get COM DCB pointer */
static com_dcb_t* get_com_dcb(int device_id) {
    if (device_id < 0 || device_id >= MAX_COM_DEVICES) return NULL;
    return &g_com_dcb[device_id];
}

/* Get COM handle */
static HANDLE get_com_handle(int device_id) {
    if (device_id < 0 || device_id >= MAX_COM_DEVICES) return NULL;
    return g_com_handle[device_id];
}

/* Configure COM port DCB */
static bool configure_com_port(int device_id) {
    HANDLE handle = get_com_handle(device_id);
    if (!handle) return false;
    
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(handle, &dcb)) return false;
    
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return false;
    
    dcb.BaudRate = com->baud_rate;
    dcb.ByteSize = com->byte_size;
    dcb.Parity = com->parity;
    dcb.StopBits = (com->stop_bits == 0xFF) ? 0 : com->stop_bits;  /* Will use default if not set */
    
    /* Set timeouts */
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    
    if (com->cts_timeout > 0) {
        timeouts.ReadTotalTimeoutConstant = com->cts_timeout;
    }
    if (com->dsr_timeout > 0) {
        timeouts.WriteTotalTimeoutConstant = com->dsr_timeout;
    }
    
    if (!SetCommTimeouts(handle, &timeouts)) return false;
    if (!SetCommState(handle, &dcb)) return false;
    
    return true;
}

/* COMOPN - Open COM port */
void giocom_open(int device_id, int baud, int parity, int data_bits, int stop_bits) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return;
    
    /* Update parameters from open call */
    if (baud > 0) com->baud_rate = baud;
    if (parity >= 0 && parity <= 4) com->parity = parity;
    if (data_bits >= 4 && data_bits <= 8) com->byte_size = data_bits;
    if (stop_bits >= 0 && stop_bits <= 2) com->stop_bits = stop_bits;
    
    /* Set default stop bits if not specified */
    if (com->stop_bits == 0xFF) {
        com->stop_bits = 0;  /* 1 stop bit default */
        if (com->baud_rate <= 110) {
            com->stop_bits = 1;  /* 1.5 stop bits for <= 110 baud */
            if (com->byte_size > 5) {
                com->stop_bits = 2;  /* 2 stop bits for > 5 data bits at <= 110 baud */
            }
        }
    }
    
    /* Set width */
    com->width = 255;  /* Default infinite width */
    com->pos = 0;
    com->cr_flag = 0;
    com->send_lf_after_cr = 0;
    
    /* Open COM port */
    char port_name[16];
    snprintf(port_name, sizeof(port_name), "COM%d:", device_id + 1);
    
    g_com_handle[device_id] = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE,
                                           0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (g_com_handle[device_id] == INVALID_HANDLE_VALUE) {
        g_com_handle[device_id] = NULL;
        return;
    }
    
    /* Configure port */
    configure_com_port(device_id);
    
    /* Set RTS unless suppressed */
    if (!com->rts_suppress) {
        EscapeCommFunction(g_com_handle[device_id], SETRTS);
    }
    
    com->fdb_ptr = 1;  /* Mark as opened */
}

/* COMCLS - Close COM port */
void giocom_close(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return;
    
    HANDLE handle = get_com_handle(device_id);
    if (handle) {
        /* Send CTL-Z if ASCII output mode */
        if (!com->bin_mode && com->ascii_eof_mode) {
            char ctlz = 0x1A;  /* CTL-Z */
            DWORD written;
            if (!WriteFile(handle, &ctlz, 1, &written, NULL)) {
                /* Failed to write CTL-Z, but continue closing */
            }
        }
        
        /* Terminate COM channel */
        EscapeCommFunction(handle, CLRRTS);
        CloseHandle(handle);
        g_com_handle[device_id] = NULL;
    }
    
    /* Reset DCB */
    com->fdb_ptr = 0;
    com->pos = 0;
    com->cr_flag = 0;
}

/* COMSWD - Set device width */
void giocom_swd(int device_id, int width) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return;
    
    if (width > 0 && width <= 255) {
        com->width = width;
    }
}

/* COMSIN - Sequential Input */
int giocom_sin(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return -1;
    
    HANDLE handle = get_com_handle(device_id);
    if (!handle) return -1;
    
    char ch;
    DWORD read;
    DWORD start_time = GetTickCount();
    DWORD timeout_ms = (com->cts_timeout > 0) ? com->cts_timeout : 1000;
    
    /* Wait for data with timeout */
    while (true) {
        if (ReadFile(handle, &ch, 1, &read, NULL) && read > 0) {
            /* Check for CTL-Z in ASCII mode */
            if (!com->bin_mode && ch == 0x1A) {
                return -1;  /* EOF */
            }
            return (unsigned char)ch;
        }
        
        /* Check for errors */
        DWORD errors;
        COMSTAT stat;
        ClearCommError(handle, &errors, &stat);
        
        if (errors) {
            return -1;
        }
        
        /* Check timeout */
        if (GetTickCount() - start_time >= timeout_ms) {
            return -1;  /* Timeout - no data available */
        }
        
        /* Small delay to avoid busy waiting */
        Sleep(1);
    }
    
    return -1;
}

/* COMSOT - Sequential Output */
void giocom_sot(int device_id, int ch) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return;
    
    HANDLE handle = get_com_handle(device_id);
    if (!handle) return;
    
    /* Handle LF after CR */
    if (ch == '\n' && com->cr_flag) {
        com->cr_flag = 0;
        return;  /* Eat LF after CR */
    }
    
    /* Reset CR flag if not CR */
    if (ch != '\r') {
        com->cr_flag = 0;
    }
    
    /* Output character */
    char c = (char)ch;
    DWORD written;
    WriteFile(handle, &c, 1, &written, NULL);
    
    /* Update column position */
    if (ch == '\r') {
        com->pos = 0;
        com->cr_flag = 1;
        
        /* Send LF after CR if LF option set */
        if (com->send_lf_after_cr) {
            char lf = '\n';
            WriteFile(handle, &lf, 1, &written, NULL);
        }
    } else if (ch == '\n') {
        com->pos = 0;
    } else if (ch == '\t') {
        /* Tab - advance to next tab stop */
        com->pos = ((com->pos / 8) + 1) * 8;
        if (com->pos >= com->width) {
            com->pos = 0;
            char cr = '\r', lf = '\n';
            WriteFile(handle, &cr, 1, &written, NULL);
            WriteFile(handle, &lf, 1, &written, NULL);
        }
    } else {
        com->pos++;
        if (com->pos >= com->width) {
            com->pos = 0;
            char cr = '\r', lf = '\n';
            WriteFile(handle, &cr, 1, &written, NULL);
            WriteFile(handle, &lf, 1, &written, NULL);
        }
    }
}

/* COMLOC - Bytes in input queue */
int giocom_loc(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return 0;
    
    HANDLE handle = get_com_handle(device_id);
    if (!handle) return 0;
    
    COMSTAT stat;
    ClearCommError(handle, NULL, &stat);
    return (int)stat.cbInQue;
}

/* COMLOF - Free space in input buffer */
int giocom_lof(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return 0;
    
    HANDLE handle = get_com_handle(device_id);
    if (!handle) return 0;
    
    COMSTAT stat;
    ClearCommError(handle, NULL, &stat);
    return (int)stat.cbOutQue;  /* Approximation */
}

/* COMGPS - Get current file position */
int giocom_gps(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return 0;
    return com->pos;
}

/* COMGWD - Get device width */
int giocom_gwd(int device_id) {
    com_dcb_t *com = get_com_dcb(device_id);
    if (!com) return 255;
    return com->width;
}