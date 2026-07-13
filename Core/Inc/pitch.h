/*
 * pitch.h
 *
 *  Created on: Jul 7, 2026
 *      Author: maxda
 */

#ifndef INC_PITCH_H_
#define INC_PITCH_H_

#include <stdbool.h>
#include <stdint.h>

//initialize fft config and buffers
bool fft_init(void);

void fft_deinit(void);

float get_freq_fft(const uint16_t *audio_buf);

#endif /* INC_PITCH_H_ */
