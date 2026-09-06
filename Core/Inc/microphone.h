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
#include "i2s.h"

#define MIC_SAMPLE_RATE_HZ 16000U

#define MIC_BUF_SIZE 4096U
#define MIC_HALF_BUF_SIZE (MIC_BUF_SIZE / 2)

// Number of 16-bit half-words per I2S audio frame (2 channels, 32 bits per channel)
#define I2S_HALFWORDS_PER_FRAME 4U

// Size of I2S DMA buffer is determined by the set MIC_BUF_SIZE and the fact that
// Each I2S audio frame consists of I2S_HALFWORDS_PER_FRAME half-words, of which only one is actually useful to us (the left channel upper 16 bits)
#define I2S_DMA_HALF_SIZE (MIC_HALF_BUF_SIZE * I2S_HALFWORDS_PER_FRAME)
#define I2S_DMA_BUF_SIZE (I2S_DMA_HALF_SIZE * 2U)

#define BUF_HALF_READY (1UL << 0)
#define BUF_FULL_READY (1UL << 1)

void microphone_init(I2S_HandleTypeDef *hi2s);   // Initialize the microphone hardware
bool microphone_start(void);  // Start DMA transfer for microphone data
bool microphone_stop(void); //Stop DMA transfer when not in use

bool microphone_copy_half(uint32_t ready_flag, int16_t *destination);

/* Atomically return and clear the DMA half/full completion flags. */
uint32_t microphone_take_ready_flags(void);

/* Discard any DMA buffer completions that have not been processed yet. */
void microphone_discard_pending(void);

#endif /* INC_MICROPHONE_H_ */
