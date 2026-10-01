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

/* RESEQ [NN[,MM[,INC]]] — resequence line numbers (ASM GWMAIN.ASM RESEQ L3102-3222).
 * NN = first destination line (0 = default 10); MM = threshold — lines < MM are
 * NOT resequenced (0 = all); INC = increment.
 * INC semantics: -1 = absent (default 10); 0 = explicit zero → FCERR (ASM L3140-3142);
 * >0 = use as-is. New numbers > 65529 → FCERR (ASM L3176-3181). */
int gwlist_reseq(int first, int mm, int inc) {
    if (!g_state && !g_program_head) return SUCCESS;

    /* Defaults (ASM L3105-3111: CX=10 assumed INC; DE=0 = all lines) */
    int start = (first > 0) ? first : 10;
    int threshold = mm;          /* 0 = reseq all */
    int increment;
    if (inc == 0) {
        return ERROR;  /* ASM L3140-3142: explicit INC=0 → FCERR */
    } else if (inc < 0) {
        increment = 10;  /* Absent INC — default */
    } else {
        increment = inc;
    }

    /* Snapshot the qualifying nodes first (in sorted order), then renumber and
     * re-position each. Prevents re-encountering already-processed nodes that
     * repositioning moves behind curr (ASM RESNX1 processes each line once). */
    struct program_line *nodes[256];
    int count = 0;
    struct program_line *curr = g_program_head;
    while (curr && count < 256) {
        if (threshold == 0 || curr->number >= threshold) {
            nodes[count++] = curr;
        }
        curr = curr->next;
    }

    int new_num = start;
    for (int i = 0; i < count; i++) {
        /* ASM NXTRSC L3176-3181: new line > 65529 → FCERR */
        if (new_num > 65529) {
            return ERROR;
        }
        struct program_line *node = nodes[i];
        node->number = new_num;
        new_num += increment;

        /* Re-position node in the sorted list (ASM RESNX1 L3236: MOV [BX],DX) */
        struct program_line *prev = g_program_head;
        if (prev == node) {
            /* Already at head — check if next breaks order */
            if (!node->next || node->number <= node->next->number) continue;
            g_program_head = node->next;
        } else {
            while (prev && prev->next != node) prev = prev->next;
            if (!prev) continue;  /* Should not happen */
            if (!node->next || node->number <= node->next->number) continue;
            prev->next = node->next;
        }
        /* Find insertion point for node->number */
        struct program_line *p = g_program_head;
        struct program_line *pp = NULL;
        while (p && p->number < node->number) {
            pp = p;
            p = p->next;
        }
        node->next = p;
        if (pp) pp->next = node;
        else g_program_head = node;
    }
    return SUCCESS;
}

/* DELETE range — delete lines in [start, end] (ASM DELETE L778-812 + SCNLIN L842).
 * start > end → ERROR (ASM FCERRG); empty range (no lines inside) → SUCCESS (no-op).
 * Returns the first surviving line AFTER the deleted range (safe relocation anchor
 * for execution pointers); NULL if no such line (deleted to end / nothing deleted). */
struct program_line *gwlist_delete_range(int start, int end) {
    if (start > end) return NULL;

    /* Find the first surviving line after the range BEFORE deleting
     * (ASM L791-793: FNDLIN finds line beyond the last line in range). */
    struct program_line *anchor = NULL;
    struct program_line *scan = g_program_head;
    while (scan) {
        if (scan->number > end) {
            anchor = scan;
            break;
        }
        scan = scan->next;
    }

    struct program_line *curr = g_program_head;
    while (curr) {
        struct program_line *next = curr->next;
        if (curr->number >= start && curr->number <= end) {
            gwlist_delete_line(curr->number);
        }
        curr = next;
    }
    return anchor;
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

struct program_line *gwlist_delete_line(int number) {
    struct program_line *prev = NULL;
    struct program_line *curr = g_program_head;

    while (curr) {
        if (curr->number == number) {
            struct program_line *next = curr->next;
            if (prev) {
                prev->next = next;
            } else {
                g_program_head = next;
            }
            gwram_free(curr->text);
            gwram_free(curr);
            /* Return the next surviving line (ASM DEL/FNDLIN: [H,L] = pointer
             * to next line) — callers use it to relocate execution pointers
             * instead of dereferencing freed nodes (UAF). */
            return next;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL;
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
