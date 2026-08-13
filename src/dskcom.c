/* DSKCOM - Disk Communication / File I/O */
/* Based on DSKCOM.ASM */

#include <windows.h>
#include "dskcom.h"
#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* File descriptor list */
static struct file_desc *g_file_list = NULL;

/* Module initialization */
void dskcom_init(void) {
    g_file_list = NULL;
}

/* Find file descriptor by file number */
struct file_desc *dskcom_find_file(int filenum) {
    struct file_desc *fd = g_file_list;
    while (fd) {
        if (fd->filenum == filenum) return fd;
        fd = fd->next;
    }
    return NULL;
}

/* OPEN statement: OPEN "filename" FOR mode AS #filenum [LEN=reclen] */
int dskcom_open(const char *filename, int filenum, int mode, int reclen) {
    if (!filename || filenum < 1 || filenum > 255) return ERROR;

    /* Check if already open */
    if (dskcom_find_file(filenum)) return ERROR;

    DWORD desired_access = 0;
    DWORD creation_disposition = 0;
    DWORD flags = FILE_ATTRIBUTE_NORMAL;

    switch (mode) {
        case FM_INPUT:
            desired_access = GENERIC_READ;
            creation_disposition = OPEN_EXISTING;
            break;
        case FM_OUTPUT:
            desired_access = GENERIC_WRITE;
            creation_disposition = CREATE_ALWAYS;
            break;
        case FM_APPEND:
            desired_access = GENERIC_WRITE | FILE_APPEND_DATA;
            creation_disposition = OPEN_ALWAYS;
            break;
        case FM_RANDOM:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            creation_disposition = OPEN_ALWAYS;
            break;
        case FM_BINARY:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            creation_disposition = OPEN_ALWAYS;
            break;
        default:
            return ERROR;
    }

    /* Convert filename to wide char for Windows API */
    int wlen = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    if (wlen == 0) return ERROR;

    wchar_t *wfilename = (wchar_t *)gwram_alloc(wlen * sizeof(wchar_t));
    if (!wfilename) return ERROR;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename, wlen);

    HANDLE handle = CreateFileW(wfilename, desired_access, FILE_SHARE_READ,
                                NULL, creation_disposition, flags, NULL);
    gwram_free(wfilename);

    if (handle == INVALID_HANDLE_VALUE) return ERROR;

    /* For APPEND mode, seek to end */
    if (mode == FM_APPEND) {
        SetFilePointer(handle, 0, NULL, FILE_END);
    }

    /* Create file descriptor */
    struct file_desc *fd = (struct file_desc *)gwram_alloc(sizeof(struct file_desc));
    if (!fd) {
        CloseHandle(handle);
        return ERROR;
    }

    fd->filenum = filenum;
    fd->mode = mode;
    fd->handle = handle;
    fd->record_length = (reclen > 0) ? reclen : 128;  /* Default record length */

    size_t fn_len = strlen(filename) + 1;
    fd->filename = (char *)gwram_alloc(fn_len);
    if (fd->filename) {
        memcpy(fd->filename, filename, fn_len);
    }

    /* Add to list */
    fd->next = g_file_list;
    g_file_list = fd;

    return SUCCESS;
}

/* CLOSE statement: CLOSE [#filenum] */
int dskcom_close(int filenum) {
    struct file_desc *fd = g_file_list;
    struct file_desc *prev = NULL;

    while (fd) {
        if (fd->filenum == filenum) {
            CloseHandle((HANDLE)fd->handle);
            if (fd->filename) gwram_free(fd->filename);

            if (prev) prev->next = fd->next;
            else g_file_list = fd->next;

            gwram_free(fd);
            return SUCCESS;
        }
        prev = fd;
        fd = fd->next;
    }
    return ERROR;
}

/* CLOSE all open files */
int dskcom_close_all(void) {
    struct file_desc *fd = g_file_list;
    while (fd) {
        struct file_desc *next = fd->next;
        CloseHandle((HANDLE)fd->handle);
        if (fd->filename) gwram_free(fd->filename);
        gwram_free(fd);
        fd = next;
    }
    g_file_list = NULL;
    return SUCCESS;
}

/* PRINT# statement: PRINT #filenum, data */
int dskcom_print(int filenum, const char *data) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_OUTPUT && fd->mode != FM_APPEND) return ERROR;

    /* Write data followed by newline (GW-BASIC behavior) */
    DWORD written;
    WriteFile((HANDLE)fd->handle, data, (DWORD)strlen(data), &written, NULL);
    if (written != strlen(data)) return ERROR;

    /* Add newline - use \n for text modes (OS converts to CRLF), \r\n for binary */
    const char *nl = (fd->mode == FM_BINARY) ? "\r\n" : "\n";
    WriteFile((HANDLE)fd->handle, nl, (DWORD)strlen(nl), &written, NULL);

    return SUCCESS;
}

/* WRITE# statement: WRITE #filenum, data */
int dskcom_write(int filenum, const void *data, int len) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_OUTPUT && fd->mode != FM_APPEND &&
        fd->mode != FM_RANDOM && fd->mode != FM_BINARY) return ERROR;

    DWORD written;
    WriteFile((HANDLE)fd->handle, data, (DWORD)len, &written, NULL);
    return (written == (DWORD)len) ? SUCCESS : ERROR;
}

