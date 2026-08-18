/*
 * lcd.c
 *
 *  Created on: Aug 13, 2026
 *      Author: maxda
 */

#include "lcd.h"
#include "spi.h"
#include "fonts.h"
#include <stdbool.h>

#define LCD_SPI_TIMEOUT_MS 100U

#define LCD_TRANSFER_PIXELS 64U

#define LCD_BG_COLOR LCD_COLOR_BLACK

static bool lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

/*
* Set LCD SPI CS Pin to low
*/
static void lcd_select(void){
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

/*
 * Set LCD SPI CS Pin to high
 */
static void lcd_deselect(void){
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

/*
* Set LCD D/C Pin to low to indicate command
*/
static void lcd_dc_command(void){
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
}

/*
* Set LCD D/C Pin to high to indicate data
*/
static void lcd_dc_data(void){
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
}

/*
 * Transmit SPI message to LCD
 * Requires CS and D/C to be set appropriately before calling
 * Needs uint8_t data POINTER
 */
static bool lcd_transmit(const uint8_t *data, uint16_t data_len){
	return HAL_SPI_Transmit(&hspi1, (uint8_t *)data, data_len, LCD_SPI_TIMEOUT_MS) == HAL_OK;
}

/*
 * Full LCD SPI write with command and data (if any)
 * Sets CS and D/C appropriately
 */
static bool lcd_write(uint8_t cmd_byte, const uint8_t *data_bytes, uint16_t data_len){
	if((data_bytes == NULL) && (data_len != 0U)) return false;

	lcd_select();
	lcd_dc_command();
	if(!lcd_transmit(&cmd_byte, 1)){
		lcd_deselect();
		return false;
	}
	
	//data bytes
	if(data_len != 0U){
		lcd_dc_data();
		if(!lcd_transmit(data_bytes, data_len)){
			lcd_deselect();
			return false;
		}
	}
	//CS High
	lcd_deselect();
	return true;
}

/*
* Reset the LCD by toggling the reset pin
*/
static void lcd_reset(void){
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_RESET);

	HAL_Delay(250);

	HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_SET);

	HAL_Delay(250);
}

/*
 * Initialize the LCD
 */
bool lcd_init(void){
	static const uint8_t madctl[] = {0xA0};	// Memory Data Access Control: row/column order, RGB/BGR order, refresh order
	//MADCTL: 0xA0 = Mirror Y, Exchange X/Y axes -> landscape orientation, RGB color order, refresh top to bottom
	static const uint8_t pixel_format[] = {0x55};  /* RGB565: 16 bits per pixel */
	static const uint8_t porch_control[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
	static const uint8_t gate_control[] = {0x35};
	static const uint8_t vcom_setting[] = {0x2B};
	static const uint8_t lcm_control[] = {0x2C};
	static const uint8_t vdv_vrh_enable[] = {0x01, 0xFF};
	static const uint8_t vrh_setting[] = {0x11};
	static const uint8_t vdv_setting[] = {0x20};
	static const uint8_t frame_rate[] = {0x0F};
	static const uint8_t power_control[] = {0xA4, 0xA1};
	static const uint8_t positive_gamma[] = {0xD0, 0x00, 0x05, 0x0E, 0x15,
												0x0D, 0x37, 0x43, 0x47, 0x09,
												0x15, 0x12, 0x16, 0x19};
	static const uint8_t negative_gamma[] = {0xD0, 0x00, 0x05, 0x0D, 0x0C,
												0x06, 0x2D, 0x44, 0x40, 0x0E,
												0x1C, 0x18, 0x16, 0x19};

    //reset
	lcd_reset();

	//factory initialization sequence from NewHaven data sheet
	if(!lcd_write(0x28, NULL, 0)) return false; //turn off display
	if(!lcd_write(0x11, NULL, 0)) return false; //exit sleep mode
	HAL_Delay(100);

	if(!lcd_write(0x36, madctl, sizeof(madctl))) return false; 
	if(!lcd_write(0x3A, pixel_format, sizeof(pixel_format))) return false; 
	if(!lcd_write(0xB2, porch_control, sizeof(porch_control))) return false;
	if(!lcd_write(0xB7, gate_control, sizeof(gate_control))) return false;
	if(!lcd_write(0xBB, vcom_setting, sizeof(vcom_setting))) return false;
	if(!lcd_write(0xC0, lcm_control, sizeof(lcm_control))) return false;
	if(!lcd_write(0xC2, vdv_vrh_enable, sizeof(vdv_vrh_enable))) return false;	
	if(!lcd_write(0xC3, vrh_setting, sizeof(vrh_setting))) return false;	
	if(!lcd_write(0xC4, vdv_setting, sizeof(vdv_setting))) return false;	
	if(!lcd_write(0xC6, frame_rate, sizeof(frame_rate))) return false;	
	if(!lcd_write(0xD0, power_control, sizeof(power_control))) return false;	
	if(!lcd_write(0xE0, positive_gamma, sizeof(positive_gamma))) return false;
	if(!lcd_write(0xE1, negative_gamma, sizeof(negative_gamma))) return false;

	if (!lcd_set_window(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U)) return false;

	HAL_Delay(10);

	if(!lcd_write(0x29, NULL, 0)) return false; /* Display ON */

	HAL_Delay(10);
	return true;
}

/*
 * Set the LCD window for pixel writes
 * x_start, y_start: top left corner of window
 * x_end, y_end: bottom right corner of window
 */
static bool lcd_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
  if ((x_start > x_end) || (y_start > y_end) || (x_end >= LCD_WIDTH) ||
      (y_end >= LCD_HEIGHT))
    return false;

  // XS[15:8], XS[7:0], XE[15:8], XE[7:0]
  uint8_t column_data[4] = {(uint8_t)(x_start >> 8), (uint8_t)(x_start),
                            (uint8_t)(x_end >> 8), (uint8_t)(x_end)};
  // YS[15:8], YS[7:0], YE[15:8], YE[7:0]
  uint8_t row_data[4] = {(uint8_t)(y_start >> 8), (uint8_t)(y_start),
                         (uint8_t)(y_end >> 8), (uint8_t)(y_end)};

  if (!lcd_write(0x2A, column_data, sizeof(column_data))) {
    return false;
  }

  if (!lcd_write(0x2B, row_data, sizeof(row_data))) {
    return false;
  }

  return true;
}

