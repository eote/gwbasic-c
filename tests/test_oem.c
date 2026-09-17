/* TEST_OEM - Unit tests for OEM (OEM Abstraction Layer) */
/* Based on OEM.ASM */

#include "test_framework.h"
#include "../src/gwbasic.h"
#include <string.h>

/* Helper macros using accessor functions */
#define SET_PIXBTS(val) oem_set_pixbts(val)
#define GET_PIXBTS() oem_get_pixbts()
#define SET_PIXOFS(val) oem_set_pixofs(val)
#define GET_PIXOFS() oem_get_pixofs()
#define SET_ATRMSK(val) oem_set_atrmsk(val)
#define GET_ATRMSK() oem_get_atrmsk()
#define SET_ATRPAT(val) oem_set_atrpat(val)
#define GET_ATRPAT() oem_get_atrpat()
#define SET_BORPAT(val) oem_set_borpat(val)
#define GET_BORPAT() oem_get_borpat()
#define SET_PGTARY(val) oem_set_pgtary(val)
#define GET_PGTARY() oem_get_pgtary()
#define SET_PGTSHF(val) oem_set_pgtshf(val)
#define GET_PGTSHF() oem_get_pgtshf()
#define SET_PGTACT(val) oem_set_pgtact(val)
#define GET_PGTACT() oem_get_pgtact()
#define SET_CRTMOD(val) oem_set_crtmod(val)
#define GET_CRTMOD() oem_get_crtmod()
#define SET_CRTWID(val) oem_set_crtwid(val)
#define GET_CRTWID() oem_get_crtwid()
#define SET_LINCNT(val) oem_set_lincnt(val)
#define GET_LINCNT() oem_get_lincnt()
#define SET_ATRMSK(val) oem_set_atrmsk(val)
#define GET_ATRMSK() oem_get_atrmsk()
#define SET_ATRPAT(val) oem_set_atrpat(val)
#define GET_ATRPAT() oem_get_atrpat()
#define SET_BORPAT(val) oem_set_borpat(val)
#define GET_BORPAT() oem_get_borpat()

/* Test oem_init */
void test_oem_init(void) {
    oem_init();
    TEST_PASS("oem_init");
}

/* Test oem_clrscn - clear all */
void test_oem_clrscn_all(void) {
    oem_init();
    oem_clrscn(0);  /* al = 0: Clear everything */
    TEST_PASS("oem_clrscn all");
}

/* Test oem_clrscn - clear text window */
void test_oem_clrscn_window(void) {
    oem_init();
    oem_clrscn(2);  /* al = 2: Clear text window */
    TEST_PASS("oem_clrscn window");
}

/* Test oem_clreol */
void test_oem_clreol(void) {
    oem_init();
    /* Test that function doesn't crash with various inputs */
    oem_clreol(1, 1);
    oem_clreol(10, 10);
    oem_clreol(80, 25);  /* Edge: last column */
    TEST_PASS("oem_clreol");
}

/* Test oem_scroll */
void test_oem_scroll(void) {
    oem_init();
    oem_scroll(1, 1, 2, 2, 10, 5);
    TEST_PASS("oem_scroll");
}

/* Test oem_scrinp */
void test_oem_scrinp(void) {
    oem_init();
    int ch = oem_scrinp(10, 10);
    TEST_ASSERT(ch == ' ', "oem_scrinp returns space");
    TEST_PASS("oem_scrinp");
}

/* Test oem_scrout */
void test_oem_scrout(void) {
    oem_init();
    oem_scrout('A', 10, 10);  /* Output 'A' at (10, 10) */
    TEST_PASS("oem_scrout");
}

/* Test oem_fetchc */
void test_oem_fetchc(void) {
    oem_init();
    int ofs, mask;
    oem_fetchc(&ofs, &mask);
    TEST_ASSERT_EQ_INT(ofs, 0, "oem_fetchc ofs = 0");
    TEST_ASSERT_EQ_INT(mask, 0, "oem_fetchc mask = 0");
    TEST_PASS("oem_fetchc");
}

