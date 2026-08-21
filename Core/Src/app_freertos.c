/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button.h"
#include "microphone.h"
#include "pitch.h"
#include "note.h"
#include "motor_cntrl.h"
#include "lcd.h"

#include <String.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum{
	STATE_TUNING_SELECT,
	STATE_LISTEN,
	STATE_ADJUST,
	STATE_DONE
} state_t;

typedef enum {
  UI_DIRTY_NONE = 0,
  UI_DIRTY_FULL = (1 << 0),
  UI_DIRTY_TUNING = (1 << 1),
  UI_DIRTY_NOTE = (1 << 2),
  UI_DIRTY_PITCH = (1 << 3)
} ui_dirty_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t tuning_idx = 0;
static note_t chosen_tuning[6] = {};

//UI control vars
static uint32_t ui_state = UI_DIRTY_FULL;

/* USER CODE END Variables */
/* Definitions for ControlTask */
osThreadId_t ControlTaskHandle;
const osThreadAttr_t ControlTask_attributes = {
  .name = "ControlTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};
/* Definitions for AudioTask */
osThreadId_t AudioTaskHandle;
const osThreadAttr_t AudioTask_attributes = {
  .name = "AudioTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static bool ui_draw_tuning_selection(uint16_t tuning_idx, bool full_redraw);
/* USER CODE END FunctionPrototypes */

void vTaskControl(void *argument);
extern void vTaskAudio(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ControlTask */
  ControlTaskHandle = osThreadNew(vTaskControl, NULL, &ControlTask_attributes);

  /* creation of AudioTask */
  AudioTaskHandle = osThreadNew(vTaskAudio, NULL, &AudioTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_vTaskControl */
/**
  * @brief  Function implementing the ControlTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_vTaskControl */
void vTaskControl(void *argument)
{
  /* USER CODE BEGIN vTaskControl */
	state_t state = STATE_TUNING_SELECT;

	//initialize
	if (!lcd_init()){
	    Error_Handler();
	}

	if (!lcd_clear()){
	    Error_Handler();
	}

//	if(!lcd_draw_text(110U, 80U, "starting...", &Atkinson32, LCD_COLOR_WHITE, LCD_COLOR_BLACK)) printf("starting print failed\r\n");
//	/*
//	if (!lcd_fill_rect(20U, 20U, 100U, 50U, LCD_COLOR_RED)) {
//	    Error_Handler();
//	}*/
//	if(!lcd_draw_text(10U, 10U, "Standard", &Atkinson32, LCD_COLOR_WHITE, LCD_COLOR_BLACK)) printf("print 32 failed \r\n");
//	if(!lcd_draw_text(10U, 60U, "48 Standard b #", &Atkinson48, LCD_COLOR_YELLOW, LCD_COLOR_BLACK)) printf("print 48 failed \r\n");
//	if(!lcd_draw_text(130U, 130U, "A  B  C#", &Atkinson72, LCD_COLOR_GREEN, LCD_COLOR_BLACK)) printf("print 72 failed \r\n");

	microphone_init(&hadc1);
	if(!fft_init()){
		printf("FFT Initialization failed\r\n");
		//stop task if pitch detection isn't working
		vTaskDelete(NULL);
	}
	printf("initialization done\r\n");

  /* Infinite loop */
  for(;;)
  {
	  switch(state){
	  case STATE_TUNING_SELECT:
		  //choose tuning here
		  if(b2_pressed_debounced()){
			  printf("left button pressed\r\n");
        if(tuning_idx == 0) tuning_idx = NUM_TUNINGS - 1;
        else tuning_idx--;
        ui_state |= UI_DIRTY_TUNING;
		  }
		  if(b3_pressed_debounced()){
			  printf("right button pressed\r\n");
        tuning_idx = (tuning_idx + 1U) % NUM_TUNINGS;
        ui_state |= UI_DIRTY_TUNING;
		  }
		  if(b1_pressed_debounced()){
			  printf("select button pressed\r\n");
        for(int i = 0; i < 6; i++){
          chosen_tuning[i] = tunings[tuning_idx].notes[i];
        }
			  microphone_start();
			  printf("state listen\r\n");
        vTaskResume(AudioTaskHandle);
			  state = STATE_LISTEN;
        ui_state |= UI_DIRTY_FULL;
		  }
		  break;
	  case STATE_LISTEN:
		  break;
	  case STATE_ADJUST:
		  break;
	  case STATE_DONE:
		  break;
    default:
      break;
	  }

    //render UI after processing state 
    if(ui_state != UI_DIRTY_NONE) {
      bool draw_ok = true;
      switch(state){
        case STATE_TUNING_SELECT:
          draw_ok = ui_draw_tuning_selection(tuning_idx, ui_state & UI_DIRTY_FULL);
          break;
      }
      if(draw_ok) ui_state = UI_DIRTY_NONE;
      else printf("UI draw failed\r\n");
    }
  }
  /* USER CODE END vTaskControl */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static bool ui_draw_tuning_selection(uint16_t tuning_idx, bool full_redraw){
	if(full_redraw){
		if(!lcd_clear()) return false;
		//draw unchanging UI items for this screen
		if(!lcd_draw_text(10U, 10U, "<", &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
		if(!lcd_draw_text(65U, 10U, "Select Tuning", &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
		if(!lcd_draw_text(LCD_WIDTH - 20U, 10U, ">", &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
		printf("full redrew\r\n");
	}
	//erase the tuning name and notes and replace
	if(!lcd_fill_rect(20U, 50U, LCD_WIDTH - 60U, LCD_HEIGHT - 60U, LCD_BG_COLOR)) return false;
	//printf("cleared\r\n");
	if(!lcd_draw_text(50U, 50U, tunings[tuning_idx].tuning_name, &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
	//printf("tuning drew\r\n");
	char notes_text[64] = ""; // buffer to hold the notes text, initialize to empty string
	for(int i = 0; i < 6; i++){
		strcat(notes_text, tunings[tuning_idx].notes[i].note_name);
		if(i < 5) strcat(notes_text, " ");
	}
	if(!lcd_draw_text(20U, 90U, notes_text, &Atkinson32, LCD_COLOR_WHITE, LCD_BG_COLOR)) return false;
	return true;

}
/* USER CODE END Application */

