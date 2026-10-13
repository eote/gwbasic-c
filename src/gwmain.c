/* GWMAIN - Main GW-BASIC Entry Point */
/* Based on GWMAIN.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Forward declarations for module init functions */
extern void advgrp_init(void);
extern void biboot_init(void);
extern void bimisc_init(void);
extern void biprtu_init(void);
extern void biptrg_init(void);
extern void bistrs_init(void);
extern void call86_init(void);
extern void dskcom_init(void);
extern void fiveo_init(void);
extern void gengrp_init(void);
extern void gio86_init(void);
extern void giocas_init(void);
extern void giocom_init(void);
extern void giocon_init(void);
extern void giodsk_init(void);
extern void giokyb_init(void);
extern void giolpt_init(void);
extern void gioscn_init(void);
extern void giotbl_init(void);
extern void gwdata_init(void);
extern void gweval_init(void);
extern void gwinit_init(void);
extern void gwlist_init(void);
extern void gwram_init(void);
extern void gwsts_init(void);
extern void ibmres_init(void);
extern void itsa86_init(void);
extern void kanj86_init(void);
extern void maclng_init(void);
extern void math1_init(void);
extern void math2_init(void);
extern void next86_init(void);
extern void oem_init(void);
extern void oemcbk_init(void);
extern void oemev_init(void);
extern void oemsnd_init(void);
extern void scndrv_init(void);
extern void scnedt_init(void);

/* Initialize all modules */
static void init_all_modules(void) {
    /* Initialize core infrastructure first (required by other modules) */
    gwdata_init();
    gwram_init();
    
    advgrp_init();
    bimisc_init();
    biprtu_init();
    biptrg_init();
    bistrs_init();
    call86_init();
    dskcom_init();
    fiveo_init();
    gengrp_init();
    gio86_init();
    giocas_init();
    giocom_init();
    giocon_init();
    giodsk_init();
    giokyb_init();
    giolpt_init();
    gioscn_init();
    giotbl_init();
    gweval_init();
    gwinit_init();
    gwlist_init();
    gwsts_init();
    ibmres_init();
    itsa86_init();
    kanj86_init();
    maclng_init();
    math1_init();
    math2_init();
    next86_init();
    oem_init();
    oemcbk_init();
    oemev_init();
    oemsnd_init();
    scndrv_init();
    scnedt_init();
}

/* Check if string starts with a line number */
static bool has_line_number(const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    return *line && isdigit((unsigned char)*line);
}

/* Extract line number from start of string */
static int extract_line_number(const char *line, const char **rest) {
    while (*line && isspace((unsigned char)*line)) line++;
    
    int num = 0;
    while (*line && isdigit((unsigned char)*line)) {
        num = num * 10 + (*line - '0');
        line++;
    }
    
    while (*line && isspace((unsigned char)*line)) line++;
    
    if (rest) *rest = line;
    return num;
}

