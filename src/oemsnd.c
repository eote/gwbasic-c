/* OEMSND - Sound Support (SOUND, PLAY) */
/* Based on OEMSND.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdio.h>
#include <math.h>

/* Sound queue structure */
#define SNDQSZ 128  /* 128 bytes queue, 4 bytes per note */
#define P0PULS 0xBA7  /* PIT counter 0 pulses per IRQ 0 for 400 Hz */

/* Sound state */
static BYTE g_snd_tic = 0;       /* Sound timer tick flag: -1=fast(400Hz), 0=slow(18.2Hz) */
static WORD g_snd_bas = 0;       /* Modulo counter for original IRQ 0 handler */
static int g_not_tic = 0;        /* Number of fast timer ticks left for current note */
static DWORD g_oirq0 = 0;        /* Original IRQ 0 vector */

#define MAX_NOTES 32
static struct sound_note {
    WORD pit_count;      /* PIT pulse count (2 bytes) */
    WORD duration;       /* Duration in 1/400 seconds (2 bytes) */
} g_sound_queue[MAX_NOTES];

static int g_queue_head = 0;
static int g_queue_tail = 0;
static int g_queue_count = 0;

static HANDLE g_timer_thread = NULL;
static DWORD g_timer_thread_id = 0;
static BOOL g_timer_running = FALSE;
static CRITICAL_SECTION g_sound_cs;
static HANDLE g_note_event = NULL;
static BOOL g_note_playing = FALSE;
static WORD g_current_pit = 0;
static WORD g_current_duration = 0;

/* Forward declarations */
static DWORD WINAPI sound_timer_thread(LPVOID lpParam);
static void sound_reset(void);
static void sound_queue_note(WORD pit_count, WORD duration);
static BOOL sound_dequeue_note(WORD *pit_count, WORD *duration);
static void sound_play_note(WORD pit_count, WORD duration);
static void sound_stop(void);
static void sound_clear_queue(void);

/* Module initialization */
void oemsnd_init(void) {
    InitializeCriticalSection(&g_sound_cs);
    g_note_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_timer_running = FALSE;
    g_timer_thread = NULL;
    g_note_playing = FALSE;
    g_not_tic = 0;
    g_snd_tic = 0;
    g_snd_bas = 0;
    g_oirq0 = 0;
    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
    g_current_pit = 0;
    g_current_duration = 0;
    g_note_playing = FALSE;
}

/* Cleanup */
void oemsnd_term(void) {
    sound_stop();
    sound_clear_queue();
    
    if (g_timer_thread) {
        g_timer_running = FALSE;
        SetEvent(g_note_event);
        WaitForSingleObject(g_timer_thread, 1000);
        CloseHandle(g_timer_thread);
        g_timer_thread = NULL;
    }
    
    if (g_note_event) {
        CloseHandle(g_note_event);
        g_note_event = NULL;
    }
    
    DeleteCriticalSection(&g_sound_cs);
}

/* Start sound system - initialize timer thread */
void oemsnd_start(void) {
    if (g_timer_thread) return;
    
    g_timer_running = TRUE;
    g_timer_thread = CreateThread(NULL, 0, sound_timer_thread, NULL, 0, &g_timer_thread_id);
}

/* Timer thread - simulates 400 Hz IRQ 0 */
static DWORD WINAPI sound_timer_thread(LPVOID lpParam) {
    (void)lpParam;
    
    while (g_timer_running) {
        Sleep(2);  /* ~500 Hz, close enough to 400 Hz for simulation */
        
        EnterCriticalSection(&g_sound_cs);
        
        if (g_snd_tic != 0) {
            /* Fast timer mode (400 Hz) */
            if (g_not_tic > 0) {
                g_not_tic--;
                if (g_not_tic == 0) {
                    /* Note finished, try to get next from queue */
                    WORD pit_count, duration;
                    if (sound_dequeue_note(&pit_count, &duration)) {
                        sound_play_note(pit_count, duration);
                    } else {
                        /* No more notes, stop sound */
                        sound_stop();
                    }
                }
            }
            
            /* Update modulo counter for original IRQ 0 */
            g_snd_bas += P0PULS;
        } else {
            /* Slow timer mode (18.2 Hz) */
            g_snd_bas += P0PULS;
            if (g_snd_bas < P0PULS) {  /* Carry occurred */
                if (g_snd_tic == 0) {
                    g_snd_tic = -1;  /* Switch to fast mode when needed */
                }
            }
        }
        
        LeaveCriticalSection(&g_sound_cs);
    }
    
    return 0;
}

