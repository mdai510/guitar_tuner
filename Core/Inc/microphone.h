/*
 * microphone.h
 *
 *  Created on: Jun 30, 2026
 *      Author: maxda
 */

#ifndef INC_MICROPHONE_H_
#define INC_MICROPHONE_H_

#include <stdint.h>
#include <stdbool.h>
#include "tim.h"
#include "adc.h"

#define MIC_SAMPLE_RATE_HZ 8000U

#define MIC_BUF_SIZE 4096U
#define MIC_HALF_BUF_SIZE (MIC_BUF_SIZE / 2U)

#define BUF_HALF_READY (1UL << 0)
#define BUF_FULL_READY (1UL << 1)

bool microphone_init(ADC_HandleTypeDef *hadc);   // Initialize the microphone hardware
void microphone_start(void);  // Start DMA transfer for microphone data
void microphone_stop(void); //Stop DMA transfer when not in use

bool microphone_copy_half(uint32_t ready_flag, int16_t *destination);

/* Atomically return and clear the DMA half/full completion flags. */
uint32_t microphone_take_ready_flags(void);

/* Discard any DMA buffer completions that have not been processed yet. */
void microphone_discard_pending(void);

#endif /* INC_MICROPHONE_H_ */
