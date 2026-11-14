/* CALL86 - 8086 CALL Statement */
/* Based on CALL86.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdio.h>
#include <string.h>

/* External references */
extern void gweval_init(void);
extern double gweval_eval_numeric(const char *expr);
extern void gweval_scan_var(const char **text, int *var_ptr);
extern void gweval_synchr(const char **text, char expected);
extern void gweval_chrgt2(const char **text);
extern int gweval_frqint(void);

/* Global state */
static int g_call_temp = 0;
static int g_call_tempa = 0;
static int g_savseg = 0;
static int g_subflg = 0;
static int g_saryfl = 0;

/* Forward declarations */
static void call86_callst(const char **text);
void call86_call_impl(const char **text, int is_calls);

/* Module initialization */
void call86_init(void) {
    g_call_temp = 0;
    g_call_tempa = 0;
    g_savseg = 0;
    g_subflg = 0;
    g_saryfl = 0;
}

/* CALL statement - calls machine language subroutine */
void call86_call(const char **text) {
    call86_call_impl(text, 0);  /* Clear carry = CALL */
}

/* CALLS statement - calls with segment:offset */
void call86_calls(const char **text) {
    call86_call_impl(text, 1);  /* Set carry = CALLS */
}

/* Internal implementation for both CALL and CALLS */
void call86_call_impl(const char **text, int is_calls) {
    (void)is_calls;
    (void)text;
    /* Simplified implementation for testing - avoid crash */
    g_subflg = 0200;
    g_saryfl = 0;
    
    /* Just advance the text pointer past the CALL keyword and routine name */
    if (text && *text) {
        while (**text && **text != ' ' && **text != '(' && **text != '\0') {
            (*text)++;
        }
        while (**text == ' ') (*text)++;
    }
    
    if (**text == '(') {
        while (**text && **text != ')') (*text)++;
        if (**text == ')') (*text)++;
    }
    
    call86_callst(text);
}

/* CALLST - Call the subroutine */
static void call86_callst(const char **text) {
    (void)text;
    g_call_temp = 0;  /* Save text pointer */
    
    /* In real implementation, this would:
     * 1. Save BASIC code segment (CS)
     * 2. Push return address (CALLRT)
     * 3. Push subroutine segment (SAVSEG)
     * 4. Push subroutine address (TEMPA)
     * 5. Do a far call to the subroutine
     * 
     * For our C implementation, we simulate this with a function pointer call
     */
    
    if (g_call_tempa != 0) {
        /* Cast address to function pointer and call */
        typedef void (*subroutine_t)(void);
        subroutine_t sub = (subroutine_t)(uintptr_t)g_call_tempa;
        
        /* Call the machine language subroutine */
        if (sub) {
            sub();
        }
    }
    
    /* Return address would be restored here */
}

/* USR function - call user machine language routine */
float call86_usr(int usr_num, float arg) {
    (void)arg;
    /* USR function calls user-defined machine language routine
     * USR0-USR9 correspond to DEF USR statements
     */
    if (usr_num < 0 || usr_num > 9) return 0.0f;
    
    /* Get USR routine address from DEF USR table */
    /* For now, return 0 */
    return 0.0f;
    }

    /* DEF USR - define USR function address */
    void call86_defusr(int usr_num, int address) {
        (void)address;
        if (usr_num < 0 || usr_num > 9) return;
    
        /* Store address in USR table */
        /* Implementation would store in DEF USR table */
    }