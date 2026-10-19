/* ITSA86 - Resident Initialization for 8086 */
/* Based on ITSA86.ASM */

#include <windows.h>
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* External references */
extern void gwdata_init(void);
extern void gwram_init(void);
extern void oem_init(void);
extern void oemcbk_init(void);
extern void oemev_init(void);
extern void oemsnd_init(void);
extern void scndrv_init(void);
extern void gengrp_init(void);
extern void giokyb_init(void);
extern void gioscn_init(void);
extern void giolpt_init(void);
extern void giocom_init(void);
extern void giodsk_init(void);
extern void scndrv_init(void);
extern void call86_init(void);
extern void ibmres_init(void);
extern void oemev_init(void);
extern void oemsnd_init(void);
extern void scnedt_init(void);
extern void advgrp_init(void);

/* Memory map variables */
static WORD g_cswflg = 0;      /* /C: option flag */
static WORD g_cswsiz = 0;      /* /C: option size */
static WORD g_mswflg = 0;      /* /M: option flag */
static WORD g_mswsiz = 0;      /* /M: option size / MAXMEM */
static WORD g_newds = 0;       /* Final DS segment */
static WORD g_maxmem = 0;      /* MAXMEM */
static WORD g_txttab = 0;      /* TXTTAB pointer */
static WORD g_memsiz = 0;      /* MEMSIZ */
static WORD g_topmem = 0;      /* TOPMEM */
static WORD g_stklow = 0;      /* STKLOW */
static WORD g_savseg = 0;      /* SAVSEG */
static WORD g_maxmem_seg = 0;  /* MAXMEM segment */
static WORD g_topmem_seg = 0;  /* TOPMEM segment */
static WORD g_stklow_seg = 0;  /* STKLOW segment */
static WORD g_filtab = 0;      /* FILTAB */
static WORD g_freflg = 0;      /* FREFLG - bytes free message flag */
static WORD g_cpmmem = 0;      /* CPMMEM */
static WORD g_stklow_val = 0;  /* STKLOW value */
static WORD g_savseg_val = 0;  /* SAVSEG value */

/* Segment base for 8086 memory model */
static WORD g_data_seg = 0;
static WORD g_code_seg = 0;

/* Memory map constants */
#define MAX_PARAGRAPHS (65535 / 16)
#define PARAGRAPH_SIZE 16

/* BASVAR table - maps index to variable addresses */
static WORD *g_bvtab[4] = {0};

/* Module initialization */
void itsa86_init(void) {
    /* Initialize data segment base.
     * v141: |= 0x1000 guarantees a non-zero segment (real .exe load
     * segments are never 0); (addr & 0xF000) alone can be 0 depending on
     * code layout, making mapclc's NEWDS flaky. */
    g_data_seg = (WORD)((uintptr_t)&g_data_seg & 0xF000) | 0x1000;
    g_code_seg = (WORD)((uintptr_t)itsa86_init & 0xF000) | 0x1000;
    
    /* Initialize memory map variables */
    g_cswflg = 0;
    g_cswsiz = 0;
    g_mswflg = 0;
    g_mswsiz = 0;
    g_newds = 0;
    g_maxmem = 0;
    g_txttab = 0;
    g_memsiz = 0;
    g_topmem = 0;
    g_stklow = 0;
    g_savseg = 0;
    g_maxmem_seg = 0;
    g_topmem_seg = 0;
    g_stklow_seg = 0;
    g_filtab = 0;
    g_memsiz = 0;
    g_freflg = 0;
    g_cpmmem = 0;
    g_stklow_val = 0;
    g_savseg_val = 0;
    
    /* Initialize BASVAR table - will be set up during GW-DATA init */
    /* These will be filled in by gwdata_init */
    g_bvtab[0] = NULL;
    g_bvtab[1] = NULL;
    g_bvtab[2] = NULL;
    g_bvtab[3] = NULL;
}

/* BASVAR - Retrieve or modify BASIC internal data locations
 * Entry: 
 *   bx = index (0=current line, 1=error line, 2=program text addr, 3=variable table addr)
 *   carry set = write, carry clear = read
 *   ax = value to write (if carry set)
 * Returns:
 *   ax = value of variable (0 on error)
 *   carry set = error, carry clear = success
 *   bx modified */
WORD itsa86_basvar(WORD bx, WORD ax, int carry) {
    /* Use global BVTAB instead of local static */
    if (!g_bvtab[0]) {
        /* Not initialized - return error */
        return 0xFFFF;  /* Error code for uninitialized */
    }
    
    if (bx >= 4) {
        return 0xFFFE;  /* Error code for invalid index */
    }
    
    WORD *addr = g_bvtab[bx];
    if (!addr) {
        return 0xFFFD;  /* Error code for null pointer */
    }
    
    if (carry) {
        /* Write */
        *addr = ax;
    }
    
    return *addr;
}

