/* TEST_FIVEO - Unit tests for FIVEO (5.0 Features) */
/* Based on FIVEO.ASM - WHILE/WEND, WRITE, CHAIN */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* ========== Init Tests ========== */

void test_fiveo_init(void) {
    fiveo_init();
    TEST_PASS("fiveo_init");
}

void test_fiveo_init_multiple(void) {
    fiveo_init();
    fiveo_init();
    fiveo_init();
    TEST_PASS("fiveo_init multiple calls");
}

void test_fiveo_init_clears_while_stack(void) {
    /* Push something, then re-init should clear it */
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "While stack cleared on init");
    TEST_PASS("fiveo_init clears while stack");
}

/* ========== WHILE/WEND Stack Tests ========== */

void test_fiveo_while_push(void) {
    fiveo_init();
    int rc = fiveo_push_while(100, 200, 10);
    TEST_ASSERT_EQ_INT(rc, 0, "Push while entry succeeds");
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Stack depth 1");
    TEST_PASS("fiveo_while_push");
}

void test_fiveo_while_push_multiple(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_push_while(500, 600, 30);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 3, "Stack depth 3");
    TEST_PASS("fiveo_while_push multiple");
}

void test_fiveo_while_pop(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    struct while_entry entry;
    int rc = fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(rc, 0, "Pop succeeds");
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Stack empty after pop");
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 100, "while_text_ptr preserved");
    TEST_ASSERT_EQ_INT(entry.wend_text_ptr, 200, "wend_text_ptr preserved");
    TEST_ASSERT_EQ_INT(entry.while_line, 10, "while_line preserved");
    TEST_PASS("fiveo_while_pop");
}

void test_fiveo_while_pop_empty(void) {
    fiveo_init();
    struct while_entry entry;
    int rc = fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(rc, -1, "Pop empty stack returns -1");
    TEST_PASS("fiveo_while_pop empty");
}

void test_fiveo_while_pop_order(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_push_while(500, 600, 30);
    
    struct while_entry entry;
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 500, "LIFO: last pushed popped first");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 300, "LIFO: second pushed popped second");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 100, "LIFO: first pushed popped last");
    
    TEST_PASS("fiveo_while_pop LIFO order");
}

void test_fiveo_while_depth(void) {
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Initial depth 0");
    
    fiveo_push_while(100, 200, 10);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Depth 1");
    
    fiveo_push_while(300, 400, 20);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 2, "Depth 2");
    
    struct while_entry entry;
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 1, "Depth 1 after pop");
    
    fiveo_pop_while(&entry);
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Depth 0 after second pop");
    
    TEST_PASS("fiveo_while_depth tracking");
}

void test_fiveo_while_overflow(void) {
    fiveo_init();
    int rc = 0;
    /* MAX_WHILE_DEPTH is 16 (matching MAX_FOR_DEPTH) */
    for (int i = 0; i < 16; i++) {
        rc = fiveo_push_while(100 + i, 200 + i, 10 + i);
        if (rc != 0) break;
    }
    TEST_ASSERT_EQ_INT(rc, 0, "16 levels OK");
    
    rc = fiveo_push_while(999, 999, 99);
    TEST_ASSERT_EQ_INT(rc, -1, "17th level overflows");
    
    TEST_PASS("fiveo_while overflow protection");
}

void test_fiveo_while_clear(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    fiveo_clear_while_stack();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "Stack cleared");
    TEST_PASS("fiveo_while_clear");
}

void test_fiveo_find_matching_while(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    fiveo_push_while(300, 400, 20);
    
    /* Find the entry with wend_text_ptr = 400 */
    int idx = fiveo_find_while(400);
    TEST_ASSERT_EQ_INT(idx, 1, "Found matching WHILE at index 1");
    
    TEST_PASS("fiveo_find_matching_while");
}

void test_fiveo_find_while_not_found(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    
    int idx = fiveo_find_while(999);
    TEST_ASSERT_EQ_INT(idx, -1, "Not found returns -1");
    
    TEST_PASS("fiveo_find_while not found");
}

void test_fiveo_find_while_empty_stack(void) {
    fiveo_init();
    int idx = fiveo_find_while(100);
    TEST_ASSERT_EQ_INT(idx, -1, "Empty stack returns -1");
    TEST_PASS("fiveo_find_while empty stack");
}

