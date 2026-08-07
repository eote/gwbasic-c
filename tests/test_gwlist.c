/* TEST_GWLIST - Unit tests for GWLIST (Program Line Storage) */
/* Based on GWLIST.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test gwlist_init */
void test_gwlist_init(void) {
    gwlist_init();
    TEST_ASSERT(gwlist_get_first_line() == NULL, "Program list is empty after init");
    TEST_PASS("gwlist_init");
}

/* Test gwlist_add_line - single line */
void test_gwlist_add_single(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"HELLO\"");
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT(line != NULL, "Line added");
    TEST_ASSERT_EQ_INT(line->number, 10, "Line number is 10");
    TEST_ASSERT(strcmp(line->text, "PRINT \"HELLO\"") == 0, "Line text matches");
    TEST_PASS("gwlist_add_single");
}

/* Test gwlist_add_line - multiple lines in order */
void test_gwlist_add_multiple(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"FIRST\"");
    gwlist_add_line(20, "PRINT \"SECOND\"");
    gwlist_add_line(30, "PRINT \"THIRD\"");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "First line is 10");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 20, "Second line is 20");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 30, "Third line is 30");
    TEST_ASSERT(line->next == NULL, "No more lines");
    TEST_PASS("gwlist_add_multiple");
}

/* Test gwlist_add_line - out of order insertion */
void test_gwlist_add_out_of_order(void) {
    gwlist_init();
    gwlist_add_line(30, "PRINT \"THIRD\"");
    gwlist_add_line(10, "PRINT \"FIRST\"");
    gwlist_add_line(20, "PRINT \"SECOND\"");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "First line is 10");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 20, "Second line is 20");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 30, "Third line is 30");
    TEST_PASS("gwlist_add_out_of_order");
}

/* Test gwlist_add_line - replace existing line */
void test_gwlist_replace_line(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"OLD\"");
    gwlist_add_line(10, "PRINT \"NEW\"");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "Line number is 10");
    TEST_ASSERT(strcmp(line->text, "PRINT \"NEW\"") == 0, "Line text replaced");
    TEST_ASSERT(line->next == NULL, "Only one line exists");
    TEST_PASS("gwlist_replace_line");
}

/* Test gwlist_find_line */
void test_gwlist_find_line(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    
    struct program_line *found = gwlist_find_line(20);
    TEST_ASSERT(found != NULL, "Line 20 found");
    TEST_ASSERT_EQ_INT(found->number, 20, "Found line number is 20");
    TEST_ASSERT(strcmp(found->text, "PRINT \"B\"") == 0, "Found line text matches");
    
    found = gwlist_find_line(999);
    TEST_ASSERT(found == NULL, "Line 999 not found");
    TEST_PASS("gwlist_find_line");
}

/* Test gwlist_delete_line - delete middle */
void test_gwlist_delete_middle(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    
    gwlist_delete_line(20);
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "First line is 10");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 30, "Second line is 30 (20 deleted)");
    TEST_ASSERT(line->next == NULL, "Only two lines");
    TEST_PASS("gwlist_delete_middle");
}

/* Test gwlist_delete_line - delete first */
void test_gwlist_delete_first(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    
    gwlist_delete_line(10);
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 20, "First line is now 20");
    TEST_ASSERT(line->next == NULL, "Only one line");
    TEST_PASS("gwlist_delete_first");
}

/* Test gwlist_delete_line - delete last */
void test_gwlist_delete_last(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    
    gwlist_delete_line(20);
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "First line is 10");
    TEST_ASSERT(line->next == NULL, "Only one line");
    TEST_PASS("gwlist_delete_last");
}

/* Test gwlist_delete_line - delete non-existent */
void test_gwlist_delete_nonexistent(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    
    gwlist_delete_line(999);  /* Should not crash */
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "Line 10 still exists");
    TEST_PASS("gwlist_delete_nonexistent");
}

/* Test gwlist_list - basic */
void test_gwlist_list_basic(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    
    /* Just verify it doesn't crash - output goes to stdout */
    gwlist_list(0, 0);
    TEST_PASS("gwlist_list_basic");
}

/* Test gwlist_list - range */
void test_gwlist_list_range(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    gwlist_add_line(40, "PRINT \"D\"");
    
    gwlist_list(15, 35);  /* Should list lines 20, 30 */
    TEST_PASS("gwlist_list_range");
}

/* Test gwlist_list - start only */
void test_gwlist_list_start_only(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    
    gwlist_list(20, 0);  /* Should list lines 20, 30 */
    TEST_PASS("gwlist_list_start_only");
}

/* Test gwlist_list - end only */
void test_gwlist_list_end_only(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"A\"");
    gwlist_add_line(20, "PRINT \"B\"");
    gwlist_add_line(30, "PRINT \"C\"");
    
    gwlist_list(0, 20);  /* Should list lines 10, 20 */
    TEST_PASS("gwlist_list_end_only");
}

/* Test gwlist_list - empty list */
void test_gwlist_list_empty(void) {
    gwlist_init();
    gwlist_list(0, 0);  /* Should not crash */
    TEST_PASS("gwlist_list_empty");
}

