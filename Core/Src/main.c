/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audio_processing.h"
#include "button.h"
#include "lcd.h"
#include "microphone.h"
#include "motor_cntrl.h"
#include "note.h"
#include "pitch.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  STATE_TUNING_SELECT,
  STATE_LISTEN,
  STATE_ADJUST,
  STATE_SETTLE,
  STATE_DONE
} state_t;

typedef enum {
  UI_DIRTY_NONE = 0U,
  UI_DIRTY_FULL = (1UL << 0),
  UI_DIRTY_TUNING = (1UL << 1),
  UI_DIRTY_STRING = (1UL << 2),
  UI_DIRTY_PITCH = (1UL << 3)
} ui_dirty_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FREQ_ALLOWED_DELTA_HZ 1.0f

#define MOTOR_SETTLE_TIME_MS 150U
#define TUNING_TOLERANCE_CENTS 5.0f

#define MOTOR_DIR_TIGHTEN 0U
#define MOTOR_DIR_LOOSEN 1U

#define NOTE_ROW_GAP_PX 10U
#define UI_CENTER_MARGIN_PX 8U
#define SHARP_OVERLAP_PX 2U
#define SHARP_LOWER_PX 6U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */
static uint8_t tuning_idx = 0U;
static tuning_t chosen_tuning;
static uint8_t current_string = 6U;
static uint32_t ui_state = UI_DIRTY_FULL; 
static uint32_t last_ui_update = 0U;