/* DONOTE implementation - play/queue note or reset sound */
int oemsnd_donote(int mode, WORD frequency, WORD duration) {
    /* mode: -1 = reset, 0 = foreground, 1 = background */
    
    if (mode < 0) {
        /* Reset sound */
        sound_reset();
        return 0;  /* Success */
    }
    
    if (duration == 0) {
        /* Duration 0 - special handling (just clear current note?) */
        return 0;
    }
    
    /* Convert frequency to PIT pulse count */
    /* PIT pulse count = 1193181 / frequency (rounded) */
    WORD pit_count = 0;
    if (frequency > 0) {
        DWORD count = 1193181UL / frequency;
        if (count > 0xFFFF) count = 0xFFFF;
        pit_count = (WORD)count;
    }
    
    /* Duration is in units of 1/400 seconds (2.5 ms) */
    WORD dur = duration;
    
    if (mode == 0) {
        /* Foreground - play and wait */
        sound_play_note(pit_count, dur);
        
        /* Wait for note to finish (simplified) */
        EnterCriticalSection(&g_sound_cs);
        while (g_note_playing) {
            LeaveCriticalSection(&g_sound_cs);
            Sleep(10);
            EnterCriticalSection(&g_sound_cs);
        }
        LeaveCriticalSection(&g_sound_cs);
    } else {
        /* Background - queue the note */
        sound_queue_note(pit_count, duration);
        
        /* Start timer thread if not running */
        if (!g_timer_thread) {
            oemsnd_start();
        }
    }
    
    return 0;  /* Success */
}

/* Reset sound system */
static void sound_reset(void) {
    EnterCriticalSection(&g_sound_cs);
    
    sound_stop();
    sound_clear_queue();
    
    g_not_tic = 0;
    g_snd_tic = 0;
    g_snd_bas = 0;
    
    /* Reset speaker */
    Beep(0, 0);
    
    LeaveCriticalSection(&g_sound_cs);
}

/* Stop current sound */
static void sound_stop(void) {
    Beep(0, 0);
    g_note_playing = FALSE;
    g_current_pit = 0;
    g_current_duration = 0;
}

/* Clear sound queue */
static void sound_clear_queue(void) {
    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
}

/* Queue a note for background playback */
static void sound_queue_note(WORD pit_count, WORD duration) {
    EnterCriticalSection(&g_sound_cs);
    
    if (g_queue_count < MAX_NOTES) {
        g_sound_queue[g_queue_tail].pit_count = pit_count;
        g_sound_queue[g_queue_tail].duration = duration;
        g_queue_tail = (g_queue_tail + 1) % MAX_NOTES;
        g_queue_count++;
    }
    
    LeaveCriticalSection(&g_sound_cs);
    
    /* Signal timer thread */
    SetEvent(g_note_event);
}

/* Dequeue next note */
static BOOL sound_dequeue_note(WORD *pit_count, WORD *duration) {
    EnterCriticalSection(&g_sound_cs);
    
    if (g_queue_count == 0) {
        LeaveCriticalSection(&g_sound_cs);
        return FALSE;
    }
    
    *pit_count = g_sound_queue[g_queue_head].pit_count;
    *duration = g_sound_queue[g_queue_head].duration;
    g_queue_head = (g_queue_head + 1) % MAX_NOTES;
    g_queue_count--;
    
    LeaveCriticalSection(&g_sound_cs);
    return TRUE;
}

/* Play a note immediately using Windows Beep API */
static void sound_play_note(WORD pit_count, WORD duration) {
    if (pit_count == 0) {
        /* Rest note */
        g_current_pit = 0;
        g_current_duration = duration;
        g_note_playing = TRUE;
        Sleep(duration * 2);  /* duration in 1/400 sec units */
        g_note_playing = FALSE;
        return;
    }
    
    /* Convert pit_count back to frequency for Beep */
    /* frequency = 1193181 / pit_count */
    DWORD frequency = 1193181UL / pit_count;
    if (frequency < 37) frequency = 37;
    if (frequency > 32767) frequency = 32767;
    
    /* Duration in milliseconds: duration * 2.5 */
    DWORD ms = duration * 25 / 10;  /* duration * 2.5 */
    
    g_current_pit = pit_count;
    g_current_duration = duration;
    g_note_playing = TRUE;
    
    Beep((DWORD)frequency, ms);
    
    g_note_playing = FALSE;
    g_current_pit = 0;
    g_current_duration = 0;
}

/* SNDSTA - Start sound support (install IRQ 0 handler) */
void oemsnd_sndsta(void) {
    oemsnd_start();
}

/* SNDTRM - Terminate sound support */
void oemsnd_sndtrm(void) {
    oemsnd_term();
}

/* PLAY statement support - parse and play music string */
void oemsnd_play(const char *music_string) {
    if (!music_string) return;
    
    /* Parse PLAY string format: 
     * A-G notes, O octave, N note, L length, T tempo, etc.
     * For now, just a stub
     */
    
    /* Simple implementation: just beep for demo */
    EnterCriticalSection(&g_sound_cs);
    Beep(440, 500);  /* A4 for 500ms */
    LeaveCriticalSection(&g_sound_cs);
}

/* SOUND statement - play frequency for duration */
void oemsnd_sound(WORD frequency, WORD duration) {
    if (frequency == 0) return;
    
    WORD pit_count = 0;
    if (frequency > 0) {
        DWORD count = 1193181UL / frequency;
        if (count > 0xFFFF) count = 0xFFFF;
        pit_count = (WORD)count;
    }
    
    /* Duration in clock ticks (1/18.2 sec) -> convert to 1/400 sec units */
    WORD dur = duration * 22 / 100;  /* approx conversion */
    if (dur == 0) dur = 1;
    
    oemsnd_donote(0, pit_count, dur);  /* Foreground */
}