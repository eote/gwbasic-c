/* GWMAIN - Main GW-BASIC Entry Point */
/* Based on GWMAIN.ASM */

#include "gwbasic.h"
#include <stdio.h>
#include <stdlib.h>

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

/* Main entry point */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    /* Initialize all modules (gwdata_init allocates g_state) */
    init_all_modules();
    
    if (!g_state) {
        return ERROR;
    }
    
    /* TODO: Main interpreter loop */
    printf("GW-BASIC C Port - Stub Implementation\n");
    printf("Based on Microsoft GW-BASIC 1983 Source Code\n\n");
    printf("This is a skeleton - interpreter not yet implemented.\n");
    
    /* Cleanup */
    free(g_state);
    g_state = NULL;
    
    return SUCCESS;
}