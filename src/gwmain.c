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
    advgrp_init();
    biboot_init();
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
    gwdata_init();
    gweval_init();
    gwinit_init();
    gwlist_init();
    gwram_init();
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

/* Check if string is a direct command (no line number) */
static bool is_direct_command(const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line) return false;
    return !isdigit((unsigned char)*line);
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
                    /* Execute all program lines in order */
                    struct program_line *curr = gwlist_get_first_line();
                    while (curr) {
                        if (curr->text) {
                            int rc = gwsts_exec_statement(curr->text);
                            if (rc != 0) {
                                printf("Error at line %d\n", curr->number);
                                break;
                            }
                        }
                        curr = curr->next;
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
    
    /* Run the interpreter */
    run_interpreter();
    
    /* Cleanup */
    free(g_state);
    g_state = NULL;
    
    return SUCCESS;
}