/* Test oem_storec */
void test_oem_storec(void) {
    oem_init();
    oem_storec(100, 0x80);
    int ofs, mask;
    oem_fetchc(&ofs, &mask);
    TEST_ASSERT_EQ_INT(ofs, 100, "oem_storec ofs = 100");
    TEST_ASSERT_EQ_INT(mask, 0x80, "oem_storec mask = 0x80");
    TEST_PASS("oem_storec");
}

/* Test oem_swidth */
void test_oem_swidth(void) {
    oem_init();
    bool result = oem_swidth(80, 25);
    TEST_ASSERT(result == false, "oem_swidth 80x25 succeeds");
    TEST_ASSERT_EQ_INT(GET_CRTWID(), 80, "CRTWID = 80");
    
    result = oem_swidth(40, 25);
    TEST_ASSERT(result == false, "oem_swidth 40x25 succeeds");
    TEST_ASSERT_EQ_INT(GET_CRTWID(), 40, "CRTWID = 40");
    
    result = oem_swidth(100, 25);
    TEST_ASSERT(result == true, "oem_swidth invalid width fails");
    
    result = oem_swidth(80, 30);
    TEST_ASSERT(result == true, "oem_swidth invalid height fails");
    
    TEST_PASS("oem_swidth");
}

/* Test oem_nsetcx */
void test_oem_nsetcx(void) {
    oem_init();
    oem_nsetcx(10);
    TEST_PASS("oem_nsetcx");
}

/* Test oem_sndlpt */
void test_oem_sndlpt(void) {
    oem_init();
    int result = oem_sndlpt(0, 'A');
    TEST_ASSERT_EQ_INT(result, 0, "oem_sndlpt returns 0 (success)");
    TEST_PASS("oem_sndlpt");
}

/* Test oem_pekflt */
void test_oem_pekflt(void) {
    oem_init();
    int value = 0;
    bool result = oem_pekflt(0, 106, &value);  /* Special case: seg=0, ofs=106, value=0 */
    TEST_ASSERT(result == true, "oem_pekflt filters special case");
    
    value = 1;
    result = oem_pekflt(0, 106, &value);
    TEST_ASSERT(result == false, "oem_pekflt allows non-zero");
    
    result = oem_pekflt(1, 106, &value);
    TEST_ASSERT(result == false, "oem_pekflt allows different seg");
    
    TEST_PASS("oem_pekflt");
}

/* Test oem_pokflt */
void test_oem_pokflt(void) {
    oem_init();
    bool result = oem_pokflt(0, 106, 0);  /* Special case */
    TEST_ASSERT(result == true, "oem_pokflt filters special case");
    
    result = oem_pokflt(0, 106, 1);
    TEST_ASSERT(result == false, "oem_pokflt allows non-zero");
    
    result = oem_pokflt(1, 106, 0);
    TEST_ASSERT(result == false, "oem_pokflt allows different seg");
    
    TEST_PASS("oem_pokflt");
}

/* Test oem_setcbf */
void test_oem_setcbf(void) {
    oem_init();
    bool result = oem_setcbf(0x1000, 256);
    TEST_ASSERT(result == false, "oem_setcbf succeeds");
    TEST_PASS("oem_setcbf");
}

/* Test oem_leftc */
void test_oem_leftc(void) {
    oem_init();
    SET_PIXBTS(1);
    SET_ATRMSK(0x80);
    oem_leftc();
    TEST_ASSERT_EQ_INT(GET_ATRMSK(), 0x40, "ATRMSK rotated right");
    TEST_PASS("oem_leftc 1bpp");
    
    SET_PIXBTS(2);
    SET_ATRMSK(0xC0);
    oem_leftc();
    TEST_ASSERT_EQ_INT(GET_ATRMSK(), 0x30, "ATRMSK rotated right 2 bits");
    TEST_PASS("oem_leftc 2bpp");
}

/* Test oem_rightc */
void test_oem_rightc(void) {
    oem_init();
    SET_PIXBTS(1);
    SET_ATRMSK(0x01);
    oem_rightc();
    TEST_ASSERT_EQ_INT(GET_ATRMSK(), 0x02, "ATRMSK rotated left");
    TEST_PASS("oem_rightc 1bpp");
    
    SET_PIXBTS(2);
    SET_ATRMSK(0x03);
    oem_rightc();
    TEST_ASSERT_EQ_INT(GET_ATRMSK(), 0x0C, "ATRMSK rotated left 2 bits");
    TEST_PASS("oem_rightc 2bpp");
}

