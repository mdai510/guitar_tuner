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

static ADC_HandleTypeDef *mic_hadc;
static uint16_t mic_adc_buf[MIC_BUF_SIZE];
static volatile uint32_t mic_ready_flags = 0U;

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
	microphone_discard_pending();
	HAL_ADC_Start_DMA(mic_hadc, (uint32_t*)mic_adc_buf, MIC_BUF_SIZE);
	// Start 8kHz timer
	HAL_TIM_Base_Start(&htim6);
}

/*
 * Stop microphone data DMA transfers
 */
void microphone_stop(void){
	HAL_TIM_Base_Stop(&htim6);
	HAL_ADC_Stop_DMA(mic_hadc);
	microphone_discard_pending();
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
 * Return and clear DMA completion flags without losing an interrupt that
 * arrives while the main loop is taking the snapshot.
 */
uint32_t microphone_take_ready_flags(void){
	uint32_t primask = __get_PRIMASK();
	uint32_t ready_flags;

	__disable_irq();
	ready_flags = mic_ready_flags;
	mic_ready_flags = 0U;

	if (primask == 0U){
		__enable_irq();
	}

	return ready_flags;
}

void microphone_discard_pending(void){
	(void)microphone_take_ready_flags();
}

/*
 * Callback function for when 1st half of buffer filled
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc){
		mic_ready_flags |= BUF_HALF_READY;
	}
}

/*
 * Callback function for when 2nd half of buffer filled
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == mic_hadc){
		mic_ready_flags |= BUF_FULL_READY;
	}
}
