#include "fonts.h"

/*
 * Generated from AtkinsonHyperlegible-Regular.ttf at 15px.
 * Characters: #
 * Bitmap format: 1 bpp, MSB first, each row byte-aligned.
 */

static const uint8_t Atkinson15_bitmap[] = {
    0x1B, 0x00, 0x1B, 0x00, 0x12, 0x00, 0x7F, 0x80, 0x12, 0x00, 0x12, 0x00,
    0x12, 0x00, 0x7F, 0x80, 0x36, 0x00, 0x26, 0x00, 0x24, 0x00,
};

static const lcd_glyph_t Atkinson15_glyphs[] = {
    {0x0023U, 0U, 10U, 11U, 0, -11, 10U}, /* # */
};

const lcd_font_t Atkinson15 = {
    .bitmap = Atkinson15_bitmap,
    .glyphs = Atkinson15_glyphs,
    .glyph_count = 1U,
    .line_height = 20U,
    .ascent = 15U,
};