/*
* Begin writing pixel data to the LCD
* CS is LOW and D/C is set to DATA after this function returns successfully
* Must call lcd_end_pixel_write() when done
*/
static bool lcd_begin_pixel_write(void){
    const uint8_t memory_write_command = 0x2CU;
    lcd_select();

    /* Send RAMWR command. */
    lcd_dc_command();
    if (!lcd_transmit(&memory_write_command, 1U)) {
        lcd_deselect();
        return false;
    }

    /* Subsequent bytes are RGB565 pixel data. */
    lcd_dc_data();
    return true;
}

/*
* End pixel write to the LCD
* Must call lcd_begin_pixel_write() before this
*/
static void lcd_end_pixel_write(void){
    lcd_deselect();
}

/*
* Fill a rectangle on the LCD with a solid color
* Transaction is fully contained within this function (pixel write is started/ended)
*/
bool lcd_fill_rect(uint16_t x,
                   uint16_t y,
                   uint16_t width,
                   uint16_t height,
                   uint16_t color){
    uint8_t transfer_buffer[LCD_TRANSFER_PIXELS * 2U];

    if ((width == 0U) || (height == 0U)) return true;
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) return false;

    /* Clip against the display boundaries. */
    if (width > (LCD_WIDTH - x)) width = LCD_WIDTH - x;
    if (height > (LCD_HEIGHT - y)) height = LCD_HEIGHT - y;

    if (!lcd_set_window(x, y, x + width - 1U, y + height - 1U)) return false;

    /*
     * Create a reusable RGB565 block.
     * The display expects the high byte first.
     */
    for (uint16_t i = 0U; i < LCD_TRANSFER_PIXELS; i++){
        transfer_buffer[2U * i] = (uint8_t)(color >> 8);
        transfer_buffer[(2U * i) + 1U] = (uint8_t)color;
    }

    if (!lcd_begin_pixel_write()) return false;

    uint32_t pixels_remaining = (uint32_t)width * (uint32_t)height;

    while (pixels_remaining > 0U){
        uint16_t chunk_pixels;

        if (pixels_remaining > LCD_TRANSFER_PIXELS){
            chunk_pixels = LCD_TRANSFER_PIXELS;
        }
        else{
            chunk_pixels = (uint16_t)pixels_remaining;
        }

        if (!lcd_transmit(transfer_buffer, (uint16_t)(chunk_pixels * 2U))){
            lcd_end_pixel_write();
            return false;
        }
        pixels_remaining -= chunk_pixels;
    }

    lcd_end_pixel_write();
    return true;
}

/*
* Clear the entire LCD with a solid color
*/
bool lcd_clear(void){
    return lcd_fill_rect(0U, 0U, LCD_WIDTH, LCD_HEIGHT, LCD_BG_COLOR);
}

/*
* Draw a single pixel on the LCD at (x, y) with the specified color
*/
bool lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color){
    return lcd_fill_rect(x, y, 1U, 1U, color);
}

