/* OEMEV - Event Handling (Joystick, Light Pen, Timer Events) */
/* Based on OEMEV.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdio.h>
#include <mmsystem.h>

/* Joystick state */
static JOYINFOEX g_joy_info_a = {0};
static JOYINFOEX g_joy_info_b = {0};
static BOOL g_joy_a_present = FALSE;
static BOOL g_joy_b_present = FALSE;

/* Light pen state */
static BOOL g_pen_enabled = FALSE;
static int g_pen_0v = 0;          /* PEN(0) - pen down since last poll */
static int g_pen_prev = 0;        /* Previous pen state */
static int g_pen_1v = 0;          /* PEN(1) - X when activated */
static int g_pen_2v = 0;          /* PEN(2) - Y when activated */
static int g_pen_4v = 0;          /* PEN(4) - last valid X */
static int g_pen_5v = 0;          /* PEN(5) - last valid Y */
static int g_pen_76vm = 0;        /* PEN(7,6) - char col/row when activated */
static int g_pen_98vm = 0;        /* PEN(9,8) - last valid char col/row */

/* Joystick button latching */
static BYTE g_trg_lat = 0xF0;     /* Latched buttons since last RDTRIG */

/* Event flag */
static BYTE g_event = 0;          /* Event occurred since last POLLEV */

/* Timer callback for event polling */
static MMRESULT g_timer_id = 0;

/* Function pointers for joystick reading (port I/O vs BIOS) */
static void (*g_rdtrg_func)(void) = NULL;
static void (*g_rdstk_func)(void) = NULL;

/* Forward declarations for internal functions */
static void oemev_rdtrg_port_impl(void);
static void oemev_rdtrg_bios_impl(void);
static void oemev_rdstk_port_impl(void);
static void oemev_rdstk_bios_impl(void);
static void CALLBACK oemev_timer_callback_impl(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

/* Module initialization */
void oemev_init(void) {
    g_joy_info_a.dwSize = sizeof(JOYINFOEX);
    g_joy_info_a.dwFlags = JOY_RETURNALL;
    g_joy_info_b.dwSize = sizeof(JOYINFOEX);
    g_joy_info_b.dwFlags = JOY_RETURNALL;
    
    /* Check for joystick A (JOYSTICKID1) */
    MMRESULT res = joyGetPosEx(JOYSTICKID1, &g_joy_info_a);
    g_joy_a_present = (res == JOYERR_NOERROR);
    
    /* Check for joystick B (JOYSTICKID2) */
    res = joyGetPosEx(JOYSTICKID2, &g_joy_info_b);
    g_joy_b_present = (res == JOYERR_NOERROR);
    
    /* Default to port I/O style functions */
    g_rdtrg_func = oemev_rdtrg_port_impl;
    g_rdstk_func = oemev_rdstk_port_impl;
    
    /* If BIOS joystick support available, use BIOS functions */
    if (g_joy_a_present || g_joy_b_present) {
        g_rdtrg_func = oemev_rdtrg_bios_impl;
        g_rdstk_func = oemev_rdstk_bios_impl;
    }
    
    g_pen_enabled = FALSE;
    g_pen_0v = 0;
    g_pen_prev = 0;
    g_pen_1v = 0;
    g_pen_2v = 0;
    g_pen_4v = 0;
    g_pen_5v = 0;
    g_pen_76vm = 0;
    g_pen_98vm = 0;
    g_trg_lat = 0xF0;
    g_event = 0;
    g_timer_id = 0;
}

/* Start event handling - install timer tick handler */
void oemev_evsta(void) {
    oemev_init();
    
    /* Set up multimedia timer for ~18.2 Hz (55ms) - simulates INT 1CH */
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR) {
        g_timer_id = timeSetEvent(55, tc.wPeriodMin, oemev_timer_callback_impl, 0, TIME_PERIODIC);
    }
}

/* Cleanup event handling */
void oemev_evtrm(void) {
    if (g_timer_id != 0) {
        timeKillEvent(g_timer_id);
        g_timer_id = 0;
    }
}

/* Timer callback - simulates INT 1CH handler */
static void CALLBACK oemev_timer_callback_impl(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    (void)uTimerID; (void)uMsg; (void)dwUser; (void)dw1; (void)dw2;
    
    /* Poll joystick buttons */
    if (g_rdtrg_func) {
        g_rdtrg_func();
        
        /* Check if any joystick buttons pressed (high nibble) */
        BYTE buttons = 0;
        if (g_joy_a_present) {
            joyGetPosEx(JOYSTICKID1, &g_joy_info_a);
            buttons |= (g_joy_info_a.dwButtons & 0x0F) << 4;
        }
        if (g_joy_b_present) {
            joyGetPosEx(JOYSTICKID2, &g_joy_info_b);
            buttons |= (g_joy_info_b.dwButtons & 0x0F);
        }
        
        if ((buttons & 0xF0) != 0xF0) {
            g_event = 1;
            g_trg_lat &= buttons;
        }
    }
    
    /* Poll light pen if enabled */
    if (g_pen_enabled) {
        /* In Windows, we can't easily detect light pen.
         * This would require specialized hardware. 
         * For now, we simulate by checking mouse if needed. */
    }
}

