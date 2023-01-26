/*******************************************************************************
 * Size: 19 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#include "lvgl.h"

#ifndef ICONS_19
#define ICONS_19 1
#endif

#if ICONS_19

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F028 "" */
    0x0, 0x0, 0xc0, 0x0, 0x3, 0x80, 0xc, 0x7,
    0x0, 0xf1, 0x8c, 0x7, 0xc3, 0x1b, 0xff, 0x6,
    0x6f, 0xfc, 0xdc, 0xff, 0xf3, 0xb3, 0xff, 0xc6,
    0xcf, 0xff, 0x1b, 0x3f, 0xfc, 0xec, 0xff, 0xf3,
    0x63, 0x7, 0xc3, 0x98, 0xf, 0x1c, 0x60, 0xc,
    0x63, 0x0, 0x0, 0x1c, 0x0, 0x0, 0xe0, 0x0,
    0x2, 0x0,

    /* U+F0DC "" */
    0x6, 0x1, 0xe0, 0x7e, 0x1f, 0xe7, 0xfe, 0xff,
    0xc0, 0x0, 0x0, 0x7f, 0xef, 0xfc, 0xff, 0xf,
    0xc0, 0xf0, 0xc, 0x0,

    /* U+F185 "" */
    0x0, 0x40, 0x0, 0x1c, 0x0, 0x3, 0x80, 0xe,
    0xfb, 0x81, 0xff, 0xf0, 0x3c, 0x1e, 0x3, 0x39,
    0x80, 0xef, 0xb8, 0x7b, 0xfb, 0xdf, 0x7f, 0x7d,
    0xef, 0xef, 0xc, 0xf9, 0x80, 0xce, 0x60, 0x18,
    0xc, 0x7, 0xc7, 0xc0, 0xff, 0xf8, 0x1c, 0xe7,
    0x0, 0x1c, 0x0, 0x3, 0x80, 0x0, 0x20, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 342, .box_w = 22, .box_h = 18, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 50, .adv_w = 190, .box_w = 11, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 304, .box_w = 19, .box_h = 20, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xb4, 0x15d
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 61480, .range_length = 350, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t icons_19 = {
#else
lv_font_t icons_19 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -7,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if ICONS_19*/

