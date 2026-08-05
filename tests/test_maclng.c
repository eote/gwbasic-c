/* TEST_MACLNG - Unit tests for MACLNG (Macro Language Driver) */
/* Based on MACLNG.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Test maclng_init */
void test_maclng_init(void) {
    maclng_init();
    
    TEST_PASS("maclng_init");
}

/* Test maclng_term */
void test_maclng_term(void) {
    maclng_term();
    
    TEST_PASS("maclng_term");
}

/* Test maclng_maclng basic */
void test_maclng_maclng_basic(void) {
    maclng_init();
    
    const char *text = "TEST";
    maclng_maclng(&text);
    
    TEST_PASS("maclng_maclng_basic");
}

/* Test maclng_mclxeq basic */
void test_maclng_mclxeq_basic(void) {
    maclng_init();
    
    const char *text = "TEST";
    maclng_mclxeq(&text);
    
    TEST_PASS("maclng_mclxeq_basic");
}

/* Test maclng_fetchr */
void test_maclng_fetchr(void) {
    maclng_init();
    
    char text[] = "test";
    const char *p = text;
    maclng_fetchr(&p);
    
    TEST_PASS("maclng_fetchr");
}

/* Test maclng_fetchz */
void test_maclng_fetchz(void) {
    maclng_init();
    
    char text[] = "test";
    const char *p = text;
    maclng_fetchz(&p);
    
    TEST_PASS("maclng_fetchz");
}

/* Test maclng_decfet */
void test_maclng_decfet(void) {
    maclng_init();
    
    char text[] = "test";
    const char *p = text + 2;
    maclng_decfet(&p);
    
    TEST_PASS("maclng_decfet");
}

/* Test maclng_valscn */
void test_maclng_valscn(void) {
    maclng_init();
    
    char text[] = "=10";
    const char *p = text;
    maclng_valscn(&p);
    
    TEST_PASS("maclng_valscn");
}

/* Test maclng_valsc2 */
void test_maclng_valsc2(void) {
    maclng_init();
    
    char text[] = "10";
    const char *p = text;
    maclng_valsc2(&p);
    
    TEST_PASS("maclng_valsc2");
}

/* Test maclng_varget */
void test_maclng_varget(void) {
    maclng_init();
    
    char text[] = "10";
    const char *p = text;
    maclng_varget(&p);
    
    TEST_PASS("maclng_varget");
}

/* Test maclng_negd */
void test_maclng_negd(void) {
    maclng_init();
    
    char text[] = "-10";
    const char *p = text;
    maclng_negd(&p);
    
    TEST_PASS("maclng_negd");
}

/* Test maclng_islet2 */
void test_maclng_islet2(void) {
    maclng_init();
    
    char text[] = "A";
    const char *p = text;
    maclng_islet2(&p);
    
    TEST_PASS("maclng_islet2");
}

/* Test maclng_scvar */
void test_maclng_scvar(void) {
    maclng_init();
    
    char text[] = "VAR";
    const char *p = text;
    maclng_scvar(&p);
    
    TEST_PASS("maclng_scvar");
}

/* Test maclng_mclnew */
void test_maclng_mclnew(void) {
    maclng_init();
    
    const char *text = "TEST";
    maclng_mclnew(&text);
    
    TEST_PASS("maclng_mclnew");
}

/* Test maclng_mclscn */
void test_maclng_mclscn(void) {
    maclng_init();
    
    const char *text = "TEST";
    maclng_mclscn(&text);
    
    TEST_PASS("maclng_mclscn");
}

/* Test maclng_maclng_main */
void test_maclng_maclng_main(void) {
    maclng_init();
    
    const char *text = "TEST";
    maclng_maclng_main(&text);
    
    TEST_PASS("maclng_maclng_main");
}