/* MAPINI - Initialize the final memory map */
void itsa86_mapini(void) {
    /* This function sets up the final memory map after loading */
    /* In the C version, this is handled by the OS and memory manager */
    
    /* Ensure zeros at TXTTAB */
    /* In C, memory is zero-initialized */
    
    /* Calculate final memory map */
    itsa86_mapclc();
    
    /* Call SEGINI equivalent */
    /* oem_segin(); - handled by OEM module */
    
    /* Save segment for PEEK/POKE */
    g_savseg = g_data_seg;
}

/* MAPCLC - Calculate final memory map limits */
void itsa86_mapclc(void) {
    WORD dx, cx, bx;
    (void)dx;
    (void)cx;
    (void)bx;

    /* Validate/get COM buffer size */
    /* DX = COM buffer size (from OEM) */
    WORD dx_val = 0; /* Would come from OEM SETCBF */
    (void)dx_val;
    
    /* Calculate NEWDS (New DS segment) */
    /* NEWDS = (D + 15/16) + CS: */
    dx = 0; /* COM buffer size in paragraphs */
    dx = (dx + 15) / 16; /* Round up to paragraph */
    
    /* NEWDS = CS + DX */
    WORD cs = g_code_seg;
    WORD newds = cs + dx;
    g_newds = newds;
    
    /* Validate /M option or calculate maximum MAXMEM */
    /* Maximum usable paragraphs */
    WORD max_paragraphs = MAX_PARAGRAPHS;
    
    /* Available paragraphs after COM buffer */
    WORD avail = MAX_PARAGRAPHS - dx;
    
    if (g_mswflg) {
        /* /M: option specified */
        WORD requested = g_mswsiz;
        if (requested > max_paragraphs * 16) {
            /* Not enough memory - set error flag and use max available */
            g_maxmem = avail * 16;
            g_maxmem_seg = dx + avail;
        } else {
            if (requested > avail * 16) {
                requested = avail * 16;
            }
            g_maxmem = requested;
            g_maxmem_seg = dx + (requested + 15) / 16;
        }
    } else {
        /* Use maximum available */
        g_maxmem = avail * 16;
        g_maxmem_seg = dx + avail;
    }
    
    g_mswsiz = g_maxmem;
    
    /* Calculate segment values */
    g_topmem = g_maxmem;
    g_memsiz = g_maxmem;
}

/* SEGOFF - Convert segment to offset from current DS
 * Entry: bx = segment
 * Exit: bx = offset from current DS in bytes
 * Returns 0 on success, -1 on error */
int itsa86_seg_off(WORD *bx) {
    WORD current_ds = g_data_seg;
    
    if (*bx < current_ds) {
        return -1; /* Error: segment below current DS */
    }
    
    WORD paragraphs = *bx - current_ds;
    
    /* Limit to 64K */
    if (paragraphs > 0x7777) {
        paragraphs = 0x7777;
    }
    
    /* Convert to bytes */
    *bx = paragraphs * 16;
    
    return 0; /* Success */
}

/* Get memory map info */
void itsa86_get_memory_info(WORD *maxmem, WORD *freemem, WORD *txttab, WORD *vartab) {
    if (maxmem) *maxmem = g_maxmem;
    if (freemem) *freemem = g_maxmem - g_txttab;
    if (txttab) *txttab = g_txttab;
    if (vartab) *vartab = 0; /* Would come from GW-DATA */
}

/* Set command line options */
void itsa86_set_c_option(WORD size) {
    g_cswflg = 1;
    g_cswsiz = size;
}

void itsa86_set_m_option(WORD size) {
    g_mswflg = 1;
    g_mswsiz = size;
}

/* Accessor functions for testing */
WORD itsa86_get_maxmem(void) { return g_maxmem; }
WORD itsa86_get_newds(void) { return g_newds; }
WORD itsa86_get_data_seg(void) { return g_data_seg; }
WORD itsa86_get_savseg(void) { return g_savseg; }
WORD itsa86_get_txttab(void) { return g_txttab; }
WORD** itsa86_get_bvtab(void) { return g_bvtab; }

/* Get memory map for debugging */
void itsa86_dump_memory_map(void) {
    /* Debug function */
}

/* End marker - code after this is discarded after MAPINI in original */
void itsa86_csend(void) {
    /* End of resident code marker */
}