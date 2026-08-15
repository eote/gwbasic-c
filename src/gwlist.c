/* GWLIST - Program Storage & Listing */
/* Based on GWLIST.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Program line list head */
static struct program_line *g_program_head = NULL;

void gwlist_init(void) {
    g_program_head = NULL;
}

void gwlist_add_line(int number, const char *text) {
    if (!text) return;
    
    /* Check if line already exists */
    struct program_line *prev = NULL;
    struct program_line *curr = g_program_head;
    
    while (curr && curr->number < number) {
        prev = curr;
        curr = curr->next;
    }
    
    if (curr && curr->number == number) {
        /* Replace existing line */
        gwram_free(curr->text);
        curr->text = (char *)gwram_alloc(strlen(text) + 1);
        if (curr->text) strcpy(curr->text, text);
        return;
    }
    
    /* Insert new line */
    struct program_line *new_line = (struct program_line *)gwram_alloc(sizeof(struct program_line));
    if (!new_line) return;
    
    new_line->number = number;
    new_line->text = (char *)gwram_alloc(strlen(text) + 1);
    if (new_line->text) strcpy(new_line->text, text);
    new_line->next = curr;
    
    if (prev) {
        prev->next = new_line;
    } else {
        g_program_head = new_line;
    }
}

struct program_line *gwlist_find_line(int number) {
    struct program_line *curr = g_program_head;
    while (curr) {
        if (curr->number == number) return curr;
        curr = curr->next;
    }
    return NULL;
}

void gwlist_delete_line(int number) {
    struct program_line *prev = NULL;
    struct program_line *curr = g_program_head;
    
    while (curr) {
        if (curr->number == number) {
            if (prev) {
                prev->next = curr->next;
            } else {
                g_program_head = curr->next;
            }
            gwram_free(curr->text);
            gwram_free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void gwlist_list(int start, int end) {
    struct program_line *curr = g_program_head;
    
    while (curr) {
        if ((start == 0 || curr->number >= start) && (end == 0 || curr->number <= end)) {
            printf("%d %s\n", curr->number, curr->text ? curr->text : "");
        }
        if (end != 0 && curr->number > end) break;
        curr = curr->next;
    }
}

struct program_line *gwlist_get_first_line(void) {
    return g_program_head;
}

/* Find line by text pointer (for FOR/NEXT loop continuation) */
struct program_line *gwlist_find_line_by_text_ptr(WORD text_ptr) {
    struct program_line *curr = g_program_head;
    while (curr) {
        if (curr->text && (WORD)(uintptr_t)curr->text == text_ptr) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Clear all program lines - free linked list nodes */
void gwlist_clear(void) {
    struct program_line *curr = g_program_head;
    while (curr) {
        struct program_line *next = curr->next;
        if (curr->text) {
            gwram_free(curr->text);
        }
        gwram_free(curr);
        curr = next;
    }
    g_program_head = NULL;
}

/* Windows-compatible getline implementation (forward declaration) */
static ssize_t getline_windows(char **lineptr, size_t *n, FILE *stream);

/* Load program from .bas file */
int gwlist_load_file(const char *filename) {
    if (!filename) return ERROR;

    DEBUG_PRINT("Opening file: %s", filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        DEBUG_PRINT("Failed to open file: %s", filename);
        return ERROR;
    }

    DEBUG_PRINT("File opened successfully");
    /* Clear existing program */
    gwlist_clear();

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len;
    int line_count = 0;
    
    /* Custom getline implementation for Windows compatibility */
    while ((line_len = getline_windows(&line, &line_cap, fp)) != -1) {
        /* Remove trailing newline */
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[--line_len] = '\0';
        }
        if (line_len > 0 && line[line_len - 1] == '\r') {
            line[--line_len] = '\0';
        }

        /* Skip empty lines and comments */
        char *trimmed = line;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        if (!*trimmed || *trimmed == '\'') continue;

        /* Extract line number using strtol with overflow check */
        char *endptr = NULL;
        errno = 0;
        long num = strtol(trimmed, &endptr, 10);
        
        if (errno == ERANGE || num > 65529 || num <= 0) {
            if (errno == ERANGE) {
                DEBUG_PRINT("Line number overflow: %s", trimmed);
            }
            free(line);
            fclose(fp);
            return ERROR;
        }
        
        /* Advance past the line number */
        trimmed = endptr;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        
        if (*trimmed) {
            DEBUG_PRINT("Adding line %ld: '%s'", num, trimmed);
            gwlist_add_line((int)num, trimmed);
            line_count++;
        }
    }

    free(line);
    
    DEBUG_PRINT("Loaded %d lines", line_count);
    
    if (line_count == 0) {
        DEBUG_PRINT("Warning: Empty program file");
    }

    fclose(fp);
    return SUCCESS;
}

/* Windows-compatible getline implementation */
static ssize_t getline_windows(char **lineptr, size_t *n, FILE *stream) {
    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }

    char *buf = *lineptr;
    size_t buf_size = *n;
    size_t len = 0;
    int c;

    if (buf == NULL || buf_size == 0) {
        buf_size = 128;
        buf = (char *)gwram_alloc(buf_size);
        if (!buf) {
            return -1;
        }
        *lineptr = buf;
        *n = buf_size;
    }

    while ((c = fgetc(stream)) != EOF) {
        if (len + 1 >= buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)gwram_realloc(buf, buf_size);
            if (!new_buf) {
                return -1;
            }
            buf = new_buf;
            *lineptr = buf;
            *n = buf_size;
        }

        buf[len++] = (char)c;
        if (c == '\n') {
            break;
        }
    }

    if (len == 0 && c == EOF) {
        return -1;
    }

    buf[len] = '\0';
    return len;
}