bool lcd_draw_rgb565_bitmap(uint16_t x,
                            uint16_t y,
                            uint16_t width,
                            uint16_t height,
                            const uint16_t *pixels){
    uint8_t transfer_buffer[LCD_TRANSFER_PIXELS * 2U];
    uint32_t total_pixels;
    uint32_t pixel_index = 0U;

    if ((pixels == NULL) ||
        (width == 0U) ||
        (height == 0U)) {
        return false;
    }

    if ((x >= LCD_WIDTH) ||
        (y >= LCD_HEIGHT) ||
        (width > (LCD_WIDTH - x)) ||
        (height > (LCD_HEIGHT - y))) {
        return false;
    }

    if (!lcd_set_window(
            x,
            y,
            x + width - 1U,
            y + height - 1U)) {
        return false;
    }

    if (!lcd_begin_pixel_write()) {
        return false;
    }

    total_pixels = (uint32_t)width * (uint32_t)height;

    while (pixel_index < total_pixels) {
        uint32_t remaining = total_pixels - pixel_index;

        uint16_t chunk_pixels =
            (remaining > LCD_TRANSFER_PIXELS)
                ? LCD_TRANSFER_PIXELS
                : (uint16_t)remaining;

        for (uint16_t i = 0U; i < chunk_pixels; i++) {
            uint16_t color = pixels[pixel_index + i];

            /* Convert native uint16_t to ST7789 byte order. */
            transfer_buffer[2U * i] =
                (uint8_t)(color >> 8);

            transfer_buffer[(2U * i) + 1U] =
                (uint8_t)color;
        }

        if (!lcd_transmit(
                transfer_buffer,
                (uint16_t)(chunk_pixels * 2U))) {
            lcd_end_pixel_write();
            return false;
        }

        pixel_index += chunk_pixels;
    }

    lcd_end_pixel_write();
    return true;
}

static const lcd_glyph_t *lcd_find_glyph(const lcd_font_t *font, uint32_t codepoint){
    if (font == NULL) return NULL;

    for (uint16_t i = 0U; i < font->glyph_count; i++){
        if (font->glyphs[i].codepoint == codepoint){
            return &font->glyphs[i];
        }
    }

    return NULL;
}

bool lcd_draw_codepoint(uint16_t x,
                        uint16_t y,
                        uint32_t codepoint,
                        const lcd_font_t *font,
                        uint16_t foreground,
                        uint16_t background){
    if (font == NULL) return false;

    const lcd_glyph_t *glyph = lcd_find_glyph(font, codepoint);
    if (glyph == NULL) return false;

    uint16_t cell_width = glyph->advance;
    uint16_t cell_height = font->line_height;

    if ((cell_width == 0U) || (cell_height == 0U)) return true;

    if ((x + cell_width > LCD_WIDTH) ||
        (y + cell_height > LCD_HEIGHT)) {
        return false;
    }

    if (!lcd_set_window(x, y, x + cell_width - 1U, y + cell_height - 1U)){
        return false;
    }

    if (!lcd_begin_pixel_write()) return false;

    uint8_t buffer[LCD_TRANSFER_PIXELS * 2U];
    uint16_t buffered_pixels = 0U;

    uint16_t glyph_row_bytes = (glyph->width + 7U) / 8U;

    /*
     * Glyph y_offset is relative to the baseline.
     * The baseline is font->ascent pixels below the top of the line.
     */
    int32_t glyph_x = glyph->x_offset;
    int32_t glyph_y = (int32_t)font->ascent + glyph->y_offset;

    for (uint16_t cell_y = 0U; cell_y < cell_height; cell_y++){
        for (uint16_t cell_x = 0U; cell_x < cell_width; cell_x++){
            int32_t bitmap_x = (int32_t)cell_x - glyph_x;
            int32_t bitmap_y = (int32_t)cell_y - glyph_y;

            bool pixel_set = false;

            if ((bitmap_x >= 0) &&
                (bitmap_y >= 0) &&
                (bitmap_x < glyph->width) &&
                (bitmap_y < glyph->height)){

                uint32_t byte_index =
                    glyph->bitmap_offset +
                    ((uint32_t)bitmap_y * glyph_row_bytes) +
                    ((uint32_t)bitmap_x / 8U);

                uint8_t bit_mask = (uint8_t)(0x80U >> ((uint32_t)bitmap_x % 8U));

                pixel_set = (font->bitmap[byte_index] & bit_mask) != 0U;
            }

            uint16_t color = pixel_set ? foreground : background;

            buffer[buffered_pixels * 2U] = (uint8_t)(color >> 8);

            buffer[(buffered_pixels * 2U) + 1U] = (uint8_t)color;

            buffered_pixels++;

            if (buffered_pixels == LCD_TRANSFER_PIXELS){
                if (!lcd_transmit(buffer, buffered_pixels * 2U)){
                    lcd_end_pixel_write();
                    return false;
                }

                buffered_pixels = 0U;
            }
        }
    }

    if (buffered_pixels != 0U){
        if (!lcd_transmit(buffer, buffered_pixels * 2U)){
            lcd_end_pixel_write();
            return false;
        }
    }

    lcd_end_pixel_write();
    return true;
}

bool lcd_draw_text(uint16_t x,
                   uint16_t y,
                   const char *text,
                   const lcd_font_t *font,
                   uint16_t foreground,
                   uint16_t background){
    if ((text == NULL) || (font == NULL)) return false;

    uint16_t cursor_x = x;

    while (*text != '\0'){
        uint32_t codepoint = (uint8_t)*text;

        const lcd_glyph_t *glyph = lcd_find_glyph(font, codepoint);

        if (glyph == NULL) return false;

        if (!lcd_draw_codepoint(cursor_x, y, codepoint, font, foreground, background)){
            return false;
        }

        cursor_x += glyph->advance;
        text++;
    }

    return true;
}
