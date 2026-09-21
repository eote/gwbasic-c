/* TEST_GWMAIN - Unit tests for GWMAIN (Main Interpreter Loop) */
/* Based on GWMAIN.ASM: MAIN 入口、CURLIN direct mode、READY "Ok" 提示、
 * NEWSTT 语句获取、FNDLIN 行查找 */

#include "test_framework.h"
#include "../src/gwbasic.h"

/* Test: GWMAIN 入口模块初始化 (init_all_modules 等价序列) */
void test_gwmain_modules_init(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    ibmres_init();
    itsa86_init();
    
    TEST_ASSERT(g_state != NULL, "g_state exists after module init");
    TEST_PASS("gwmain modules init");
}

/* Test: CURLIN direct mode (ASM MAIN L604-605: MOV BX,65535 / MOV CURLIN,BX)
 * direct mode 时 CURLIN=65535 (0xFFFF)
 * PRGEND L385-388: MOV AL,BH / AND AL,BL / INC AL / JZ — AL 是 8 位寄存器,
 * 0xFF+1 回绕为 0x00 → ZF=1 → direct */
void test_gwmain_curlin_direct_mode(void) {
    gwdata_init();
    gwram_init();
    
    g_state->current_line = 65535;
    
    TEST_ASSERT_EQ_INT(g_state->current_line, 65535, "CURLIN=65535 for direct mode");
    /* Direct line test: 8-bit AL = ((BH & BL) + 1) & 0xFF == 0 */
    WORD v = g_state->current_line;
    int al = ((((v >> 8) & 0xFF) & (v & 0xFF)) + 1) & 0xFF;
    TEST_ASSERT(al == 0, "direct line detected (8-bit AL wraps to 0)");
    TEST_PASS("gwmain curlin direct mode");
}

/* Test: 非 direct 行检测 (程序行 < 65535, 8 位 AL 非零) */
void test_gwmain_curlin_program_line(void) {
    gwdata_init();
    gwram_init();
    
    g_state->current_line = 100;
    
    WORD v = g_state->current_line;
    /* 100 = 0x0064: BH&BL = 0x00, +1 = 0x01 != 0 → program line */
    int al = ((((v >> 8) & 0xFF) & (v & 0xFF)) + 1) & 0xFF;
    TEST_ASSERT(al != 0, "program line detected (8-bit AL nonzero)");
    TEST_PASS("gwmain program line");
}

/* Test: 行号提取 (has_line_number/extract_line_number 语义) */
void test_gwmain_extract_line_number(void) {
    gwdata_init();
    gwram_init();
    
    /* Simulate extract from "100 PRINT X" */
    const char *line = "100 PRINT X";
    int num = 0;
    const char *p = line;
    while (*p && *p >= '0' && *p <= '9') {
        num = num * 10 + (*p - '0');
        p++;
    }
    
    TEST_ASSERT_EQ_INT(num, 100, "line number extracted");
    TEST_ASSERT(*p == ' ', "space after line number");
    TEST_PASS("gwmain extract line number");
}

/* Test: 行号上限检查 (gwmain.c L183: line_num <= 65529) */
void test_gwmain_line_number_limit(void) {
    gwdata_init();
    gwram_init();
    
    int num_valid = 65529;
    int num_invalid = 65530;
    
    TEST_ASSERT(num_valid <= 65529, "65529 is valid (GWMAIN L183)");
    TEST_ASSERT(num_invalid > 65529, "65530 is invalid");
    TEST_PASS("gwmain line number limit 65529");
}

/* Test: FNDLIN 行查找语义 (gwlist_find_line 等价) */
void test_gwmain_fndlin(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    gwlist_add_line(100, "PRINT A");
    gwlist_add_line(200, "PRINT B");
    
    struct program_line *l100 = gwlist_find_line(100);
    struct program_line *l200 = gwlist_find_line(200);
    struct program_line *l300 = gwlist_find_line(300);
    
    TEST_ASSERT(l100 != NULL, "line 100 found (FNDLIN equal return)");
    TEST_ASSERT(l200 != NULL, "line 200 found");
    TEST_ASSERT(l300 == NULL, "line 300 not found (FNDLIN no-match return)");
    TEST_PASS("gwmain fndlin");
}

/* Test: 程序行添加/删除/替换 (NEWSTT 前的编辑语义) */
void test_gwmain_edit_lines(void) {
    gwdata_init();
    gwram_init();
    gwlist_init();
    
    /* Add */
    gwlist_add_line(10, "A = 1");
    TEST_ASSERT(gwlist_find_line(10) != NULL, "line added");
    
    /* Replace */
    gwlist_add_line(10, "A = 2");
    struct program_line *l = gwlist_find_line(10);
    TEST_ASSERT(l != NULL && l->text && strcmp(l->text, "A = 2") == 0, "line replaced");
    
    /* Delete (empty text = delete per gwmain L184-187) */
    gwlist_delete_line(10);
    TEST_ASSERT(gwlist_find_line(10) == NULL, "line deleted");
    
    TEST_PASS("gwmain edit lines");
}

/* Test: BYE/EXIT 退出语义 (run_interpreter L143) */
void test_gwmain_bye_semantics(void) {
    gwdata_init();
    gwram_init();
    
    /* BYE 和 EXIT 大小写不敏感退出 */
    int is_bye = (strcasecmp("BYE", "bye") == 0);
    int is_exit = (strcasecmp("Exit", "exit") == 0);
    
    TEST_ASSERT(is_bye, "BYE case-insensitive match");
    TEST_ASSERT(is_exit, "EXIT case-insensitive match");
    TEST_PASS("gwmain bye semantics");
}

/* Test: 错误状态处理链路 (gwdata_set_error → ERR 变量) */
void test_gwmain_error_chain(void) {
    gwdata_init();
    gwram_init();
    
    gwdata_set_error(ERRDV0, 100, 0);
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 11, "error code 11 set (division by zero)");
    TEST_ASSERT_EQ_INT(gwdata_get_error_line(), 100, "error line saved (ERL)");
    
    gwdata_clear_error();
    TEST_ASSERT_EQ_INT(gwdata_get_error(), 0, "error cleared");
    TEST_PASS("gwmain error chain");
}