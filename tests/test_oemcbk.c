/* TEST_OEMCBK - Unit tests for OEMCBK (Ctrl-Break & Pause Handling) */
/* Based on OEMCBK.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test oemcbk_init */
void test_oemcbk_init(void) {
    oemcbk_init();
    TEST_PASS("oemcbk_init");
}

/* Test oem_cbksta */
void test_oem_cbksta(void) {
    oemcbk_init();
    oem_cbksta();
    TEST_PASS("oem_cbksta");
}

/* Test oem_cbktrm */
void test_oem_cbktrm(void) {
    oemcbk_init();
    oem_cbksta();
    oem_cbktrm();
    TEST_PASS("oem_cbktrm");
}

/* Test oem_check_ctrl_break */
void test_oem_check_ctrl_break(void) {
    oemcbk_init();
    oem_cbksta();
    BOOL result = oem_check_ctrl_break();
    TEST_ASSERT(result == FALSE, "No ctrl-break initially");
    TEST_PASS("oem_check_ctrl_break");
}

/* Test oem_check_pause */
void test_oem_check_pause(void) {
    oemcbk_init();
    oem_cbksta();
    BOOL result = oem_check_pause();
    TEST_ASSERT(result == FALSE, "No pause initially");
    TEST_PASS("oem_check_pause");
}

/* Test oem_simulate_ctrl_break */
void test_oem_simulate_ctrl_break(void) {
    oemcbk_init();
    oem_cbksta();
    oem_simulate_ctrl_break();
    BOOL result = oem_check_ctrl_break();
    TEST_ASSERT(result == TRUE, "Ctrl-break detected after simulation");
    TEST_PASS("oem_simulate_ctrl_break");
}

/* Test oem_simulate_pause */
void test_oem_simulate_pause(void) {
    oemcbk_init();
    oem_cbksta();
    oem_simulate_pause();
    BOOL result = oem_check_pause();
    TEST_ASSERT(result == TRUE, "Pause detected after simulation");
    TEST_PASS("oem_simulate_pause");
}

/* Test oem_process_keyboard_input - Ctrl-Shift-B */
void test_oem_process_keyboard_ctrl_shift_b(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result = oem_check_ctrl_break();
    TEST_ASSERT(result == TRUE, "Ctrl-break detected from Ctrl-Shift-B");
    TEST_PASS("oem_process_keyboard_ctrl_shift_b");
}

/* Test oem_process_keyboard_input - Ctrl-Shift-P */
void test_oem_process_keyboard_ctrl_shift_p(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'P';
    ir.Event.KeyEvent.wVirtualScanCode = 0x19;
    ir.Event.KeyEvent.uChar.AsciiChar = 'P';
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result = oem_check_pause();
    TEST_ASSERT(result == TRUE, "Pause detected from Ctrl-Shift-P");
    TEST_PASS("oem_process_keyboard_ctrl_shift_p");
}

/* Test oem_process_keyboard_input - Ctrl-NumLock */
void test_oem_process_keyboard_ctrl_numlock(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMLOCK;
    ir.Event.KeyEvent.wVirtualScanCode = 0x45;
    ir.Event.KeyEvent.uChar.AsciiChar = 0;
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result = oem_check_pause();
    TEST_ASSERT(result == TRUE, "Pause detected from Ctrl-NumLock");
    TEST_PASS("oem_process_keyboard_ctrl_numlock");
}

/* Test oem_process_keyboard_input - regular key (should not trigger) */
void test_oem_process_keyboard_regular(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'A';
    ir.Event.KeyEvent.wVirtualScanCode = 0x1E;
    ir.Event.KeyEvent.uChar.AsciiChar = 'A';
    ir.Event.KeyEvent.dwControlKeyState = 0;
    
    oem_process_keyboard_input(&ir);
    BOOL result_cb = oem_check_ctrl_break();
    BOOL result_p = oem_check_pause();
    TEST_ASSERT(result_cb == FALSE, "No ctrl-break from regular key");
    TEST_ASSERT(result_p == FALSE, "No pause from regular key");
    TEST_PASS("oem_process_keyboard_regular");
}