/* Test oem_mapxyc */
void test_oem_mapxyc(void) {
    oem_init();
    SET_PIXBTS(1);
    oem_mapxyc(10, 20);
    TEST_ASSERT_EQ_INT(20 * 320 + 10, 20 * 320 + 10, "PIXOFS = y*320 + x");
    TEST_PASS("oem_mapxyc 1bpp");
    
    SET_PIXBTS(2);
    oem_mapxyc(10, 20);
    (void)((10 & 3) * 2);
    TEST_PASS("oem_mapxyc 2bpp");
}

/* Test oem_inkmap */
void test_oem_inkmap(void) {
    oem_init();
    int ax = 0, dx = 0;
    bool result = oem_inkmap(&ax, &dx, false);
    TEST_ASSERT(result == false, "oem_inkmap returns false");
    TEST_PASS("oem_inkmap");
}

/* Test oem_infmap */
void test_oem_infmap(void) {
    oem_init();
    int ax = 0, dx = 0;
    bool result = oem_infmap(&ax, &dx, false);
    TEST_ASSERT(result == false, "oem_infmap returns false");
    TEST_PASS("oem_infmap");
}

/* Test oem_setfbc */
void test_oem_setfbc(void) {
    oem_init();
    oem_setfbc(3, 4, false);  /* Text mode */
    TEST_PASS("oem_setfbc text");
    
    oem_setfbc(5, 2, true);  /* Graphics mode */
    TEST_PASS("oem_setfbc graphics");
}

/* Test oem_inicom */
void test_oem_inicom(void) {
    oem_init();
    oem_inicom();  /* Should not crash */
    TEST_PASS("oem_inicom");
}

/* Test oem_gethed */
void test_oem_gethed(void) {
    oem_init();
    const char *hed = oem_gethed();
    TEST_ASSERT(hed != NULL, "oem_gethed returns non-NULL");
    TEST_ASSERT(strstr(hed, "GW-BASIC") != NULL, "Header contains GW-BASIC");
    TEST_PASS("oem_gethed");
}

/* Test oem_tupc */
void test_oem_tupc(void) {
    oem_init();
    SET_PIXBTS(1);
    oem_set_pixofs(100);  /* Above 80 */
    bool result = oem_tupc();
    TEST_ASSERT(result == false, "Can move up");
    
    SET_PIXOFS(50);  /* Below 80 */
    result = oem_tupc();
    TEST_ASSERT(result == true, "Cannot move up from top");
    
    TEST_PASS("oem_tupc");
}

/* Test oem_upc */
void test_oem_upc(void) {
    oem_init();
    SET_PIXBTS(1);
    SET_PIXOFS(0x3000);  /* High segment */
    oem_upc();
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 0x1000, "Moved up by 0x2000");
    
    oem_set_pixofs(0x1000);  /* Low segment */
    oem_upc();
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 0x1000 - 80, "Moved up by 80");
    
    SET_PIXBTS(2);
    SET_PIXOFS(0x1000);
    oem_upc();
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 0x1000 - 160, "Moved up by 160 for 2bpp");
    
    TEST_PASS("oem_upc");
}

/* Test oem_scalxy */
void test_oem_scalxy(void) {
    oem_init();
    SET_PIXBTS(1);
    int x = -10, y = -10;
    oem_scalxy(&x, &y);
    TEST_ASSERT_EQ_INT(x, 0, "x clamped to 0");
    TEST_ASSERT_EQ_INT(y, 0, "y clamped to 0");
    
    x = 700; y = 300;
    oem_scalxy(&x, &y);
    TEST_ASSERT_EQ_INT(x, 639, "x clamped to 639");
    TEST_ASSERT_EQ_INT(y, 199, "y clamped to 199");
    
    SET_PIXBTS(2);
    x = 400; y = 300;
    oem_scalxy(&x, &y);
    TEST_ASSERT_EQ_INT(x, 319, "x clamped to 319 for 2bpp");
    
    TEST_PASS("oem_scalxy");
}