/* Main interpreter loop */
static void run_interpreter(void) {
    char input[256];
    
    printf("GW-BASIC C Port\n");
    printf("Based on Microsoft GW-BASIC 1983 Source Code\n");
    printf("Type 'BYE' to exit\n\n");
    printf("Ok\n");
    
    while (g_state && g_state->running) {
        /* Display prompt */
        printf("Ok\n");
        fflush(stdout);
        
        /* Read input line */
        if (!fgets(input, sizeof(input), stdin)) {
            break;  /* EOF */
        }
        
        /* Remove trailing newline */
        input[strcspn(input, "\n")] = '\0';
        
        /* Skip empty lines */
        char *trimmed = input;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        if (!*trimmed) continue;
        
        /* Check for BYE/EXIT */
        if (strcasecmp(trimmed, "BYE") == 0 || strcasecmp(trimmed, "EXIT") == 0) {
            break;
        }
        
        /* Check for LIST command */
        if (strncasecmp(trimmed, "LIST", 4) == 0 && (trimmed[4] == '\0' || isspace((unsigned char)trimmed[4]))) {
            gwlist_list(0, 0);
            continue;
        }
        
        /* Check for NEW command */
        if (strcasecmp(trimmed, "NEW") == 0) {
            /* Clear program - reinitialize gwlist */
            gwlist_init();
            printf("Ok\n");
            continue;
        }
        
        /* Check for RUN command */
        if (strcasecmp(trimmed, "RUN") == 0) {
            /* Execute all program lines in order (ASM NEWST1/GONE chain).
             * CRITICAL-1 fix: a statement may DELETE lines (including the
             * current one or the next one) — never dereference a possibly-
             * freed node. Relocation:
             * - Branch 1 (current line survives): curr->next is a LIVE pointer
             *   (gwlist_delete_line already updated the chain) — use it, never
             *   the pre-execution snapshot.
             * - Branch 2 (current line deleted): find the next surviving line
             *   by walking the list with pointer-identity comparison (never
             *   read fields of freed nodes).
             * - Branch 3 (saved_next deleted too): number scan (DELETE never
             *   renumbers, so number order is stable). */
            struct program_line *curr = gwlist_get_first_line();
            while (curr) {
                if (curr->text) {
                    int line_num = curr->number;
                    struct program_line *saved_next = curr->next;
                    long link_version = gwlist_get_link_version();
                    int rc = gwsts_exec_statement(curr->text);
                    if (rc != 0) {
                        printf("Error at line %d\n", line_num);
                        break;
                    }
                    /* LOW-2 (v127): liveness walk ONLY when the statement
                     * mutated the chain (link version changed). Snapshot the
                     * version before executing; O(1) fast path otherwise. */
                    if (link_version != gwlist_get_link_version()) {
                        /* Chain mutated — liveness by POINTER IDENTITY (not
                         * number lookup; RENUM renumbers the current line while
                         * it stays alive): both alive → physical next line (ASM
                         * SAVTXT); else first surviving line > line_num. */
                        if (gwlist_is_alive(curr) && gwlist_is_alive(saved_next)) {
                            curr = saved_next;
                        } else {
                            curr = gwlist_get_first_line();
                            while (curr && curr->number <= line_num) curr = curr->next;
                        }
                        link_version = gwlist_get_link_version();
                    } else {
                        /* Chain unchanged — curr and saved_next are both valid
                         * live pointers (nothing was deleted). */
                        curr = saved_next;
                    }
                } else {
                    curr = curr->next;
                }
            }
            continue;
        }
        
        /* If line starts with number, it's a program line */
        if (has_line_number(trimmed)) {
            const char *rest;
            int line_num = extract_line_number(trimmed, &rest);
            
            if (line_num > 0 && line_num <= 65529) {
                if (*rest == '\0') {
                    /* Delete line */
                    gwlist_delete_line(line_num);
                } else {
                    /* Add/replace line */
                    gwlist_add_line(line_num, rest);
                }
                printf("Ok\n");
            } else {
                printf("Illegal line number\n");
            }
            continue;
        }
        
        /* Otherwise execute as direct statement */
        int rc = gwsts_exec_statement(trimmed);
        if (rc != 0) {
            printf("Error executing statement\n");
        }
    }
    
    printf("\nGoodbye!\n");
}