/* Test oem_process_keyboard_input - Ctrl only (no shift) */
void test_oem_process_keyboard_ctrl_only(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result_cb = oem_check_ctrl_break();
    TEST_ASSERT(result_cb == FALSE, "No ctrl-break from Ctrl-B (no shift)");
    TEST_PASS("oem_process_keyboard_ctrl_only");
}

/* Test oem_process_keyboard_input - Shift only (no ctrl) */
void test_oem_process_keyboard_shift_only(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result_cb = oem_check_ctrl_break();
    TEST_ASSERT(result_cb == FALSE, "No ctrl-break from Shift-B (no ctrl)");
    TEST_PASS("oem_process_keyboard_shift_only");
}

/* Test oem_process_keyboard_input - key up (should not trigger) */
void test_oem_process_keyboard_key_up(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = FALSE;  /* Key up */
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result_cb = oem_check_ctrl_break();
    TEST_ASSERT(result_cb == FALSE, "No ctrl-break from key up");
    TEST_PASS("oem_process_keyboard_key_up");
}

/* Test double init/term */
void test_oemcbk_init_term_double(void) {
    oemcbk_init();
    oem_cbksta();
    oem_cbktrm();
    oemcbk_init();
    oem_cbksta();
    oem_cbktrm();
    TEST_PASS("oemcbk_init_term_double");
}

/* Test ctrl_break_occurred flag is cleared after check */
void test_oemcbk_ctrl_break_cleared(void) {
    oemcbk_init();
    oem_cbksta();
    oem_simulate_ctrl_break();
    
    BOOL result1 = oem_check_ctrl_break();
    TEST_ASSERT(result1 == TRUE, "First check returns TRUE");
    
    BOOL result2 = oem_check_ctrl_break();
    TEST_ASSERT(result2 == FALSE, "Second check returns FALSE (flag cleared)");
    TEST_PASS("oemcbk_ctrl_break_cleared");
}

/* Test pause flag is cleared after check */
void test_oemcbk_pause_cleared(void) {
    oemcbk_init();
    oem_cbksta();
    oem_simulate_pause();
    
    BOOL result1 = oem_check_pause();
    TEST_ASSERT(result1 == TRUE, "First check returns TRUE");
    
    BOOL result2 = oem_check_pause();
    TEST_ASSERT(result2 == FALSE, "Second check returns FALSE (flag cleared)");
    TEST_PASS("oemcbk_pause_cleared");
}

/* Test without init (should not crash) */
void test_oemcbk_without_init(void) {
    oem_check_ctrl_break();
    oem_check_pause();
    oem_simulate_ctrl_break();
    oem_simulate_pause();
    TEST_PASS("oemcbk_without_init");
}

/* Test process keyboard without cbksta */
void test_oemcbk_process_without_cbksta(void) {
    oemcbk_init();
    /* Don't call oem_cbksta() */
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result = oem_check_ctrl_break();
    TEST_ASSERT(result == FALSE, "No ctrl-break when cbksta not called");
    TEST_PASS("oemcbk_process_without_cbksta");
}

/* Test right ctrl/shift combinations */
void test_oemcbk_right_ctrl_shift(void) {
    oemcbk_init();
    oem_cbksta();
    
    INPUT_RECORD ir = {0};
    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = TRUE;
    ir.Event.KeyEvent.wRepeatCount = 1;
    ir.Event.KeyEvent.wVirtualKeyCode = 'B';
    ir.Event.KeyEvent.wVirtualScanCode = 0x30;
    ir.Event.KeyEvent.uChar.AsciiChar = 'B';
    ir.Event.KeyEvent.dwControlKeyState = RIGHT_CTRL_PRESSED | SHIFT_PRESSED;
    
    oem_process_keyboard_input(&ir);
    BOOL result = oem_check_ctrl_break();
    TEST_ASSERT(result == TRUE, "Ctrl-break detected from Right-Ctrl-Shift-B");
    TEST_PASS("oemcbk_right_ctrl_shift");
}