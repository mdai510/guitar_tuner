/*
 * button.c
 *
 *  Created on: Jul 12, 2026
 *      Author: maxda
 */

#include <stdbool.h>
#include "main.h"

#define BUTTON_DEBOUNCE_MS 20U

//volatile, since modified by EXTI interrupt
static volatile uint32_t b1_edge_time = 0U;
static volatile bool b1_debounce_pending = false;

static volatile uint32_t b2_edge_time = 0U;
static volatile bool b2_debounce_pending = false;

static volatile uint32_t b3_edge_time = 0U;
static volatile bool b3_debounce_pending = false;

//button 1 is set to pull up
bool b1_pressed_debounced(void){
	if(!b1_debounce_pending) return false;

	if((HAL_GetTick() - b1_edge_time) < BUTTON_DEBOUNCE_MS) return false;

	b1_debounce_pending = false;
	//if after debounce time, button is still low, register button press
	if(HAL_GPIO_ReadPin(Button1_GPIO_Port, Button1_Pin) == GPIO_PIN_RESET){
		return true;
	}
	return false;
}

bool b2_pressed_debounced(void){
	if(!b2_debounce_pending) return false;

	if((HAL_GetTick() - b2_edge_time) < BUTTON_DEBOUNCE_MS) return false;

	b2_debounce_pending = false;
	if(HAL_GPIO_ReadPin(Button2_GPIO_Port, Button2_Pin) == GPIO_PIN_RESET){
		return true;
	}
	return false;
}

bool b3_pressed_debounced(void){
	if(!b3_debounce_pending) return false;

	if((HAL_GetTick() - b3_edge_time) < BUTTON_DEBOUNCE_MS) return false;

	b3_debounce_pending = false;
	if(HAL_GPIO_ReadPin(Button3_GPIO_Port, Button3_Pin) == GPIO_PIN_RESET){
		return true;
	}
	return false;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == Button1_Pin){
		b1_edge_time = HAL_GetTick();
		b1_debounce_pending = true;
	}
	else if(GPIO_Pin == Button2_Pin){
		b2_edge_time = HAL_GetTick();
		b2_debounce_pending = true;
	}
	else if(GPIO_Pin == Button3_Pin){
		b3_edge_time = HAL_GetTick();
		b3_debounce_pending = true;
	}
}
