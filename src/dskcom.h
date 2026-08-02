/* DSKCOM - Disk Communication / File I/O Header */
/* Based on DSKCOM.ASM */

#ifndef DSKCOM_H
#define DSKCOM_H

#include "gwbasic.h"

/* File modes */
#define FM_INPUT  1
#define FM_OUTPUT 2
#define FM_RANDOM 3
#define FM_APPEND 4
#define FM_BINARY 5

/* File descriptor structure - opaque handle for cross-platform compatibility */
/* Windows stores HANDLE, POSIX would store int fd. This preserves the interface. */
struct file_desc {
    int filenum;              /* GW-BASIC file number (1-255) */
    int mode;                 /* FM_INPUT, FM_OUTPUT, etc. */
    void *handle;             /* Opaque file handle (HANDLE on Windows, int fd on POSIX) */
    char *filename;           /* Full path */
    int record_length;        /* For random files */
    struct file_desc *next;   /* Linked list */
};

/* DSKCOM module functions */
void dskcom_init(void);
int dskcom_open(const char *filename, int filenum, int mode, int reclen);
int dskcom_close(int filenum);  /* filenum=0 means close all */
int dskcom_close_all(void);  /* close all open files */
struct file_desc *dskcom_find_file(int filenum);
int dskcom_print(int filenum, const char *data);
int dskcom_write(int filenum, const void *data, int len);
int dskcom_input(int filenum, char *buffer, int maxlen);
int dskcom_line_input(int filenum, char *buffer, int maxlen);
int dskcom_seek(int filenum, long record);
int dskcom_get(int filenum, long record, void *buffer, int buflen);
int dskcom_put(int filenum, long record, const void *buffer, int buflen);

#endif /* DSKCOM_H */