/* INPUT# statement: INPUT #filenum, var1, var2, ... */
int dskcom_input(int filenum, char *buffer, int maxlen) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_INPUT && fd->mode != FM_RANDOM) return ERROR;

    /* Read until comma or newline */
    char ch;
    int pos = 0;
    DWORD read;
    bool in_quotes = false;

    while (pos < maxlen - 1 && ReadFile((HANDLE)fd->handle, &ch, 1, &read, NULL) && read == 1) {
        if (ch == '\"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (!in_quotes && (ch == ',' || ch == '\n' || ch == '\r')) {
            if (ch == '\r') {
                /* Peek next char for \n */
                char next;
                if (ReadFile((HANDLE)fd->handle, &next, 1, &read, NULL) && read == 1 && next != '\n') {
                    SetFilePointer((HANDLE)fd->handle, -1, NULL, FILE_CURRENT);
                }
            }
            break;
        }
        buffer[pos++] = ch;
    }
    buffer[pos] = '\0';

    /* Trim whitespace */
    while (pos > 0 && (buffer[pos-1] == ' ' || buffer[pos-1] == '\t')) {
        buffer[--pos] = '\0';
    }

    return (pos > 0) ? SUCCESS : ERROR;
}

/* LINE INPUT# statement: LINE INPUT #filenum, var$ */
int dskcom_line_input(int filenum, char *buffer, int maxlen) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_INPUT && fd->mode != FM_RANDOM) return ERROR;

    /* Read entire line up to newline */
    char ch;
    int pos = 0;
    DWORD read;

    while (pos < maxlen - 1 && ReadFile((HANDLE)fd->handle, &ch, 1, &read, NULL) && read == 1) {
        if (ch == '\n') break;
        if (ch == '\r') continue;  /* Skip CR */
        buffer[pos++] = ch;
    }
    buffer[pos] = '\0';

    /* Return ERROR if EOF reached immediately (empty file) */
    if (pos == 0 && read == 0) {
        return ERROR;
    }

    return SUCCESS;
}

/* SEEK statement: SEEK #filenum, record */
int dskcom_seek(int filenum, long record) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_RANDOM) return ERROR;

    /* Validate record number (must be >= 1) */
    if (record < 1) return ERROR;

    long offset = (record - 1) * fd->record_length;
    DWORD result = SetFilePointer((HANDLE)fd->handle, offset, NULL, FILE_BEGIN);
    return (result != INVALID_SET_FILE_POINTER) ? SUCCESS : ERROR;
}

/* GET statement: GET #filenum, [record], variable */
int dskcom_get(int filenum, long record, void *buffer, int buflen) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_RANDOM && fd->mode != FM_BINARY) return ERROR;

    /* Seek to record if specified */
    if (record > 0) {
        int rc = dskcom_seek(filenum, record);
        if (rc != SUCCESS) return ERROR;
    }

    /* Read min(buflen, record_length) bytes */
    int read_len = (buflen < fd->record_length) ? buflen : fd->record_length;
    DWORD read;
    if (!ReadFile((HANDLE)fd->handle, buffer, read_len, &read, NULL)) return ERROR;

    /* If we read less than requested (EOF), zero-pad the rest */
    if (read < (DWORD)read_len) {
        memset((char *)buffer + read, 0, read_len - read);
    }

    /* If user buffer is smaller than record_length, skip remaining bytes to align to next record */
    if (buflen < fd->record_length) {
        SetFilePointer((HANDLE)fd->handle, fd->record_length - read_len, NULL, FILE_CURRENT);
    }

    return SUCCESS;
}

/* PUT statement: PUT #filenum, [record], variable */
int dskcom_put(int filenum, long record, const void *buffer, int buflen) {
    struct file_desc *fd = dskcom_find_file(filenum);
    if (!fd) return ERROR;
    if (fd->mode != FM_RANDOM && fd->mode != FM_BINARY) return ERROR;

    /* Seek to record if specified */
    if (record > 0) {
        int rc = dskcom_seek(filenum, record);
        if (rc != SUCCESS) return ERROR;
    }

    /* Write record_length bytes (pad with zeros if buffer smaller) */
    DWORD written;

    /* Write user data */
    if (!WriteFile((HANDLE)fd->handle, buffer, buflen, &written, NULL)) return ERROR;

    /* If user data is shorter than record_length, pad with zeros in batch */
    if (buflen < fd->record_length) {
        int pad_len = fd->record_length - buflen;
        char *zeros = (char *)calloc(1, pad_len);
        if (!zeros) return ERROR;

        if (!WriteFile((HANDLE)fd->handle, zeros, pad_len, &written, NULL)) {
            free(zeros);
            return ERROR;
        }
        free(zeros);
    }

    return SUCCESS;
}

/* Find a free file number (1-255) not currently in use */
int dskcom_find_free_filenum(void) {
    /* Scan from 1 to 255 for a free file number */
    for (int filenum = 1; filenum <= 255; filenum++) {
        if (!dskcom_find_file(filenum)) {
            return filenum;
        }
    }
    return 0;  /* All file numbers in use */
}