/* Port I/O style joystick button read */
static void oemev_rdtrg_port_impl(void) {
    /* Not applicable on Windows - use BIOS instead */
}

/* BIOS style joystick button read */
static void oemev_rdtrg_bios_impl(void) {
    if (g_joy_a_present) {
        joyGetPosEx(JOYSTICKID1, &g_joy_info_a);
    }
    if (g_joy_b_present) {
        joyGetPosEx(JOYSTICKID2, &g_joy_info_b);
    }
}

/* Port I/O style joystick coordinate read */
static void oemev_rdstk_port_impl(void) {
    /* Not applicable on Windows */
}

/* BIOS style joystick coordinate read */
static void oemev_rdstk_bios_impl(void) {
    if (g_joy_a_present) {
        joyGetPosEx(JOYSTICKID1, &g_joy_info_a);
    }
    if (g_joy_b_present) {
        joyGetPosEx(JOYSTICKID2, &g_joy_info_b);
    }
}

/* POLLEV - Poll for events */
BOOL oemev_pollev(void) {
    BOOL result = (g_event != 0);
    g_event = 0;
    return result;
}

/* RDPEN - Read light pen status or enable/disable */
int oemev_rdpen(int func, int *result) {
    if (func == 0xFE) {         /* PEN ON */
        g_pen_enabled = TRUE;
        return 0;
    }
    if (func == 0xFF) {         /* PEN OFF */
        g_pen_enabled = FALSE;
        return 0;
    }
    if (func < 0 || func > 9) { /* Invalid */
        return -1;
    }
    if (!g_pen_enabled) {       /* PEN not enabled */
        return -1;
    }
    
    switch (func) {
        case 0: *result = g_pen_0v; break;           /* Pen down since last poll */
        case 1: *result = g_pen_1v; break;           /* X when activated */
        case 2: *result = g_pen_2v; break;           /* Y when activated */
        case 3: *result = (g_pen_prev != 0) ? -1 : 0; break; /* Current switch state */
        case 4: *result = g_pen_4v; break;           /* Last valid X */
        case 5: *result = g_pen_5v; break;           /* Last valid Y */
        case 6: *result = (g_pen_76vm & 0xFF) - 1; break; /* Row - 1 */
        case 7: *result = (g_pen_76vm >> 8) - 1; break;  /* Col - 1 */
        case 8: *result = (g_pen_98vm & 0xFF) - 1; break; /* Row - 1 */
        case 9: *result = (g_pen_98vm >> 8) - 1; break;  /* Col - 1 */
        default: return -1;
    }
    return 0;
}

/* RDTRIG - Read joystick button status */
int oemev_rdtrig(int button, int mode, int *result) {
    if (button < 0 || button > 3) {
        *result = 0;
        return -1;
    }
    
    if (g_rdtrg_func) {
        g_rdtrg_func();
    }
    
    BYTE buttons = 0;
    if (g_joy_a_present) {
        buttons |= (g_joy_info_a.dwButtons & 0x03) << 4;  /* A buttons 0,1 */
    }
    if (g_joy_b_present) {
        buttons |= (g_joy_info_b.dwButtons & 0x03);        /* B buttons 0,1 */
    }
    
    /* Map button index to bit position */
    int bit_pos = 4 + button;  /* Buttons are in bits 4-7 */
    
    if (mode == 0) {
        /* Current status */
        *result = (buttons & (1 << bit_pos)) ? 1 : 0;
    } else {
        /* Latched status */
        *result = (g_trg_lat & (1 << bit_pos)) ? 1 : 0;
        
        /* Update latch */
        g_trg_lat |= buttons;
    }
    
    return 0;
}

/* RDSTIK - Read joystick coordinate (STICK function) */
int oemev_rdstik(int coord, int *result) {
    if (coord < 0 || coord > 3) {
        return -1;
    }
    
    if (g_rdstk_func) {
        g_rdstk_func();
    }
    
    switch (coord) {
        case 0: *result = g_joy_a_present ? (int)g_joy_info_a.dwXpos : 0; break;  /* STICK(0) - X of A */
        case 1: *result = g_joy_a_present ? (int)g_joy_info_a.dwYpos : 0; break;  /* STICK(1) - Y of A */
        case 2: *result = g_joy_b_present ? (int)g_joy_info_b.dwXpos : 0; break;  /* STICK(2) - X of B */
        case 3: *result = g_joy_b_present ? (int)g_joy_info_b.dwYpos : 0; break;  /* STICK(3) - Y of B */
        default: return -1;
    }
    
    return 0;
}