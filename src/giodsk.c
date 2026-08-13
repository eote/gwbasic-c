/* GIODSK - Generalized I/O Disk Driver */
/* Based on GIODSK.ASM */
/* This module initializes the disk I/O subsystem (merged with DSKCOM) */

#include "gwbasic.h"
#include "dskcom.h"

/* Module initialization - initialize disk I/O subsystem */
void giodsk_init(void) {
    /* Initialize the disk communication subsystem */
    dskcom_init();
}

/* GIODSK module functions - Disk I/O entry points */
/* These are thin wrappers around DSKCOM functions for compatibility */

int giodsk_open(const char *filename, int filenum, int mode, int reclen) {
    return dskcom_open(filename, filenum, mode, reclen);
}

int giodsk_close(int filenum) {
    return dskcom_close(filenum);
}

int giodsk_close_all(void) {
    return dskcom_close_all();
}

int giodsk_print(int filenum, const char *data) {
    return dskcom_print(filenum, data);
}

int giodsk_write(int filenum, const void *data, int len) {
    return dskcom_write(filenum, data, len);
}

int giodsk_input(int filenum, char *buffer, int maxlen) {
    return dskcom_input(filenum, buffer, maxlen);
}

int giodsk_line_input(int filenum, char *buffer, int maxlen) {
    return dskcom_line_input(filenum, buffer, maxlen);
}

int giodsk_seek(int filenum, long record) {
    return dskcom_seek(filenum, record);
}

int giodsk_get(int filenum, long record, void *buffer, int buflen) {
    return dskcom_get(filenum, record, buffer, buflen);
}

int giodsk_put(int filenum, long record, const void *buffer, int buflen) {
    return dskcom_put(filenum, record, buffer, buflen);
}

int giodsk_find_free_filenum(void) {
    return dskcom_find_free_filenum();
}