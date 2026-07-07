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

// Initialize the microphone with the given ADC handle
void microphone_init(ADC_HandleTypeDef *hadc){
	mic_hadc = hadc;
}

// Start the DMA transfer for microphone data
void microphone_start(void){
	HAL_ADC_Start_DMA(mic_hadc, (uint32_t*)mic_adc_buf, MIC_BUF_SIZE);
	// Start 8kHz timer
	HAL_TIM_Base_Start(&htim6);
}

// Get buffer pointer
const uint16_t* microphone_get_buffer(void){
	return mic_adc_buf;
}

// Get buffer length
uint32_t microphone_get_buffer_length(void){
	return MIC_BUF_SIZE;
}

// Helpers to calculate average, max, and min of the buffer
static uint32_t ave_buffer_uint32(const uint16_t *buf, uint32_t len){
	uint64_t sum = 0;
	for(uint32_t i = 0; i < len; i++){
		sum += buf[i];
	}
	return (uint32_t)(sum / len);
}

static uint32_t max_buffer_uint32(const uint16_t *buf, uint32_t len){
	uint32_t max_val = 0;
	for(uint32_t i = 0; i < len; i++){
		if(buf[i] > max_val){
			max_val = buf[i];
		}
	}
	return max_val;
}

static uint32_t min_buffer_uint32(const uint16_t *buf, uint32_t len){
	uint32_t min_val = 0xFFFF;
	for(uint32_t i = 0; i < len; i++){
		if(buf[i] < min_val){
			min_val = buf[i];
		}
	}
	return min_val;
}

// Called when first half of buffer is filled
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc)
	    {
	        uint32_t avg = ave_buffer_uint32(mic_adc_buf, MIC_HALF_BUF_SIZE);
			uint32_t max = max_buffer_uint32(mic_adc_buf, MIC_HALF_BUF_SIZE);
			uint32_t min = min_buffer_uint32(mic_adc_buf, MIC_HALF_BUF_SIZE);
			uint32_t peak_to_peak = max - min;

	        //printf("First half avg = %lu, max = %lu, min = %lu, peak-to-peak = %lu\r\n", avg, max, min, peak_to_peak);

	        //send notification to audio task
	        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	        xTaskNotifyFromISR((TaskHandle_t)AudioTaskHandle, BUF_HALF_READY, eSetBits, &xHigherPriorityTaskWoken);

			// If xHigherPriorityTaskWoken was set to true, we should yield.
	        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }
}

// Called when buffer is completely filled
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc)
	    {
	        uint32_t avg = ave_buffer_uint32(&mic_adc_buf[MIC_HALF_BUF_SIZE], MIC_HALF_BUF_SIZE);
			uint32_t max = max_buffer_uint32(&mic_adc_buf[MIC_HALF_BUF_SIZE], MIC_HALF_BUF_SIZE);
			uint32_t min = min_buffer_uint32(&mic_adc_buf[MIC_HALF_BUF_SIZE], MIC_HALF_BUF_SIZE);
			uint32_t peak_to_peak = max - min;

	        //printf("Second half avg = %lu, max = %lu, min = %lu, peak-to-peak = %lu\r\n", avg, max, min, peak_to_peak);

			//send notification to audio task
	        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	        xTaskNotifyFromISR((TaskHandle_t)AudioTaskHandle, BUF_FULL_READY, eSetBits, &xHigherPriorityTaskWoken);

			// If xHigherPriorityTaskWoken was set to true, we should yield.
	        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }
}
