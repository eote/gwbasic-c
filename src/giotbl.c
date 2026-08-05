/* GIOTBL - Device Name Table, Dispatch Table Address Tables */
/* Based on GIOTBL.ASM */

#include "gwbasic.h"
#include <string.h>

/* Device name table */
static const device_name_t g_device_names[] = {
    {"KYBD", 0},   /* Keyboard */
    {"SCRN", 1},   /* Screen/CRT */
    {"CONS", 2},   /* Raw-I/O CRT (Esc Sequences) */
    {"LPT1", 3},   /* Line printer 1 */
    {"LPT2", 4},   /* Line printer 2 */
    {"LPT3", 5},   /* Line printer 3 */
    {"CAS1", 6},   /* Cassette 1 */
    {"COM1", 7},   /* COM channel 1 */
    {"COM2", 8},   /* COM channel 2 */
    {"COM3", 9},   /* COM channel 3 */
    {"COM4", 10},  /* COM channel 4 */
    {NULL, -1}     /* End of table */
};

/* Device dispatch table - using only functions that exist */
static const device_dispatch_t g_device_dispatch[] = {
    {"KYBD", NULL, NULL, NULL},   /* Keyboard */
    {"SCRN", NULL, NULL, NULL},   /* Screen/CRT */
    {"CONS", NULL, NULL, NULL},   /* Raw-I/O CRT */
    {"LPT1", NULL, NULL, NULL},   /* Line printer 1 */
    {"LPT2", NULL, NULL, NULL},   /* Line printer 2 */
    {"LPT3", NULL, NULL, NULL},   /* Line printer 3 */
    {"CAS1", NULL, NULL, NULL},   /* Cassette 1 */
    {"COM1", NULL, NULL, NULL},   /* COM channel 1 */
    {"COM2", NULL, NULL, NULL},   /* COM channel 2 */
    {"COM3", NULL, NULL, NULL},   /* COM channel 3 */
    {"COM4", NULL, NULL, NULL},   /* COM channel 4 */
    {NULL, NULL, NULL, NULL}      /* End of table */
};

/* Number of non-disk devices */
#define G_NDEV 11

/* Module initialization */
void giotbl_init(void) {
    /* Initialize device tables - nothing to do for stub implementation */
}

/* Module termination */
void giotbl_term(void) {
    /* Nothing to clean up */
}

/* Get device ID by name */
int giotbl_get_device_id(const char *name) {
    if (!name) return -1;
    
    for (int i = 0; i < G_NDEV; i++) {
        if (g_device_names[i].name && strcmp(g_device_names[i].name, name) == 0) {
            return g_device_names[i].device_id;
        }
    }
    return -1;  /* Device not found */
}

/* Get device name by ID */
const char* giotbl_get_device_name(int device_id) {
    if (device_id < 0 || device_id >= G_NDEV) return NULL;
    return g_device_names[device_id].name;
}

/* Get dispatch table entry by device ID */
int giotbl_get_dispatch(int device_id, void **dsp, void **ini, void **trm) {
    if (device_id < 0 || device_id >= G_NDEV) return -1;
    
    if (dsp) *dsp = g_device_dispatch[device_id].dsp_addr;
    if (ini) *ini = g_device_dispatch[device_id].ini_addr;
    if (trm) *trm = g_device_dispatch[device_id].trm_addr;
    return 0;
}

/* Get number of devices */
int giotbl_get_ndev(void) {
    return G_NDEV;
}

/* Lookup device by name */
int giotbl_lookup_device(const char *name, int *device_id) {
    int id = giotbl_get_device_id(name);
    if (id >= 0) {
        if (device_id) *device_id = id;
        return 0;
    }
    return -1;
}

/* Get device name table */
const device_name_t* giotbl_get_name_table(void) {
    return g_device_names;
}

/* Get device dispatch table */
const device_dispatch_t* giotbl_get_dispatch_table(void) {
    return g_device_dispatch;
}

/* Find device in name table */
int giotbl_find_device(const char *name) {
    return giotbl_get_device_id(name);
}

/* Initialize all device tables */
void giotbl_init_all(void) {
    giotbl_init();
}