/* Test gwlist - multiple add/delete cycles */
void test_gwlist_cycles(void) {
    gwlist_init();
    
    for (int cycle = 0; cycle < 5; cycle++) {
        gwlist_add_line(10, "LINE 10");
        gwlist_add_line(20, "LINE 20");
        gwlist_add_line(30, "LINE 30");
        
        gwlist_delete_line(20);
        gwlist_delete_line(10);
        gwlist_delete_line(30);
    }
    
    TEST_ASSERT(gwlist_get_first_line() == NULL, "List empty after cycles");
    TEST_PASS("gwlist_cycles");
}

/* Test gwlist - large line numbers */
void test_gwlist_large_numbers(void) {
    gwlist_init();
    gwlist_add_line(65535, "MAX LINE");
    gwlist_add_line(1, "MIN LINE");
    
    struct program_line *first = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(first->number, 1, "First line is 1");
    TEST_ASSERT_EQ_INT(first->next->number, 65535, "Second line is 65535");
    TEST_PASS("gwlist_large_numbers");
}

/* Test gwlist - empty line text */
void test_gwlist_empty_text(void) {
    gwlist_init();
    gwlist_add_line(10, "");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "Line number is 10");
    TEST_ASSERT(strlen(line->text) == 0, "Text is empty");
    TEST_PASS("gwlist_empty_text");
}

/* Test gwlist - long line text */
void test_gwlist_long_text(void) {
    gwlist_init();
    char long_text[500];
    memset(long_text, 'A', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    
    gwlist_add_line(10, long_text);
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT(strcmp(line->text, long_text) == 0, "Long text stored correctly");
    TEST_PASS("gwlist_long_text");
}

/* Test gwlist - delete all lines */
void test_gwlist_delete_all(void) {
    gwlist_init();
    gwlist_add_line(10, "A");
    gwlist_add_line(20, "B");
    gwlist_add_line(30, "C");
    gwlist_add_line(40, "D");
    gwlist_add_line(50, "E");
    
    gwlist_delete_line(30);
    gwlist_delete_line(10);
    gwlist_delete_line(50);
    gwlist_delete_line(20);
    gwlist_delete_line(40);
    
    TEST_ASSERT(gwlist_get_first_line() == NULL, "All lines deleted");
    TEST_PASS("gwlist_delete_all");
}

/* Test gwlist - insert at beginning */
void test_gwlist_insert_at_beginning(void) {
    gwlist_init();
    gwlist_add_line(20, "SECOND");
    gwlist_add_line(30, "THIRD");
    gwlist_add_line(10, "FIRST");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT_EQ_INT(line->number, 10, "First line is 10");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 20, "Second line is 20");
    line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 30, "Third line is 30");
    TEST_PASS("gwlist_insert_at_beginning");
}

/* Test gwlist - insert at end */
void test_gwlist_insert_at_end(void) {
    gwlist_init();
    gwlist_add_line(10, "FIRST");
    gwlist_add_line(20, "SECOND");
    gwlist_add_line(30, "THIRD");
    
    struct program_line *line = gwlist_get_first_line();
    while (line->next) line = line->next;
    TEST_ASSERT_EQ_INT(line->number, 30, "Last line is 30");
    TEST_PASS("gwlist_insert_at_end");
}

/* Test gwlist - add line with special characters */
void test_gwlist_special_chars(void) {
    gwlist_init();
    gwlist_add_line(10, "PRINT \"Hello, World!\"");
    gwlist_add_line(20, "IF A>B THEN GOTO 100");
    gwlist_add_line(30, "FOR I=1 TO 10: NEXT I");
    
    struct program_line *line = gwlist_find_line(20);
    TEST_ASSERT(strcmp(line->text, "IF A>B THEN GOTO 100") == 0, "Special chars preserved");
    TEST_PASS("gwlist_special_chars");
}

/* Test gwlist - init twice */
void test_gwlist_init_twice(void) {
    gwlist_init();
    gwlist_add_line(10, "TEST");
    gwlist_init();  /* Re-init should clear */
    TEST_ASSERT(gwlist_get_first_line() == NULL, "List empty after re-init");
    TEST_PASS("gwlist_init_twice");
}

/* Test gwlist - delete then add same number */
void test_gwlist_delete_then_add_same(void) {
    gwlist_init();
    gwlist_add_line(10, "OLD");
    gwlist_delete_line(10);
    gwlist_add_line(10, "NEW");
    
    struct program_line *line = gwlist_get_first_line();
    TEST_ASSERT(strcmp(line->text, "NEW") == 0, "New text stored");
    TEST_PASS("gwlist_delete_then_add_same");
}

/* Test gwlist - many lines */
void test_gwlist_many_lines(void) {
    gwlist_init();
    for (int i = 1; i <= 100; i++) {
        char text[32];
        snprintf(text, sizeof(text), "LINE %d", i);
        gwlist_add_line(i * 10, text);
    }
    
    int count = 0;
    struct program_line *line = gwlist_get_first_line();
    while (line) {
        TEST_ASSERT_EQ_INT(line->number, (count + 1) * 10, "Line number correct");
        count++;
        line = line->next;
    }
    TEST_ASSERT_EQ_INT(count, 100, "100 lines stored");
    TEST_PASS("gwlist_many_lines");
}