/* ========== WRITE Statement Tests ========== */

void test_fiveo_write_init(void) {
    fiveo_init();
    /* WRITE module doesn't need separate init, tested via fiveo_init */
    TEST_PASS("fiveo_write init safe");
}

void test_fiveo_write_clears_state(void) {
    fiveo_init();
    fiveo_write_reset();
    TEST_PASS("fiveo_write_reset safe");
}

/* ========== COMMON Variable Tests ========== */

void test_fiveo_common_mark_basic(void) {
    fiveo_init();
    fiveo_common_mark("X");
    TEST_ASSERT(fiveo_common_is_marked("X"), "X is marked");
    TEST_ASSERT(!fiveo_common_is_marked("Y"), "Y not marked");
    TEST_PASS("fiveo_common_mark basic");
}

void test_fiveo_common_mark_multiple(void) {
    fiveo_init();
    fiveo_common_mark("A");
    fiveo_common_mark("B");
    fiveo_common_mark("C");
    TEST_ASSERT(fiveo_common_is_marked("A"), "A marked");
    TEST_ASSERT(fiveo_common_is_marked("B"), "B marked");
    TEST_ASSERT(fiveo_common_is_marked("C"), "C marked");
    TEST_ASSERT(!fiveo_common_is_marked("D"), "D not marked");
    TEST_PASS("fiveo_common_mark multiple");
}

void test_fiveo_common_mark_case_insensitive(void) {
    fiveo_init();
    fiveo_common_mark("MYVAR");
    TEST_ASSERT(fiveo_common_is_marked("myvar"), "case insensitive match");
    TEST_ASSERT(fiveo_common_is_marked("MyVar"), "mixed case match");
    TEST_PASS("fiveo_common_mark case insensitive");
}

void test_fiveo_common_mark_duplicate(void) {
    fiveo_init();
    fiveo_common_mark("X");
    fiveo_common_mark("X");
    fiveo_common_mark("X");
    /* Should not add duplicates */
    fiveo_common_list_reset();
    int count = 0;
    while (fiveo_common_list_next()) count++;
    TEST_ASSERT_EQ_INT(count, 1, "Only one entry for duplicates");
    TEST_PASS("fiveo_common_mark duplicate");
}

void test_fiveo_common_mark_empty(void) {
    fiveo_init();
    fiveo_common_mark("");
    TEST_ASSERT(!fiveo_common_is_marked(""), "Empty not marked");
    TEST_PASS("fiveo_common_mark empty name");
}

void test_fiveo_common_mark_null(void) {
    fiveo_init();
    fiveo_common_mark(NULL);
    TEST_ASSERT(!fiveo_common_is_marked(NULL), "NULL not marked");
    TEST_PASS("fiveo_common_mark null name");
}

void test_fiveo_common_clear_all(void) {
    fiveo_init();
    fiveo_common_mark("A");
    fiveo_common_mark("B");
    fiveo_common_clear_all();
    TEST_ASSERT(!fiveo_common_is_marked("A"), "A cleared");
    TEST_ASSERT(!fiveo_common_is_marked("B"), "B cleared");
    TEST_PASS("fiveo_common_clear_all");
}

void test_fiveo_common_list_iterator(void) {
    fiveo_init();
    fiveo_common_mark("ALPHA");
    fiveo_common_mark("BETA");
    fiveo_common_mark("GAMMA");
    
    fiveo_common_list_reset();
    const char *name;
    int count = 0;
    while ((name = fiveo_common_list_next()) != NULL) {
        count++;
    }
    TEST_ASSERT_EQ_INT(count, 3, "Iterator returns 3 names");
    TEST_PASS("fiveo_common_list iterator");
}

void test_fiveo_common_list_empty(void) {
    fiveo_init();
    fiveo_common_list_reset();
    TEST_ASSERT(fiveo_common_list_next() == NULL, "Empty list returns NULL");
    TEST_PASS("fiveo_common_list empty");
}

void test_fiveo_common_list_reset(void) {
    fiveo_init();
    fiveo_common_mark("X");
    fiveo_common_mark("Y");
    
    fiveo_common_list_reset();
    fiveo_common_list_next();
    fiveo_common_list_next();
    
    /* After reset, should start from beginning */
    fiveo_common_list_reset();
    const char *name = fiveo_common_list_next();
    TEST_ASSERT(name != NULL, "Has entry after reset");
    TEST_ASSERT(strcmp(name, "X") == 0, "First entry after reset is X");
    TEST_PASS("fiveo_common_list_reset");
}

