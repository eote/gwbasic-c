/* TEST_OEMSND - Unit tests for OEMSND (Sound Support) */
/* Based on OEMSND.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test oemsnd_init */
void test_oemsnd_init(void) {
    oemsnd_init();
    TEST_PASS("oemsnd_init");
}

/* Test oemsnd_term */
void test_oemsnd_term(void) {
    oemsnd_init();
    oemsnd_term();
    TEST_PASS("oemsnd_term");
}

/* Test oemsnd_donote - reset mode (-1) */
void test_oemsnd_donote_reset(void) {
    oemsnd_init();
    int res = oemsnd_donote(-1, 0, 0);
    TEST_ASSERT(res == 0, "donote reset returns success");
    TEST_PASS("oemsnd_donote_reset");
}

/* Test oemsnd_donote - foreground mode (0) */
void test_oemsnd_donote_foreground(void) {
    oemsnd_init();
    int res = oemsnd_donote(0, 440, 100);  /* A4, 100 ticks */
    TEST_ASSERT(res == 0, "donote foreground returns success");
    TEST_PASS("oemsnd_donote_foreground");
}

/* Test oemsnd_donote - background mode (1) */
void test_oemsnd_donote_background(void) {
    oemsnd_init();
    int res = oemsnd_donote(1, 440, 100);  /* A4, 100 ticks background */
    TEST_ASSERT(res == 0, "donote background returns success");
    TEST_PASS("oemsnd_donote_background");
}

/* Test oemsnd_donote - zero frequency (rest) */
void test_oemsnd_donote_rest(void) {
    oemsnd_init();
    int res = oemsnd_donote(0, 0, 100);  /* Rest note */
    TEST_ASSERT(res == 0, "donote rest returns success");
    TEST_PASS("oemsnd_donote_rest");
}

/* Test oemsnd_donote - duration 0 */
void test_oemsnd_donote_zero_duration(void) {
    oemsnd_init();
    int res = oemsnd_donote(0, 440, 0);  /* Zero duration */
    TEST_ASSERT(res == 0, "donote zero duration returns success");
    TEST_PASS("oemsnd_donote_zero_duration");
}

/* Test oemsnd_sound */
void test_oemsnd_sound(void) {
    oemsnd_init();
    oemsnd_sound(440, 100);  /* A4, 100 clock ticks */
    TEST_PASS("oemsnd_sound");
}

/* Test oemsnd_play */
void test_oemsnd_play(void) {
    oemsnd_init();
    oemsnd_play("CDE");  /* Simple play string */
    TEST_PASS("oemsnd_play");
}

/* Test oemsnd_start and term */
void test_oemsnd_start_term(void) {
    oemsnd_init();
    oemsnd_start();
    oemsnd_term();
    TEST_PASS("oemsnd_start_term");
}

/* Test oemsnd_sndsta and sndtrm */
void test_oemsnd_sndsta_sndtrm(void) {
    oemsnd_init();
    oemsnd_sndsta();
    oemsnd_sndtrm();
    TEST_PASS("oemsnd_sndsta_sndtrm");
}

/* Test multiple notes */
void test_oemsnd_multiple_notes(void) {
    oemsnd_init();
    
    /* Play a sequence of notes in background */
    oemsnd_donote(1, 262, 50);  /* C4 */
    oemsnd_donote(1, 294, 50);  /* D4 */
    oemsnd_donote(1, 330, 50);  /* E4 */
    oemsnd_donote(1, 349, 50);  /* F4 */
    oemsnd_donote(1, 392, 50);  /* G4 */
    
    /* Wait a bit for notes to process */
    Sleep(100);
    
    TEST_PASS("oemsnd_multiple_notes");
}

/* Test sound reset */
void test_oemsnd_reset(void) {
    oemsnd_init();
    
    /* Queue some notes */
    oemsnd_donote(1, 440, 50);
    oemsnd_donote(1, 880, 50);
    
    /* Reset */
    oemsnd_donote(-1, 0, 0);
    
    TEST_PASS("oemsnd_reset");
}

/* Test sound play various frequencies */
void test_oemsnd_various_frequencies(void) {
    oemsnd_init();
    
    /* Test low frequency */
    oemsnd_sound(100, 50);
    
    /* Test mid frequency */
    oemsnd_sound(440, 50);
    
    /* Test high frequency */
    oemsnd_sound(2000, 50);
    
    TEST_PASS("oemsnd_various_frequencies");
}

/* Test sound stop */
void test_oemsnd_stop(void) {
    oemsnd_init();
    oemsnd_donote(1, 440, 100);
    Sleep(10);
    oemsnd_donote(-1, 0, 0);  /* Reset stops sound */
    TEST_PASS("oemsnd_stop");
}

/* Test PLAY command */
void test_oemsnd_play_command(void) {
    oemsnd_init();
    oemsnd_play("O4 L8 CDEFGAB");  /* Octave 4, eighth notes, C major scale */
    TEST_PASS("oemsnd_play_command");
}

/* Test edge cases */
void test_oemsnd_edge_cases(void) {
    oemsnd_init();
    
    /* Very high frequency */
    oemsnd_sound(30000, 50);
    
    /* Very low frequency */
    oemsnd_sound(37, 50);
    
    /* Long duration */
    oemsnd_sound(440, 1000);
    
    TEST_PASS("oemsnd_edge_cases");
}

/* Test cleanup on term */
void test_oemsnd_term_cleanup(void) {
    oemsnd_init();
    oemsnd_start();
    oemsnd_term();
    oemsnd_term();  /* Double cleanup should be safe */
    TEST_PASS("oemsnd_term_cleanup");
}