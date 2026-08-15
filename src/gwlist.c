/* GWLIST - Program Storage & Listing */
/* Based on GWLIST.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    char line[256];
    int line_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
        line[strcspn(line, "\n")] = '\0';

        /* Skip empty lines and comments */
        char *trimmed = line;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        if (!*trimmed || *trimmed == '\'') continue;

        /* Extract line number */
        int num = 0;
        char *p = trimmed;
        while (*p && isdigit((unsigned char)*p)) {
            num = num * 10 + (*p - '0');
            p++;
        }

        if (num > 0 && num <= 65529) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p) {
                DEBUG_PRINT("Adding line %d: '%s'", num, p);
                gwlist_add_line(num, p);
                line_count++;
            }
        }
    }

    DEBUG_PRINT("Loaded %d lines", line_count);

    fclose(fp);
    return SUCCESS;
}