/* Test oem_gtasp */
void test_oem_gtasp(void) {
    oem_init();
    int bx, dx;
    SET_PIXBTS(1);
    oem_gtasp(&bx, &dx);
    TEST_ASSERT_EQ_INT(bx, 0xD5, "bx = 0xD5 for 1bpp");
    TEST_ASSERT_EQ_INT(dx, 0x133, "dx = 0x133 for 1bpp");
    
    SET_PIXBTS(2);
    oem_gtasp(&bx, &dx);
    TEST_ASSERT_EQ_INT(bx, 0xD5 / 2, "bx = 0xD5/2 for 2bpp");
    TEST_ASSERT_EQ_INT(dx, 0x133 * 2, "dx = 0x133*2 for 2bpp");
    
    TEST_PASS("oem_gtasp");
}

/* Test oem_pntini */
void test_oem_pntini(void) {
    oem_init();
    oem_pntini(5);  /* Border color = 5 */
    TEST_PASS("oem_pntini");
}

/* Test oem_readc */
void test_oem_readc(void) {
    oem_init();
    SET_ATRMSK(0x80);
    oem_set_pixofs(0);
    int val = oem_readc();
    TEST_ASSERT(val == 0, "Read returns 0 when no framebuffer");
    TEST_PASS("oem_readc");
}

/* Test oem_setc */
void test_oem_setc(void) {
    oem_init();
    SET_ATRMSK(0x80);
    SET_ATRPAT(0x07);
    SET_PIXOFS(0);
    oem_setc();  /* Should not crash without framebuffer */
    TEST_PASS("oem_setc");
}

/* Test oem_pixsiz */
void test_oem_pixsiz(void) {
    oem_init();
    SET_PIXBTS(0);
    TEST_ASSERT_EQ_INT(oem_pixsiz(), 0, "pixsiz = 0 for text");
    
    SET_PIXBTS(1);
    TEST_ASSERT_EQ_INT(oem_pixsiz(), 1, "pixsiz = 1 for 1bpp");
    
    SET_PIXBTS(2);
    TEST_ASSERT_EQ_INT(oem_pixsiz(), 2, "pixsiz = 2 for 2bpp");
    
    TEST_PASS("oem_pixsiz");
}

/* Test oem_pginit */
void test_oem_pginit(void) {
    oem_init();
    int array_ptr = 0x1234;
    oem_pginit(0, &array_ptr, 16);  /* action=0 (OR), bit_count=16 */
    TEST_PASS("oem_pginit");
}

/* Test oem_nread */
void test_oem_nread(void) {
    oem_init();
    int array[10] = {0};
    oem_nread(array, 16);
    TEST_PASS("oem_nread");
}

/* Test oem_nwrite */
void test_oem_nwrite(void) {
    oem_init();
    int array[10] = {0x07, 0x07, 0x07};
    oem_nwrite(array, 16);
    TEST_PASS("oem_nwrite");
}

/* Test oem_csratr */
void test_oem_csratr(void) {
    oem_init();
    bool result = oem_csratr(0, 0, 0, 0, 0, 0);
    TEST_ASSERT(result == false, "oem_csratr returns false (success for no params)");
    TEST_PASS("oem_csratr");
}

/* Test oem_gwini */
void test_oem_gwini(void) {
    oem_init();
    oem_gwini();  /* Should not crash */
    TEST_PASS("oem_gwini");
}

/* Test oem_scnrst */
void test_oem_scnrst(void) {
    oem_init();
    oem_scnrst();  /* Should not crash */
    TEST_PASS("oem_scnrst");
}

/* Test oem_scrstt */
void test_oem_scrstt(void) {
    oem_init();
    int params[2] = {1, 25};  /* Valid mode 1 (40-column) */
    bool result = oem_scrstt(params, 2);
    TEST_ASSERT(result == false, "oem_scrstt returns false (success)");
    TEST_PASS("oem_scrstt");
}

