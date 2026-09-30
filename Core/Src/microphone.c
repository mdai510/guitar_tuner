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
bool microphone_init(ADC_HandleTypeDef *hadc){
    if (hadc == NULL) {
        return false;
    }

    mic_hadc = hadc;
    mic_ready_flags = 0U;

    return HAL_ADCEx_Calibration_Start(mic_hadc, ADC_SINGLE_ENDED) == HAL_OK;
}

/*
 * Start the ADC DMA transfer and the timer that triggers conversions.
 */
void microphone_start(void){
    microphone_discard_pending();
    HAL_ADC_Start_DMA(mic_hadc, (uint32_t *)mic_adc_buf, MIC_BUF_SIZE);
    HAL_TIM_Base_Start(&htim6);
}

/*
 * Stop the timer and ADC DMA transfer.
 */
void microphone_stop(void){
    HAL_TIM_Base_Stop(&htim6);
    HAL_ADC_Stop_DMA(mic_hadc);
    microphone_discard_pending();
}

/*
 * Copy the requested half of the ADC DMA buffer to the destination buffer.
 * Returns true if the copy was successful, false otherwise
 */
bool microphone_copy_half(uint32_t ready_flag, int16_t *destination){
    uint32_t offset;

    if (destination == NULL) {
        return false;
    }

    if (ready_flag == BUF_HALF_READY) {
        offset = 0U;
    }
    else if (ready_flag == BUF_FULL_READY) {
        offset = MIC_HALF_BUF_SIZE;
    }
    else {
        return false;
    }

    for (uint32_t i = 0U; i < MIC_HALF_BUF_SIZE; i++) {
        destination[i] = (int16_t)mic_adc_buf[offset + i];
    }

    return true;
}

/*
 * Atomically return and clear the DMA half/full completion flags.
 */
uint32_t microphone_take_ready_flags(void){
    uint32_t primask = __get_PRIMASK();
    uint32_t flags;

    __disable_irq();
    flags = mic_ready_flags;
    mic_ready_flags = 0U;

    if (primask == 0U){
        __enable_irq();
    }

    return flags;
}

/*
 * Discard any DMA buffer completions that have not been processed yet.
 */
void microphone_discard_pending(void){
	(void)microphone_take_ready_flags();
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc){
    if (hadc == mic_hadc){
        mic_ready_flags |= BUF_HALF_READY;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
    if (hadc == mic_hadc){
        mic_ready_flags |= BUF_FULL_READY;
    }
}
