/* OEMCBK - Ctrl-Break & Pause Handling */
/* Based on OEMCBK.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdio.h>

/* Global state for Ctrl-Break and Pause handling */
static BOOL g_ctrl_break_enabled = FALSE;
static BOOL g_pause_requested = FALSE;
static BOOL g_ctrl_break_occurred = FALSE;
static HANDLE g_console_input = NULL;
static DWORD g_original_console_mode = 0;

/* Ctrl-Break handler */
static BOOL WINAPI ctrl_break_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_C_EVENT) {
        g_ctrl_break_occurred = TRUE;
        return TRUE;  /* We handled it */
    }
    return FALSE;
}

/* Module initialization */
void oemcbk_init(void) {
    g_ctrl_break_enabled = FALSE;
    g_pause_requested = FALSE;
    g_ctrl_break_occurred = FALSE;
    
    /* Get console input handle */
    g_console_input = GetStdHandle(STD_INPUT_HANDLE);
    if (g_console_input != INVALID_HANDLE_VALUE) {
        GetConsoleMode(g_console_input, &g_original_console_mode);
    }
    
    /* Set up Ctrl-Break handler */
    SetConsoleCtrlHandler(ctrl_break_handler, TRUE);
}

/* Start Ctrl-Break & Pause emulation */
void oem_cbksta(void) {
    g_ctrl_break_enabled = TRUE;
    g_ctrl_break_occurred = FALSE;
    
    /* Enable extended input mode for better key detection */
    if (g_console_input != INVALID_HANDLE_VALUE) {
        SetConsoleMode(g_console_input, g_original_console_mode | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    }
}

/* Cleanup Ctrl-Break & Pause handling */
void oem_cbktrm(void) {
    g_ctrl_break_enabled = FALSE;
    
    /* Restore original console mode */
    if (g_console_input != INVALID_HANDLE_VALUE) {
        SetConsoleMode(g_console_input, g_original_console_mode);
    }
    
    /* Remove Ctrl-Break handler */
    SetConsoleCtrlHandler(ctrl_break_handler, FALSE);
}

/* Check if Ctrl-Break occurred */
BOOL oem_check_ctrl_break(void) {
    if (g_ctrl_break_occurred) {
        g_ctrl_break_occurred = FALSE;
        return TRUE;
    }
    return FALSE;
}

/* Check if Pause was requested */
BOOL oem_check_pause(void) {
    if (g_pause_requested) {
        g_pause_requested = FALSE;
        return TRUE;
    }
    return FALSE;
}

/* Simulate Ctrl-Break (called when Ctrl-Shift-B detected) */
void oem_simulate_ctrl_break(void) {
    g_ctrl_break_occurred = TRUE;
    
    /* Clear keyboard buffer */
    FlushConsoleInputBuffer(g_console_input);
    
    /* Also trigger the handler for good measure */
    ctrl_break_handler(CTRL_BREAK_EVENT);
}

/* Simulate Pause (called when Ctrl-Shift-P or Ctrl-NumLock detected) */
void oem_simulate_pause(void) {
    g_pause_requested = TRUE;
    
    /* Insert special key code into buffer */
    INPUT_RECORD ir;
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = VK_PAUSE;
    ir.Event.KeyEvent.wVirtualScanCode = 0x45;  /* NumLock scan code */
    ir.Event.KeyEvent.uChar.AsciiChar = 0;
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
    
    WriteConsoleInput(g_console_input, &ir, 1, NULL);
}

/* Process keyboard input for special key combinations */
void oem_process_keyboard_input(INPUT_RECORD *ir) {
    if (!g_ctrl_break_enabled) return;
    
    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
        WORD vk = ir->Event.KeyEvent.wVirtualKeyCode;
        DWORD ctrl_state = ir->Event.KeyEvent.dwControlKeyState;
        
        BOOL ctrl_pressed = (ctrl_state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        BOOL shift_pressed = (ctrl_state & SHIFT_PRESSED) != 0;
        
        /* Ctrl-Shift-B -> Ctrl-Break */
        if (ctrl_pressed && shift_pressed && vk == 'B') {
            oem_simulate_ctrl_break();
            ir->Event.KeyEvent.bKeyDown = FALSE;  /* Consume the key */
        }
        /* Ctrl-Shift-P -> Pause */
        else if (ctrl_pressed && shift_pressed && vk == 'P') {
            oem_simulate_pause();
            ir->Event.KeyEvent.bKeyDown = FALSE;  /* Consume the key */
        }
        /* Ctrl-NumLock -> Pause */
        else if (ctrl_pressed && vk == VK_NUMLOCK) {
            oem_simulate_pause();
            ir->Event.KeyEvent.bKeyDown = FALSE;  /* Consume the key */
        }
    }
}