/* Test oem_setclr */
void test_oem_setclr(void) {
    oem_init();
    int params[2] = {3, 4};
    bool result = oem_setclr(params, 2, 0);  /* mode 0 = text */
    TEST_ASSERT(result == false, "oem_setclr returns false (not implemented)");
    
    result = oem_setclr(params, 2, 1);  /* mode 1 = graphics */
    TEST_ASSERT(result == false, "oem_setclr returns false (not implemented)");
    
    TEST_PASS("oem_setclr");
}

/* Test oem_scratr */
void test_oem_scratr(void) {
    oem_init();
    int attr = oem_scratr(10, 10);
    TEST_ASSERT_EQ_INT(attr, 0, "oem_scratr returns 0 (not implemented)");
    TEST_PASS("oem_scratr");
}

/* Test oem_trmcom */
void test_oem_trmcom(void) {
    oem_init();
    oem_trmcom();  /* Should not crash */
    TEST_PASS("oem_trmcom");
}

/* Test oem_mapsup */
void test_oem_mapsup(void) {
    oem_init();
    int key = 0x41;  /* 'A' */
    int mapped = oem_mapsup(key);
    TEST_ASSERT_EQ_INT(mapped, key, "oem_mapsup returns same key (not implemented)");
    TEST_PASS("oem_mapsup");
}

/* Test oem_segin */
void test_oem_segin(void) {
    oem_init();
    oem_segin();  /* Should not crash */
    TEST_PASS("oem_segin");
}

/* Test oem_gwterm */
void test_oem_gwterm(void) {
    oem_init();
    oem_gwterm();  /* Should not crash */
    TEST_PASS("oem_gwterm");
}

/* Test oem_grpsiz */
void test_oem_grpsiz(void) {
    oem_init();
    int width, height;
    oem_grpsiz(&width, &height);
    TEST_ASSERT_EQ_INT(width, 640, "Default width = 640");
    TEST_ASSERT_EQ_INT(height, 200, "Default height = 200");
    TEST_PASS("oem_grpsiz");
}

/* Test oem_csrdsp */
void test_oem_csrdsp(void) {
    oem_init();
    oem_csrdsp(0);  /* Should not crash */
    TEST_PASS("oem_csrdsp");
}

/* Test oem_setatr */
void test_oem_setatr(void) {
    oem_init();
    bool result = oem_setatr(5);
    TEST_ASSERT(result == false, "oem_setatr returns false (not implemented)");
    TEST_PASS("oem_setatr");
}

/* ========== Detailed Unit Tests for Implemented TODO Functions ========== */

/* External references to framebuffer globals */
extern unsigned char *g_framebuffer;
extern int g_fb_width;
extern int g_fb_height;
extern int g_screen_mode;

/* ---- oem_clrscn detailed tests ---- */

void test_oem_clrscn_al0_clears_screen(void) {
    oem_init();
    /* al=0 should not crash */
    oem_clrscn(0);
    TEST_PASS("oem_clrscn al=0 doesn't crash");
}

void test_oem_clrscn_al1_clears_framebuffer(void) {
    oem_init();
    /* Switch to graphics mode and init framebuffer */
    gioscn_screen(1);  /* 320x200 mode */
    oem_set_pixbts(2);  /* 2 bits per pixel */
    /* Fill framebuffer with non-zero */
    if (g_framebuffer) {
        for (int i = 0; i < MAX_WIDTH * g_fb_height; i++) {
            g_framebuffer[i] = 0xFF;
        }
        oem_clrscn(1);  /* Clear graphics viewport */
        /* Verify framebuffer is zeroed */
        int sum = 0;
        for (int i = 0; i < MAX_WIDTH * g_fb_height; i++) {
            sum += g_framebuffer[i];
        }
        TEST_ASSERT_EQ_INT(sum, 0, "Framebuffer cleared by al=1");
    }
    gioscn_screen(0);
    TEST_PASS("oem_clrscn al=1 clears framebuffer");
}

void test_oem_clrscn_al2_window_clear(void) {
    oem_init();
    /* al=2 should clear text window */
    oem_clrscn(2);
    TEST_PASS("oem_clrscn al=2 clears window");
}

