/*
 * microphone.c
 *
 *  Created on: Jun 30, 2026
 *      Author: maxda
 */

#include "microphone.h"
#include "main.h"
#include "tim.h"
#include "adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

extern osThreadId_t AudioTaskHandle;

static ADC_HandleTypeDef *mic_hadc;
static uint16_t mic_adc_buf[MIC_BUF_SIZE];

/*
 * Initialize the microphone with the given ADC handle
 */
void microphone_init(ADC_HandleTypeDef *hadc){
	mic_hadc = hadc;
}

/*
 * Start the DMA transfer for microphone data
 */
void microphone_start(void){
	HAL_ADC_Start_DMA(mic_hadc, (uint32_t*)mic_adc_buf, MIC_BUF_SIZE);
	// Start 8kHz timer
	HAL_TIM_Base_Start(&htim6);
}

/*
 * Stop microphone data DMA transfers
 */
void microhone_stop(void){
	HAL_TIM_Base_Stop(&htim6);
	HAL_ADC_Stop_DMA(mic_hadc);
}

/*
 * Get mic buffer pointer
 */
const uint16_t* microphone_get_buffer(void){
	return mic_adc_buf;
}

/*
 * Get mic buffer length
 */
uint32_t microphone_get_buffer_length(void){
	return MIC_BUF_SIZE;
}

/*
 * Callback function for when 1st half of buffer filled
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc){
		//send notification to audio task
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR((TaskHandle_t)AudioTaskHandle, BUF_HALF_READY, eSetBits, &xHigherPriorityTaskWoken);

		// If xHigherPriorityTaskWoken was set to true, we should yield.
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/*
 * Callback function for when 2nd half of buffer filled
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc){
		//send notification to audio task
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR((TaskHandle_t)AudioTaskHandle, BUF_FULL_READY, eSetBits, &xHigherPriorityTaskWoken);

		// If xHigherPriorityTaskWoken was set to true, we should yield.
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
