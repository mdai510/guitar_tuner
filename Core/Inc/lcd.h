/*
 * lcd.h
 *
 *  Created on: Aug 13, 2026
 *      Author: maxda
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdbool.h>
#include <stdint.h>
#include "fonts.h"

//For 240x320 TFT screen

#define LCD_WIDTH   320U
#define LCD_HEIGHT  240U

#define LCD_COLOR_BLACK    0x0000U
#define LCD_COLOR_WHITE    0xFFFFU
#define LCD_COLOR_RED      0xF800U
#define LCD_COLOR_GREEN    0x07E0U
#define LCD_COLOR_BLUE     0x001FU
#define LCD_COLOR_YELLOW   0xFFE0U
#define LCD_COLOR_CYAN     0x07FFU
#define LCD_COLOR_MAGENTA  0xF81FU

#define LCD_BG_COLOR LCD_COLOR_BLACK

bool lcd_init(void);

bool lcd_clear(void);

bool lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

bool lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

bool lcd_draw_codepoint(uint16_t x,
                        uint16_t y,
                        uint32_t codepoint,
                        const lcd_font_t *font,
                        uint16_t foreground,
                        uint16_t background);

bool lcd_draw_text(uint16_t x,
                   uint16_t y,
                   const char *text,
                   const lcd_font_t *font,
                   uint16_t foreground,
                   uint16_t background);

#endif /* INC_LCD_H_ */