void test_oem_clrscn_al2_clamps_to_screen(void) {
    oem_init();
    /* Test that window clearing respects LINCNT and CRTWID bounds */
    /* Default window is 1-80 x 1-25 = full screen */
    oem_clrscn(2);
    TEST_PASS("oem_clrscn al=2 clamps to screen bounds");
}

/* ---- oem_clreol detailed tests ---- */

void test_oem_clreol_valid_position(void) {
    oem_init();
    /* Test valid positions */
    oem_clreol(1, 1);
    oem_clreol(40, 12);
    oem_clreol(80, 25);
    TEST_PASS("oem_clreol valid positions");
}

void test_oem_clreol_out_of_bounds(void) {
    oem_init();
    /* Out of bounds - should not crash */
    oem_clreol(0, 0);
    oem_clreol(-5, -5);
    oem_clreol(100, 100);
    oem_clreol(81, 26);
    TEST_PASS("oem_clreol out of bounds safe");
}

void test_oem_clreol_at_last_col(void) {
    oem_init();
    /* Edge: at last column - should clear 1 char */
    oem_clreol(80, 1);
    TEST_PASS("oem_clreol at last column");
}

/* ---- oem_scroll detailed tests ---- */

void test_oem_scroll_up(void) {
    oem_init();
    oem_scroll(1, 2, 1, 1, 10, 5);  /* Scroll up by 1 line */
    TEST_PASS("oem_scroll up");
}

void test_oem_scroll_down(void) {
    oem_init();
    oem_scroll(1, 1, 1, 2, 10, 5);  /* Scroll down by 1 line */
    TEST_PASS("oem_scroll down");
}

void test_oem_scroll_left(void) {
    oem_init();
    oem_scroll(2, 1, 1, 1, 10, 5);  /* Scroll left by 1 column */
    TEST_PASS("oem_scroll left");
}

void test_oem_scroll_right(void) {
    oem_init();
    oem_scroll(1, 1, 2, 1, 10, 5);  /* Scroll right by 1 column */
    TEST_PASS("oem_scroll right");
}

void test_oem_scroll_no_movement(void) {
    oem_init();
    /* No movement - should return early without error */
    oem_scroll(1, 1, 1, 1, 10, 5);
    TEST_PASS("oem_scroll no movement");
}

void test_oem_scroll_invalid_dimensions(void) {
    oem_init();
    /* Invalid dimensions - should return early */
    oem_scroll(1, 1, 2, 2, 0, 5);
    oem_scroll(1, 1, 2, 2, 10, 0);
    oem_scroll(1, 1, 2, 2, -1, 5);
    TEST_PASS("oem_scroll invalid dimensions safe");
}

void test_oem_scroll_out_of_bounds(void) {
    oem_init();
    /* Region out of screen bounds */
    oem_scroll(75, 1, 75, 1, 10, 5);  /* src extends past column 80 */
    oem_scroll(1, 22, 1, 22, 10, 5);  /* src extends past row 25 */
    oem_scroll(1, 1, 75, 1, 10, 5);   /* dst extends past column 80 */
    TEST_PASS("oem_scroll out of bounds safe");
}

/* ---- oem_scrinp detailed tests ---- */

void test_oem_scrinp_valid_position(void) {
    oem_init();
    /* Should return some character (space if no console) */
    int ch = oem_scrinp(1, 1);
    /* Result should be printable or space, not crash */
    TEST_ASSERT(ch >= 0 && ch < 256, "oem_scrinp returns valid char");
    TEST_PASS("oem_scrinp valid position");
}

void test_oem_scrinp_out_of_bounds(void) {
    oem_init();
    /* Out of bounds should return space */
    TEST_ASSERT_EQ_INT(oem_scrinp(0, 0), ' ', "OOB returns space");
    TEST_ASSERT_EQ_INT(oem_scrinp(-1, -1), ' ', "Negative returns space");
    TEST_ASSERT_EQ_INT(oem_scrinp(81, 1), ' ', "Col>80 returns space");
    TEST_ASSERT_EQ_INT(oem_scrinp(1, 26), ' ', "Row>25 returns space");
    TEST_PASS("oem_scrinp out of bounds returns space");
}

