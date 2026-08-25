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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  STATE_TUNING_SELECT,
  STATE_LISTEN,
  STATE_ADJUST,
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static bool ui_draw_tuning_selection(uint16_t selected_tuning, bool full_redraw);
static bool ui_draw_listen_adjust_screen(uint8_t string, bool full_redraw);
static bool ui_draw_done_screen(void);
static bool string_to_array_index(uint8_t string, uint8_t *array_index);
static uint16_t ui_text_width(const char *text, const lcd_font_t *font);

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
  MX_ADC1_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  if (!lcd_init()) {
    Error_Handler();
  }

  if (!lcd_clear()) {
    Error_Handler();
  }

  microphone_init(&hadc1);

  if (!fft_init()) {
    Error_Handler();
  }

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

  while (1)  {
    switch (state) {
      case STATE_TUNING_SELECT:
        if (b2_pressed_debounced()) {
          if (tuning_idx == 0U) {
            tuning_idx = NUM_TUNINGS - 1U;
          }
          else {
            tuning_idx--;
          }

          ui_state |= UI_DIRTY_TUNING;
        }
        else if (b3_pressed_debounced()) {
          tuning_idx = (tuning_idx + 1U) % NUM_TUNINGS;
          ui_state |= UI_DIRTY_TUNING;
        }
        else if (b1_pressed_debounced()) {
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
          if (audio_result.frequency_valid) {
            printf("Frequency: %.2f Hz\r\n", audio_result.frequency_hz);
          }

          if (audio_result.stable_frequency) {
            printf("string %u: %.2f Hz\r\n",
                   (unsigned int)current_string,
                   audio_result.frequency_hz);

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
        }
        break;

      case STATE_ADJUST:
        /* Motor adjustment will be added here. */
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
          if ((ui_state & (UI_DIRTY_FULL | UI_DIRTY_STRING)) != 0U) {
            draw_ok = ui_draw_listen_adjust_screen(
                current_string,
                (ui_state & UI_DIRTY_FULL) != 0U);
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

    /* TIM7 wakes the CPU every millisecond for HAL_GetTick(). */
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
static bool ui_draw_tuning_selection(uint16_t selected_tuning, bool full_redraw)
{
  if (selected_tuning >= NUM_TUNINGS) {
    return false;
  }

  if (full_redraw) {
    if (!lcd_clear()) return false;
    if (!lcd_draw_text(10U, 10U, "<", &Atkinson32,
                       LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
    if (!lcd_draw_text(65U, 10U, "Select Tuning", &Atkinson32,
                       LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
    if (!lcd_draw_text(LCD_WIDTH - 20U, 10U, ">", &Atkinson32,
                       LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
  }

  if (!lcd_fill_rect(20U, 50U, LCD_WIDTH - 60U,
                     LCD_HEIGHT - 60U, LCD_BG_COLOR)) return false;

  uint16_t tuning_width =
      ui_text_width(tunings[selected_tuning].tuning_name, &Atkinson32);

  if (!lcd_draw_text((LCD_WIDTH - tuning_width) / 2U, 70U,
                     tunings[selected_tuning].tuning_name,
                     &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;

  char notes_text[64] = "";

  for (uint8_t i = 0U; i < 6U; i++) {
    strcat(notes_text, tunings[selected_tuning].notes[i].note_name);

    if (i < 5U) {
      strcat(notes_text, " ");
    }
  }

  uint16_t notes_width = ui_text_width(notes_text, &Atkinson32);

  return lcd_draw_text((LCD_WIDTH - notes_width) / 2U, 110U,
                       notes_text, &Atkinson32,
                       LCD_COLOR_WHITE, LCD_BG_COLOR);
}

static bool ui_draw_listen_adjust_screen(uint8_t string, bool full_redraw)
{
  uint8_t array_index;

  if (!string_to_array_index(string, &array_index)) {
    return false;
  }

  const note_t *note = &chosen_tuning.notes[array_index];
  char frequency_text[16];
  char note_name[3] = {0};
  char octave[2] = {0};
  size_t note_name_length = strlen(note->note_name);

  if ((note_name_length < 2U) || (note_name_length > 3U)) {
    return false;
  }

  memcpy(note_name, note->note_name, note_name_length - 1U);
  octave[0] = note->note_name[note_name_length - 1U];

  if (snprintf(frequency_text, sizeof(frequency_text),
               "%.2f Hz", note->frequency) < 0) return false;

  if (full_redraw) {
    if (!lcd_clear()) return false;
  }
  else {
    if (!lcd_fill_rect(0U, 0U, LCD_WIDTH, 60U,
                       LCD_BG_COLOR)) return false;
    if (!lcd_fill_rect(0U, 70U, LCD_WIDTH,
                       LCD_HEIGHT - 70U, LCD_BG_COLOR)) return false;
  }

  uint16_t frequency_width = ui_text_width(frequency_text, &Atkinson32);
  uint16_t note_width = ui_text_width(note_name, &Atkinson72);
  uint16_t octave_width = ui_text_width(octave, &Atkinson48);
  uint16_t note_x = (LCD_WIDTH - note_width - octave_width) / 2U;
  uint16_t note_y = (LCD_HEIGHT - Atkinson72.line_height) / 2U;
  uint16_t octave_y = note_y + Atkinson72.ascent - Atkinson48.ascent;

  if (!lcd_draw_text((LCD_WIDTH - frequency_width) / 2U, 8U,
                     frequency_text, &Atkinson32,
                     LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
  if (!lcd_draw_text(note_x, note_y, note_name, &Atkinson72,
                     LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;

  return lcd_draw_text(note_x + note_width, octave_y, octave,
                       &Atkinson48, LCD_COLOR_WHITE, LCD_BG_COLOR);
}

static bool ui_draw_done_screen(void)
{
  if (!lcd_clear()) {
    return false;
  }

  uint16_t title_width = ui_text_width("Tuning Complete", &Atkinson32);
  uint16_t prompt_width = ui_text_width("Press Select", &Atkinson32);

  if (!lcd_draw_text((LCD_WIDTH - title_width) / 2U, 70U,
                     "Tuning Complete", &Atkinson32,
                     LCD_COLOR_GREEN, LCD_BG_COLOR)) return false;

  return lcd_draw_text((LCD_WIDTH - prompt_width) / 2U, 120U,
                       "Press Select", &Atkinson32,
                       LCD_COLOR_WHITE, LCD_BG_COLOR);
}

static bool string_to_array_index(uint8_t string, uint8_t *array_index){
	if ((array_index == NULL) || (string == 0U) || (string > 6U)){
		return false;
	}

	*array_index = 6U - string;
	return true;
}

static uint16_t ui_text_width(const char *text, const lcd_font_t *font)
{
  uint16_t width = 0U;

  if ((text == NULL) || (font == NULL)) {
    return 0U;
  }

  for (; *text != '\0'; text++) {
    for (uint16_t glyph_idx = 0U;
         glyph_idx < font->glyph_count;
         glyph_idx++) {
      if (font->glyphs[glyph_idx].codepoint == (uint8_t)*text) {
        width += font->glyphs[glyph_idx].advance;
        break;
      }
    }
  }

  return width;
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
