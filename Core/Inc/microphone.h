/*
 * microphone.h
 *
 *  Created on: Jun 30, 2026
 *      Author: maxda
 */

#ifndef INC_MICROPHONE_H_
#define INC_MICROPHONE_H_

#include <stdint.h>
#include "main.h"
#include "tim.h"
#include "adc.h"

#define MIC_BUF_SIZE 2048
#define MIC_HALF_BUF_SIZE (MIC_BUF_SIZE / 2)

#define BUF_HALF_READY (1UL << 0)
#define BUF_FULL_READY (1UL << 1)

void microphone_init(ADC_HandleTypeDef *hadc1);   // Initialize the microphone hardware
void microphone_start(void);  // Start DMA transfer for microphone data

// functions to get buffer pointer and length
const uint16_t* microphone_get_buffer(void);
uint32_t microphone_get_buffer_length(void);

#endif /* INC_MICROPHONE_H_ */