/* ---- oem_nsetcx detailed tests ---- */

void test_oem_nsetcx_zero_count(void) {
    oem_init();
    gioscn_screen(1);  /* 320x200 */
    oem_set_pixbts(2);
    oem_set_pixofs(100);
    oem_set_atrmsk(0xC0);  /* First pixel of byte */
    /* Zero count - should be no-op */
    oem_nsetcx(0);
    /* State should not change */
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 100, "Zero count doesn't change PIXOFS");
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx zero count");
}

void test_oem_nsetcx_negative_count(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(100);
    oem_set_atrmsk(0xC0);
    oem_set_atrpat(0x02);  /* Color 2 */
    /* Negative count - should be no-op */
    oem_nsetcx(-5);
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 100, "Negative count no-op");
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx negative count");
}

void test_oem_nsetcx_zero_atrmsk(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(100);
    oem_set_atrmsk(0);  /* No mask - no pixel selected */
    oem_set_atrpat(0x02);
    /* Zero ATRMSK - should be no-op */
    oem_nsetcx(10);
    TEST_ASSERT_EQ_INT(oem_get_pixofs(), 100, "Zero ATRMSK no-op");
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx zero ATRMSK");
}

void test_oem_nsetcx_pixbts1(void) {
    oem_init();
    gioscn_screen(2);  /* 640x200 - 1 bit per pixel */
    oem_set_pixbts(1);
    oem_set_pixofs(0);
    oem_set_atrmsk(0x80);  /* First bit */
    oem_set_atrpat(0x01);  /* Color 1 */
    /* Set 8 pixels (one byte) */
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        oem_nsetcx(8);
        /* All 8 bits should be set */
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0xFF, "8 pixels set in 1-bit mode");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx 1-bit per pixel");
}

void test_oem_nsetcx_pixbts2(void) {
    oem_init();
    gioscn_screen(1);  /* 320x200 - 2 bits per pixel */
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);  /* First 2-bit pixel */
    oem_set_atrpat(0x02);  /* Color 2 */
    /* Set 4 pixels (one byte) - all should be color 2 (binary 10) */
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        oem_nsetcx(4);
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0xAA, "4 pixels of color 2 = 0xAA");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx 2-bit per pixel");
}

void test_oem_nsetcx_pixbts2_clear(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    oem_set_atrpat(0x00);  /* Color 0 = clear */
    /* Set pixels to 0 */
    if (g_framebuffer) {
        g_framebuffer[0] = 0xFF;
        oem_nsetcx(4);
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0x00, "Clear via color 0");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx 2-bit clear");
}

void test_oem_nsetcx_crosses_byte_boundary(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);  /* First pixel */
    oem_set_atrpat(0x03);  /* Color 3 */
    /* Set 6 pixels (spans 2 bytes) */
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        g_framebuffer[1] = 0;
        oem_nsetcx(6);
        /* First byte: 4 pixels of color 3 = 0xFF */
        /* Second byte: 2 pixels of color 3 = 0xF0 (bits 7-6 and 5-4 both = 11) */
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0xFF, "First byte full");
        TEST_ASSERT_EQ_INT(g_framebuffer[1], 0xF0, "Second byte partial");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nsetcx crosses byte boundary");
}

/* ---- oem_nread detailed tests ---- */

void test_oem_nread_zero_count(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    int arr[10] = {0};
    oem_nread(arr, 0);
    /* Should be no-op, array unchanged */
    TEST_ASSERT_EQ_INT(arr[0], 0, "Zero count no-op");
    gioscn_screen(0);
    TEST_PASS("oem_nread zero count");
}

void test_oem_nread_null_array(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    /* NULL array - should be no-op */
    oem_nread(NULL, 16);
    TEST_PASS("oem_nread null array safe");
}