void test_fiveo_common_get_marked(void) {
    fiveo_init();
    fiveo_common_mark("VAR1");
    fiveo_common_mark("VAR2");
    
    char *names[10];
    int count = fiveo_common_get_marked(names, 10);
    TEST_ASSERT_EQ_INT(count, 2, "get_marked returns 2");
    TEST_PASS("fiveo_common_get_marked");
}

void test_fiveo_common_get_marked_overflow(void) {
    fiveo_init();
    fiveo_common_mark("A");
    fiveo_common_mark("B");
    fiveo_common_mark("C");
    
    char *names[2];
    int count = fiveo_common_get_marked(names, 2);
    TEST_ASSERT_EQ_INT(count, 3, "get_marked returns total count (3)");
    TEST_PASS("fiveo_common_get_marked with overflow");
}

void test_fiveo_common_init_clears(void) {
    fiveo_init();
    fiveo_common_mark("PRESERVED");
    fiveo_init();
    TEST_ASSERT(!fiveo_common_is_marked("PRESERVED"), "Cleared on init");
    TEST_PASS("fiveo_init clears common marks");
}

/* ========== CHAIN State Tests ========== */

void test_fiveo_chain_line_default(void) {
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_get_chain_line(), 0, "Default chain line 0");
    TEST_PASS("fiveo_chain_line default");
}

void test_fiveo_chain_merge_default(void) {
    fiveo_init();
    TEST_ASSERT(!fiveo_get_chain_merge(), "Default merge false");
    TEST_PASS("fiveo_chain_merge default");
}

void test_fiveo_chain_null_filename(void) {
    fiveo_init();
    int rc = fiveo_chain(NULL, 0, false, false);
    TEST_ASSERT_EQ_INT(rc, -1, "NULL filename returns -1");
    TEST_PASS("fiveo_chain null filename");
}

void test_fiveo_chain_empty_filename(void) {
    fiveo_init();
    int rc = fiveo_chain("", 0, false, false);
    TEST_ASSERT_EQ_INT(rc, -1, "Empty filename returns -1");
    TEST_PASS("fiveo_chain empty filename");
}

void test_fiveo_chain_nonexistent_file(void) {
    fiveo_init();
    int rc = fiveo_chain("NONEXIST.BAS", 0, false, false);
    TEST_ASSERT(rc != 0, "Nonexistent file returns error");
    TEST_PASS("fiveo_chain nonexistent file");
}

void test_fiveo_chain_init_resets_state(void) {
    fiveo_init();
    /* Set some state */
    fiveo_push_while(100, 200, 10);
    fiveo_common_mark("VAR");
    /* Re-init should reset */
    fiveo_init();
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 0, "While stack cleared");
    TEST_ASSERT(!fiveo_common_is_marked("VAR"), "Common cleared");
    TEST_ASSERT_EQ_INT(fiveo_get_chain_line(), 0, "Chain line cleared");
    TEST_ASSERT(!fiveo_get_chain_merge(), "Merge flag cleared");
    TEST_PASS("fiveo_init resets all state");
}

/* ========== Untested Functions Coverage (MEDIUM-3) ========== */

/* ---- fiveo_peek_while ---- */

void test_fiveo_peek_while_empty(void) {
    fiveo_init();
    /* Peek on empty stack should return -1 */
    struct while_entry entry;
    TEST_ASSERT_EQ_INT(fiveo_peek_while(&entry), -1, "Peek empty stack returns -1");
    TEST_PASS("fiveo_peek_while empty");
}

void test_fiveo_peek_while_after_push(void) {
    fiveo_init();
    fiveo_push_while(100, 200, 10);
    /* Peek should return 0 on success and fill entry */
    struct while_entry entry;
    TEST_ASSERT_EQ_INT(fiveo_peek_while(&entry), 0, "Peek returns 0 on success");
    TEST_ASSERT_EQ_INT(entry.while_text_ptr, 100, "while_text_ptr correct");
    TEST_ASSERT_EQ_INT(entry.wend_text_ptr, 200, "wend_text_ptr correct");
    TEST_ASSERT_EQ_INT(entry.while_line, 10, "while_line correct");
    TEST_PASS("fiveo_peek_while success path");
}