static uint32_t motor_settle_start = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint32_t choose_motor_steps(float absolute_error_cents);
static float frequency_error_cents(float measured_hz, float target_hz);
static bool ui_draw_tuning_selection(uint16_t selected_tuning, bool full_redraw);
static bool ui_draw_listen_adjust_screen(uint8_t string, bool full_redraw);
static bool ui_draw_done_screen(void);
static uint8_t string_to_array_index(uint8_t string);
static uint16_t ui_text_width(const char *text, const lcd_font_t *font);
static void ui_note_letter(uint8_t array_index, char *out);
static uint16_t ui_tuning_color_for_string(uint8_t string, uint8_t current_string);
static void ui_layout_tuning_notes(void);
static bool ui_draw_tuning_note(uint8_t array_index, uint8_t current_string);
static uint16_t ui_centered_x(uint16_t text_width);
static uint16_t ui_glyph_advance(const lcd_font_t *font, uint8_t codepoint);
static uint16_t ui_note_name_width(const char *text, const lcd_font_t *font, bool use_tiny_sharp);
static bool ui_draw_note_name(uint16_t x, uint16_t y, const char *text, const lcd_font_t *font, uint16_t color, bool use_tiny_sharp);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  state_t state = STATE_TUNING_SELECT;
  audio_processing_result_t audio_result = {0};

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  if (!lcd_init()) {
    Error_Handler();
  }

  if (!lcd_clear()) {
    Error_Handler();
  }

  if(!microphone_init(&hadc1)) {
    Error_Handler();
  }

  if (!fft_init()) {
    Error_Handler();
  }

  motor_cntrl_init();

  audio_processing_reset();

  /* USER CODE END 2 */

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1){
    switch (state){
      case STATE_TUNING_SELECT:
        if (b2_pressed_debounced()){
          if (tuning_idx == 0U) tuning_idx = NUM_TUNINGS - 1U;
          else tuning_idx--;
          ui_state |= UI_DIRTY_TUNING;
        }
        else if (b3_pressed_debounced()){
          tuning_idx = (tuning_idx + 1U) % NUM_TUNINGS;
          ui_state |= UI_DIRTY_TUNING;
        }
        else if (b1_pressed_debounced()){
          chosen_tuning = tunings[tuning_idx];
          current_string = 6U;
          audio_processing_reset();
          microphone_start();
          state = STATE_LISTEN;
          ui_state = UI_DIRTY_FULL;
        }
        break;

      case STATE_LISTEN:
    	  if (audio_process_pending(current_string, &audio_result)) {
			  if (audio_result.frequency_valid) printf("Frequency: %.2f Hz\r\n", audio_result.frequency_hz);
			  if (audio_result.stable_frequency) {
				  printf("string %u detected: %.2f Hz\r\n", (unsigned int)current_string, audio_result.frequency_hz);

				  float error_cents = frequency_error_cents(audio_result.frequency_hz, chosen_tuning.notes[string_to_array_index(current_string)].frequency);

				  //if frequency is close enough to the target, move to the next string
				  if (fabsf(error_cents) <= TUNING_TOLERANCE_CENTS) {
					  printf("String %u in tune: %.2f Hz\r\n", (unsigned int)current_string, audio_result.frequency_hz);
					  if (current_string <= 1U) {
						  microphone_stop();
						  state = STATE_DONE;
						  ui_state = UI_DIRTY_FULL;
					  }
					  else {
						  current_string--;
						  audio_processing_reset();
						  ui_state |= UI_DIRTY_STRING;
					  }
				  }
				  //else record the current frequency and then adjust based on it
				  else{
					  printf("String %u out of tune. Cents of error: %.2f\r\n", (unsigned int)current_string, error_cents);
					  uint8_t direction = (error_cents < 0.0f) ? MOTOR_DIR_TIGHTEN : MOTOR_DIR_LOOSEN;
					  microphone_stop();
					  audio_processing_reset();
					  uint32_t steps = choose_motor_steps(fabsf(error_cents));
					  if(motor_cntrl_move_steps(direction, steps)){
						  state = STATE_ADJUST;
					  }
					  else{
						  //failed to start motor so resume listening
						  microphone_start();
					  }
					  //UI may or may not change (decide later)
				  }
			  }
        }
        break;

      case STATE_ADJUST:
        if(!motor_cntrl_is_busy()){
        	motor_settle_start = HAL_GetTick();
        	state = STATE_SETTLE;
        }
        break;

      case STATE_SETTLE:
    	  if((HAL_GetTick() - motor_settle_start) >= MOTOR_SETTLE_TIME_MS){
    		  audio_processing_reset();
    		  microphone_start();
    		  state = STATE_LISTEN;
    	  }
    	  break;

      case STATE_DONE:
        if (b1_pressed_debounced()) {
          microphone_stop();
          microphone_discard_pending();
          state = STATE_TUNING_SELECT;
          ui_state = UI_DIRTY_FULL;
        }
        break;

      default:
        microphone_stop();
        state = STATE_TUNING_SELECT;
        ui_state = UI_DIRTY_FULL;
        break;
    }

    if (ui_state != UI_DIRTY_NONE) {
      bool draw_ok = true;

      switch (state) {
        case STATE_TUNING_SELECT:
          draw_ok = ui_draw_tuning_selection(
              tuning_idx,
              (ui_state & UI_DIRTY_FULL) != 0U);
          break;

        case STATE_LISTEN:
            //if no new mic data and last 
            //if(!microphone_get_ready_flags() )
          if ((ui_state & (UI_DIRTY_FULL | UI_DIRTY_STRING)) != 0U) {
            draw_ok = ui_draw_listen_adjust_screen(
                current_string,
                (ui_state & UI_DIRTY_FULL) != 0U);
            last_ui_update = HAL_GetTick();
          }
          break;

        case STATE_DONE:
          if ((ui_state & UI_DIRTY_FULL) != 0U) {
            draw_ok = ui_draw_done_screen();
          }
          break;

        case STATE_ADJUST:
        default:
          break;
      }

      if (draw_ok) {
        ui_state = UI_DIRTY_NONE;
      }
      else {
        printf("UI draw failed\r\n");
      }
    }

    /* SysTick wakes the CPU every millisecond for HAL_GetTick(). */
    __WFI();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//temp adjustment steps for now
static uint32_t choose_motor_steps(float absolute_error_cents){
  if (absolute_error_cents > 50.0f) {
    return 40U;
  }

  if (absolute_error_cents > 20.0f) {
    return 20U;
  }

  if (absolute_error_cents > 8.0f) {
    return 8U;
  }

  return 3U;
}

static float frequency_error_cents(float measured_hz, float target_hz){
  return 1200.0f * log2f(measured_hz / target_hz);
} 

static bool ui_draw_tuning_selection(uint16_t selected_tuning, bool full_redraw){
	if (selected_tuning >= NUM_TUNINGS) return false;

	if (full_redraw) {
		if (!lcd_clear()) return false;
		if (!lcd_draw_text(10U, 10U, "<", &Atkinson32,
						   LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
		if (!lcd_draw_text(65U, 10U, "Select Tuning", &Atkinson32,
						   LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
		if (!lcd_draw_text(LCD_WIDTH - 20U, 10U, ">", &Atkinson32,
						   LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
	}

	/* Clear the full row width: a previous, wider tuning's text can extend past a narrower box. */
	if (!lcd_fill_rect(0U, 50U, LCD_WIDTH, LCD_HEIGHT - 60U, LCD_BG_COLOR)) return false;

	uint16_t tuning_width = ui_text_width(tunings[selected_tuning].tuning_name, &Atkinson32);

	if (!lcd_draw_text(ui_centered_x(tuning_width), 70U,
					 tunings[selected_tuning].tuning_name,
					 &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;

	const note_t *notes = tunings[selected_tuning].notes;
	uint16_t note_widths[6];
	uint16_t total_width = 0U;

	for (uint8_t i = 0U; i < 6U; i++) {
		note_widths[i] = ui_note_name_width(notes[i].note_name, &Atkinson28, true);
		total_width += note_widths[i];
		if (i < 5U) total_width += NOTE_ROW_GAP_PX;
	}

	uint16_t cursor_x = ui_centered_x(total_width);

	for (uint8_t i = 0U; i < 6U; i++) {
		if (!ui_draw_note_name(cursor_x, 110U, notes[i].note_name, &Atkinson28, LCD_COLOR_WHITE, true)) return false;
		cursor_x += note_widths[i] + NOTE_ROW_GAP_PX;
	}

	return true;
}

static bool ui_draw_listen_adjust_screen(uint8_t string, bool full_redraw){
	if ((string == 0U) || (string > 6U)) return false;

	uint8_t array_index = string_to_array_index(string);
	const note_t *note = &chosen_tuning.notes[array_index];
	char string_text[12];
	char frequency_text[16];
	char note_letter[2] = {0};
	char octave[2] = {0};
	size_t note_name_length = strlen(note->note_name);

	if ((note_name_length < 2U) || (note_name_length > 3U)) return false;

	/* A 3-char note name is letter + '#' + octave digit, e.g. "D#2". */
	bool has_sharp = (note_name_length == 3U);

	note_letter[0] = note->note_name[0];
	octave[0] = note->note_name[note_name_length - 1U];

	if (snprintf(string_text, sizeof(string_text), "String %u",
			   (unsigned int)string) < 0) return false;
	if (snprintf(frequency_text, sizeof(frequency_text),
			   "%.2f Hz", note->frequency) < 0) return false;

	if (full_redraw) {
		if (!lcd_clear()) return false;

		ui_layout_tuning_notes();

		for (uint8_t note_index = 0U; note_index < 6U; note_index++) {
			if (!ui_draw_tuning_note(note_index, string)) return false;
		}
	}
	else {
		if (!lcd_fill_rect(0U, 0U, LCD_WIDTH, 180U, LCD_BG_COLOR)) return false;

		/* Only the string that just finished and the newly current string change color. */
		if (string < 6U) {
			if (!ui_draw_tuning_note(string_to_array_index(string + 1U), string)) return false;
		}
		if (!ui_draw_tuning_note(array_index, string)) return false;
	}


	uint16_t string_width = ui_text_width(string_text, &Atkinson32);
	uint16_t frequency_width = ui_text_width(frequency_text, &Atkinson32);
	uint16_t note_width = ui_text_width(note_letter, &Atkinson72);

	/* Sharp and octave share the same font/baseline, so measure and draw them as one string. */
	char accidental_text[3] = {0};
	if (has_sharp) accidental_text[0] = '#';
	accidental_text[has_sharp ? 1U : 0U] = octave[0];

	uint16_t accidental_width = ui_text_width(accidental_text, &Atkinson32);
	uint16_t note_x = (LCD_WIDTH - note_width - accidental_width) / 2U;
	uint16_t note_y = 38U;
	uint16_t accidental_y = note_y + Atkinson72.ascent - Atkinson32.ascent;

	if (!lcd_draw_text((LCD_WIDTH - string_width) / 2U, 4U,
					 string_text, &Atkinson32,
					 LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
	if (!lcd_draw_text(note_x, note_y, note_letter, &Atkinson72,
					 LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
	if (!lcd_draw_text(note_x + note_width, accidental_y, accidental_text,
					 &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;

	return lcd_draw_text((LCD_WIDTH - frequency_width) / 2U, 128U,
					   frequency_text, &Atkinson32,
					   LCD_COLOR_WHITE, LCD_BG_COLOR);
}

static bool ui_draw_done_screen(void){
	if (!lcd_clear()) return false;

	uint16_t title_width = ui_text_width("Tuning Complete", &Atkinson32);
	uint16_t prompt_width = ui_text_width("Press Select", &Atkinson32);

	if (!lcd_draw_text((LCD_WIDTH - title_width) / 2U, 70U,
					 "Tuning Complete", &Atkinson32,
					 LCD_COLOR_GREEN, LCD_BG_COLOR)) return false;

	return lcd_draw_text((LCD_WIDTH - prompt_width) / 2U, 120U,
					   "Press Select", &Atkinson32,
					   LCD_COLOR_WHITE, LCD_BG_COLOR);
}

static uint8_t string_to_array_index(uint8_t string){
	return 6U - string;
}

/* x position of each note's colored letter in the listen screen's note row, indexed by array_index. */
static uint16_t tuning_note_x[6];

/* Copies a note's letter (name minus trailing octave digit) into out, which must hold >= 3 bytes. */
static void ui_note_letter(uint8_t array_index, char *out){
	const char *note_name = chosen_tuning.notes[array_index].note_name;
	size_t len = strlen(note_name);

	memcpy(out, note_name, len - 1U);
	out[len - 1U] = '\0';
}

static uint16_t ui_tuning_color_for_string(uint8_t string, uint8_t current_string){
	if (string > current_string) return LCD_COLOR_GREEN;
	if (string == current_string) return LCD_COLOR_WHITE;
	return LCD_COLOR_RED;
}

static void ui_layout_tuning_notes(void){
	char letters[6][3];
	uint16_t widths[6];
	uint16_t total_width = 0U;

	for (uint8_t i = 0U; i < 6U; i++) {
		ui_note_letter(i, letters[i]);
		widths[i] = ui_note_name_width(letters[i], &Atkinson32, false);
		total_width += widths[i];
		if (i < 5U) total_width += NOTE_ROW_GAP_PX;
	}

	uint16_t cursor_x = ui_centered_x(total_width);

	for (uint8_t i = 0U; i < 6U; i++) {
		tuning_note_x[i] = cursor_x;
		cursor_x += widths[i] + NOTE_ROW_GAP_PX;
	}
}

static bool ui_draw_tuning_note(uint8_t array_index, uint8_t current_string){
	char letter[3];
	uint8_t string = 6U - array_index;
	uint16_t color = ui_tuning_color_for_string(string, current_string);

	ui_note_letter(array_index, letter);

	return ui_draw_note_name(tuning_note_x[array_index], 180U, letter, &Atkinson32, color, false);
}

static uint16_t ui_text_width(const char *text, const lcd_font_t *font){
	uint16_t width = 0U;
	if ((text == NULL) || (font == NULL)) return 0U;

	for(; *text != '\0'; text++){
		width += ui_glyph_advance(font, (uint8_t)*text);
	}

	return width;
}

/* Returns 0 if the text fits centered normally, else clamps to a small margin instead of underflowing. */
static uint16_t ui_centered_x(uint16_t text_width){
	return (text_width + (2U * UI_CENTER_MARGIN_PX) < LCD_WIDTH)
		? (LCD_WIDTH - text_width) / 2U
		: UI_CENTER_MARGIN_PX;
}

static uint16_t ui_glyph_advance(const lcd_font_t *font, uint8_t codepoint){
	for (uint16_t glyph_idx = 0U; glyph_idx < font->glyph_count; glyph_idx++){
		if (font->glyphs[glyph_idx].codepoint == codepoint){
			return font->glyphs[glyph_idx].advance;
		}
	}

	return 0U;
}

/* '#' reserves less cursor space than its glyph width so it overlaps back into the preceding letter. */
static uint16_t ui_note_char_advance(const lcd_font_t *font, char c, bool use_tiny_sharp){
	if (use_tiny_sharp && (c == '#')){
		uint16_t advance = ui_glyph_advance(&Atkinson15, (uint8_t)c);
		return (advance > SHARP_OVERLAP_PX) ? (advance - SHARP_OVERLAP_PX) : 0U;
	}

	return ui_glyph_advance(font, (uint8_t)c);
}

static uint16_t ui_note_name_width(const char *text, const lcd_font_t *font, bool use_tiny_sharp){
	uint16_t width = 0U;
	if ((text == NULL) || (font == NULL)) return 0U;

	for (; *text != '\0'; text++){
		width += ui_note_char_advance(font, *text, use_tiny_sharp);
	}

	return width;
}

static bool ui_draw_note_name(uint16_t x, uint16_t y, const char *text, const lcd_font_t *font, uint16_t color, bool use_tiny_sharp){
	uint16_t cursor_x = x;
	if ((text == NULL) || (font == NULL)) return false;

	for (; *text != '\0'; text++){
		if (use_tiny_sharp && (*text == '#')) {
			uint16_t sharp_y = y + font->ascent - Atkinson15.ascent + SHARP_LOWER_PX;
			uint16_t sharp_x = (cursor_x > SHARP_OVERLAP_PX) ? (cursor_x - SHARP_OVERLAP_PX) : 0U;

			if (!lcd_draw_codepoint(sharp_x, sharp_y, (uint8_t)*text, &Atkinson15, color, LCD_BG_COLOR)) return false;
		}
		else {
			if (!lcd_draw_codepoint(cursor_x, y, (uint8_t)*text, font, color, LCD_BG_COLOR)) return false;
		}
		cursor_x += ui_note_char_advance(font, *text, use_tiny_sharp);
	}

	return true;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