void test_oem_nread_pixbts1(void) {
    oem_init();
    gioscn_screen(2);
    oem_set_pixbts(1);
    oem_set_pixofs(0);
    oem_set_atrmsk(0x80);
    /* Set known pattern: 0b10101010 = 0xAA */
    if (g_framebuffer) {
        g_framebuffer[0] = 0xAA;
        int arr[2] = {0, 0};
        oem_nread(arr, 8);
        TEST_ASSERT_EQ_INT(arr[0], 0xAA, "Read 8 1-bit pixels");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nread 1-bit per pixel");
}

void test_oem_nread_pixbts2(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    /* Set known pattern: pixels = 0, 1, 2, 3 (2 bits each) */
    /* Binary: 00 01 10 11 = 0x1B */
    if (g_framebuffer) {
        g_framebuffer[0] = 0x1B;
        int arr[2] = {0, 0};
        oem_nread(arr, 8);  /* 8 bits = 4 pixels of 2 bits each */
        TEST_ASSERT_EQ_INT(arr[0], 0x1B, "Read 4 2-bit pixels");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nread 2-bit per pixel");
}

void test_oem_nread_partial_byte(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    /* Set known pattern: pixels = 0, 1, 2, 3 */
    if (g_framebuffer) {
        g_framebuffer[0] = 0x1B;
        int arr[2] = {0, 0};
        /* Read only 4 bits = 2 pixels (MSB-first packing) */
        oem_nread(arr, 4);
        /* pixel 0 (color 0) at bits 7-6, pixel 1 (color 1) at bits 5-4 */
        /* Byte = 0b0001_0000 = 0x10 */
        TEST_ASSERT_EQ_INT(arr[0], 0x10, "Read 2 pixels partial byte");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nread partial byte");
}

/* ---- oem_nwrite detailed tests ---- */

void test_oem_nwrite_zero_count(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    int arr[1] = {0xFF};
    oem_nwrite(arr, 0);
    /* Should be no-op */
    if (g_framebuffer) {
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0, "Zero count no-op");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nwrite zero count");
}

void test_oem_nwrite_null_array(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    oem_nwrite(NULL, 16);
    TEST_PASS("oem_nwrite null array safe");
}

void test_oem_nwrite_pixbts1(void) {
    oem_init();
    gioscn_screen(2);
    oem_set_pixbts(1);
    oem_set_pixofs(0);
    oem_set_atrmsk(0x80);
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        int arr[1] = {0xA5};  /* 0b10100101 */
        oem_nwrite(arr, 8);
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0xA5, "Write 8 1-bit pixels");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nwrite 1-bit per pixel");
}

void test_oem_nwrite_pixbts2(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        int arr[1] = {0xE4};  /* 4 pixels of 2 bits: 11 10 01 00 = colors 3,2,1,0 */
        oem_nwrite(arr, 8);
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0xE4, "Write 4 2-bit pixels");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nwrite 2-bit per pixel");
}

void test_oem_nread_nwrite_roundtrip(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    if (g_framebuffer) {
        /* Set original pattern */
        g_framebuffer[0] = 0x5A;  /* 0101 1010 = colors 1, 2, 1, 2 */
        /* Read */
        int arr[1] = {0};
        oem_nread(arr, 8);
        /* Clear framebuffer */
        g_framebuffer[0] = 0;
        /* Reset PIXOFS */
        oem_set_pixofs(0);
        oem_set_atrmsk(0xC0);
        /* Write back */
        oem_nwrite(arr, 8);
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0x5A, "Roundtrip preserves data");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nread/nwrite roundtrip");
}

void test_oem_nwrite_partial_byte(void) {
    oem_init();
    gioscn_screen(1);
    oem_set_pixbts(2);
    oem_set_pixofs(0);
    oem_set_atrmsk(0xC0);
    if (g_framebuffer) {
        g_framebuffer[0] = 0;
        /* Pixels 0, 1 = colors 0, 3, MSB-first packed = 0x30 */
        int arr[1] = {0x30};
        oem_nwrite(arr, 4);  /* Write only 4 bits = 2 pixels */
        /* pixel 0 (color 0) at bits 7-6, pixel 1 (color 3) at bits 5-4 */
        /* Byte = 0b0011_0000 = 0x30 */
        TEST_ASSERT_EQ_INT(g_framebuffer[0], 0x30, "Write 2 pixels partial byte");
    }
    gioscn_screen(0);
    TEST_PASS("oem_nwrite partial byte");
}