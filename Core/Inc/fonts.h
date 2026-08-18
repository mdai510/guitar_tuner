/*
 * fonts.h
 *
 *  Created on: Aug 17, 2026
 *      Author: maxda
 */

#ifndef INC_FONTS_H_
#define INC_FONTS_H_

#include <stdint.h>

typedef struct {
    uint32_t codepoint;
    uint32_t bitmap_offset;

    uint16_t width;
    uint16_t height;

    int16_t x_offset;
    int16_t y_offset;

    uint16_t advance;
} lcd_glyph_t;

typedef struct {
    const uint8_t *bitmap;
    const lcd_glyph_t *glyphs;

    uint16_t glyph_count;
    uint16_t line_height;
    uint16_t ascent;
} lcd_font_t;

extern const lcd_font_t Atkinson32;
extern const lcd_font_t Atkinson48;
extern const lcd_font_t Atkinson72;

#endif /* INC_FONTS_H_ */