/* Main entry point */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    /* Initialize all modules (gwdata_init allocates g_state) */
    init_all_modules();

    if (!g_state) {
        return ERROR;
    }

    DEBUG_PRINT("GWBASIC starting");

    /* If a .bas file is provided as argument, load and run it */
    if (argc >= 2) {
        const char *filename = argv[1];
        DEBUG_PRINT("Loading program: %s", filename);
        
        /* Load the program file */
        int load_rc = gwlist_load_file(filename);
        if (load_rc != SUCCESS) {
            printf("Error loading file: %s\n", filename);
            free(g_state);
            g_state = NULL;
            return ERROR;
        }
        
        DEBUG_PRINT("Program loaded, executing...");
        
        /* Initialize DATA pointer for READ/DATA support */
        bimisc_data_init();
        
        /* Execute all program lines in order - with jump handling for FOR/GOTO/GOSUB/RETURN */
        struct program_line *curr = gwlist_get_first_line();
        int exec_rc = SUCCESS;
        
        while (curr) {
            if (curr->text) {
                DEBUG_PRINT("Executing line %d: '%s'", curr->number, curr->text);
                
                /* Set current line for FOR/GOTO handling */
                extern WORD g_curlin;
                g_curlin = curr->number;
                g_state->current_line = curr->number;
                
                /* CRITICAL-1 fix: snapshot identity before executing — a
                 * statement may DELETE the current line (freeing curr).
                 * LOW-2 (v127): also snapshot the link version. */
                int line_num = curr->number;
                struct program_line *saved_next = curr->next;
                long link_version = gwlist_get_link_version();
                
                int was_in_handler = gwdata_is_in_error_handler();
                int rc = gwsts_exec_statement(curr->text);
                DEBUG_PRINT("Statement returned %d", rc);
                
                /* Check if an error occurred during expression evaluation (e.g., division by zero) */
                                if (gwdata_get_error() != 0) {
                                    /* GW-BASIC: an error raised while ALREADY
                                     * inside the handler is FATAL — never
                                     * re-enter (prevents infinite OM loops).
                                     * was_in_handler snapshot distinguishes
                                     * this from the ERROR statement, which
                                     * legitimately enters the handler itself. */
                                    if (was_in_handler && gwdata_is_in_error_handler()) {
                                        printf("Error at line %d\n", line_num);
                                        exec_rc = ERROR;
                                        break;
                                    }
                                    /* Error occurred - check if ON ERROR GOTO is active */
                                    if (gwdata_is_on_error_active()) {
                                        int error_code = gwdata_get_error();
                                        int handler_line = gwdata_get_on_error_line();
                                        if (handler_line > 0) {
                                            /* Store error code in ERR variable for the handler */
                                            struct variable *err_var = gwdata_alloc_variable("ERR", VT_INTEGER);
                                            if (err_var) {
                                                err_var->value.i = error_code;
                                            }
                                            /* Clear internal error state so main loop doesn't keep re-entering handler.
                                             * ERR variable persists until next error (GW-BASIC behavior). */
                                            gwdata_clear_error();
                                            gwdata_enter_error_handler();
                                            curr = gwlist_find_line((WORD)handler_line);
                                            continue;
                                        }
                                    }
                                    /* No ON ERROR handler or handler not found */
                                    printf("Error at line %d\n", line_num);
                                    exec_rc = ERROR;
                                    break;
                                }
                
                                if (rc < 0) {
                                    /* Check if it's END statement (-2 from gwsts_exec_statement) */
                                    if (rc == -2) {
                                        /* END - stop execution gracefully */
                                        break;
                                    }
                                    /* Error occurred (from statement execution).
                                     * Liveness by pointer identity (RENUM-safe). */
                                    if (gwlist_is_alive(curr)) {
                                        if (gwdata_handle_error(line_num)) {
                                            curr = gwlist_find_line(gwdata_get_on_error_line());
                                            continue;
                                        }
                                    } else {
                                        /* curr was deleted — no ON ERROR line context */
                                        exec_rc = ERROR;
                                        break;
                                    }
                                    /* No ON ERROR handler or handler not found */
                                    exec_rc = ERROR;
                                    break;
                                } else if (rc > 0) {
                                    /* FOR/GOTO/GOSUB/RETURN/ON ERROR continuation - rc is the line number to jump to */
                                    curr = gwlist_find_line((WORD)rc);
                                    continue;
                                }
                                /* rc == 0 is success, continue to next line.
                                 * CRITICAL-1 fix (same as RUN path):
                                 * - Branch 1 (current line survives): curr->next
                                 *   is a LIVE pointer (delete_line already
                                 *   updated the chain) — use it.
                                 * - Branch 2/3 (current line freed): walk the
                                 *   live list for first surviving line >
                                 *   line_num (pointer-identity walk, never read
                                 *   freed node fields). */
                                /* LOW-2 (v127): liveness walk ONLY when the
                                 * statement mutated the chain. Same semantics
                                 * as the RUN path. */
                                if (link_version != gwlist_get_link_version()) {
                                    /* Chain mutated — liveness by POINTER
                                     * IDENTITY (not number lookup; RENUM
                                     * renumbers the current line while it stays
                                     * alive): both alive → physical next line
                                     * (ASM SAVTXT); else first surviving line >
                                     * line_num. */
                                    if (gwlist_is_alive(curr) && gwlist_is_alive(saved_next)) {
                                        curr = saved_next;
                                    } else {
                                        curr = gwlist_get_first_line();
                                        while (curr && curr->number <= line_num) curr = curr->next;
                                    }
                                    link_version = gwlist_get_link_version();
                                } else {
                                    /* Chain unchanged — both valid live pointers. */
                                    curr = saved_next;
                                }
            } else {
                curr = curr->next;
            }
        }
        
        /* Cleanup */
        free(g_state);
        g_state = NULL;
        return exec_rc;
    }

    /* Run the interpreter (interactive mode) */
    run_interpreter();

    /* Cleanup */
    free(g_state);
    g_state = NULL;

    return SUCCESS;
}