void test_fiveo_peek_while_does_not_pop(void) {
    fiveo_init();
    fiveo_push_while(50, 100, 5);
    fiveo_push_while(150, 200, 15);
    struct while_entry entry;
    fiveo_peek_while(&entry);
    TEST_ASSERT_EQ_INT(entry.while_line, 15, "Top is most recent");
    /* Stack depth should be unchanged */
    TEST_ASSERT_EQ_INT(fiveo_while_depth(), 2, "Peek doesn't pop");
    TEST_PASS("fiveo_peek_while non-destructive");
}

/* ---- fiveo_write_begin ---- */

void test_fiveo_write_begin_sets_state(void) {
    fiveo_init();
    /* Initially inactive */
    TEST_ASSERT(!fiveo_write_active(), "Inactive before begin");
    /* Begin WRITE to file 5 */
    fiveo_write_begin(5);
    TEST_ASSERT(fiveo_write_active(), "Active after begin");
    TEST_ASSERT_EQ_INT(fiveo_write_file(), 5, "File number set");
    TEST_PASS("fiveo_write_begin sets state");
}

void test_fiveo_write_begin_overrides(void) {
    fiveo_init();
    fiveo_write_begin(1);
    fiveo_write_begin(2);  /* Second begin should override */
    TEST_ASSERT_EQ_INT(fiveo_write_file(), 2, "Second begin overrides");
    TEST_PASS("fiveo_write_begin overrides");
}

/* ---- fiveo_write_end ---- */

void test_fiveo_write_end_clears_state(void) {
    fiveo_init();
    fiveo_write_begin(3);
    TEST_ASSERT(fiveo_write_active(), "Active after begin");
    fiveo_write_end();
    TEST_ASSERT(!fiveo_write_active(), "Inactive after end");
    TEST_PASS("fiveo_write_end clears state");
}

void test_fiveo_write_end_when_not_active(void) {
    fiveo_init();
    /* End when not active - should be safe no-op */
    fiveo_write_end();
    TEST_ASSERT(!fiveo_write_active(), "Still inactive");
    TEST_PASS("fiveo_write_end safe when inactive");
}

/* ---- fiveo_write_active ---- */

void test_fiveo_write_active_after_begin_end(void) {
    fiveo_init();
    TEST_ASSERT(!fiveo_write_active(), "Initially inactive");
    fiveo_write_begin(1);
    TEST_ASSERT(fiveo_write_active(), "Active after begin");
    fiveo_write_end();
    TEST_ASSERT(!fiveo_write_active(), "Inactive after end");
    TEST_PASS("fiveo_write_active tracks state");
}

/* ---- fiveo_write_file ---- */

void test_fiveo_write_file_default(void) {
    fiveo_init();
    /* Default should be 0 (console) */
    TEST_ASSERT_EQ_INT(fiveo_write_file(), 0, "Default file = 0 (console)");
    TEST_PASS("fiveo_write_file default");
}

void test_fiveo_write_file_returns_current(void) {
    fiveo_init();
    fiveo_write_begin(7);
    TEST_ASSERT_EQ_INT(fiveo_write_file(), 7, "Returns current file");
    fiveo_write_end();
    /* After end, file number may persist but active flag cleared */
    TEST_PASS("fiveo_write_file returns current");
}

/* ---- fiveo_common_is_marked ---- */

void test_fiveo_common_is_marked_basic(void) {
    fiveo_init();
    TEST_ASSERT(!fiveo_common_is_marked("X"), "Unmarked returns false");
    fiveo_common_mark("X");
    TEST_ASSERT(fiveo_common_is_marked("X"), "Marked returns true");
    TEST_PASS("fiveo_common_is_marked basic");
}

void test_fiveo_common_is_marked_case_insensitive(void) {
    fiveo_init();
    fiveo_common_mark("X");
    TEST_ASSERT(fiveo_common_is_marked("x"), "Lowercase match");
    TEST_ASSERT(fiveo_common_is_marked("X"), "Uppercase match");
    TEST_PASS("fiveo_common_is_marked case insensitive");
}

void test_fiveo_common_is_marked_null(void) {
    fiveo_init();
    TEST_ASSERT(!fiveo_common_is_marked(NULL), "NULL returns false");
    TEST_PASS("fiveo_common_is_marked NULL safe");
}
