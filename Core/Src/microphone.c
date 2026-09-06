/*
 * microphone.c
 *
 *  Created on: Jun 30, 2026
 *      Author: maxda
 */

#include "microphone.h"
#include "main.h"

#define MIC_CHANNEL_HALFWORD_OFFSET 0U  /* Left channel */

static I2S_HandleTypeDef *mic_hi2s;
static uint16_t i2s_dma_buf[I2S_DMA_BUF_SIZE];
static volatile uint32_t mic_ready_flags = 0U;

/*
 * Initialize the microphone with the given I2S handle
 */
void microphone_init(I2S_HandleTypeDef *hi2s){
	mic_hi2s = hi2s;
	mic_ready_flags = 0U;
}

/*
 * Start the DMA transfer for I2S mic data
 * This I2S is configured for 24-bit data, while DMA/STM32 handles it as 16-bit half-words.
 * INMP441 sends 24 bit signed left-aligned sample inside 32 bit slot (the lower 8 bits are padding)
 * The DMA captures complete slot as two 16-bit half-words
 * 
 * I2S also has two channels, but the L/R pin is grounded so only the left on is driven by the INMP441
 * Thus the structure of the buffer as DMA fills it is:
 * [left sample 1 upper 16 bits, left sample 1 lower 8 bits + padding,
 *  right sample 1 upper 16 bits (maybe junk), right sample 1 lower 8 bits + padding(maybe junk),
 *  ...
 * ]
 * Since we only need the left channel, and only the upper 16 bits of each left sample
 * we can ignore the rest of the data in the DMA buffer (each 2nd to 4th half-word of every frame)
 *
 * HAL_I2S_Receive_DMA() requires special handling for its Size argument
 * in 24-bit and 32-bit I2S modes. STM32 HAL internally doubles Size
 * because every I2S channel slot requires two 16-bit DMA transfers.
 *
 * I2S_DMA_BUF_SIZE is the actual number of uint16_t elements allocated
 * in i2s_dma_buf. Therefore, Size is passed as I2S_DMA_BUF_SIZE / 2:
 *
 *     HAL internal transfer count
 *         = (I2S_DMA_BUF_SIZE / 2) * 2
 *         = I2S_DMA_BUF_SIZE halfwords
 */
bool microphone_start(void){
	if (mic_hi2s == NULL){
		return false;
	}
	microphone_discard_pending();
	return (HAL_I2S_Receive_DMA(mic_hi2s, i2s_dma_buf, I2S_DMA_BUF_SIZE / 2U) == HAL_OK);
}

/*
 * Stop microphone data DMA transfers
 */
bool microphone_stop(void){
	if(mic_hi2s == NULL){
		return false;
	}
	HAL_StatusTypeDef status = HAL_I2S_DMAStop(mic_hi2s);
	microphone_discard_pending();
	return status == HAL_OK;
}

/*
 * Copy the requested half of the I2S DMA buffer to the destination buffer
 * Returns true if the copy was successful, false otherwise
 */
bool microphone_copy_half(uint32_t ready_flag, int16_t *destination){
    const uint16_t *source;

    if (destination == NULL) {
        return false;
    }

    if (ready_flag == BUF_HALF_READY) {
        source = &i2s_dma_buf[0];
    }
    else if (ready_flag == BUF_FULL_READY) {
        source = &i2s_dma_buf[I2S_DMA_HALF_SIZE];
    }
    else {
        return false;
    }

    for (uint32_t i = 0U; i < MIC_HALF_BUF_SIZE; i++) {
        uint32_t index = (i * I2S_HALFWORDS_PER_FRAME) + MIC_CHANNEL_HALFWORD_OFFSET;
        destination[i] = (int16_t)source[index];
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

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s){
    if (hi2s == mic_hi2s){
        mic_ready_flags |= BUF_HALF_READY;
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s){
    if (hi2s == mic_hi2s){
        mic_ready_flags |= BUF_FULL_READY;
    }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s){
    if (hi2s == mic_hi2s) {
        /* Set a breakpoint here during initial testing. */
    }
}
