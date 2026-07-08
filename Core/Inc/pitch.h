/*
 * pitch.h
 *
 *  Created on: Jul 7, 2026
 *      Author: maxda
 */

#ifndef INC_PITCH_H_
#define INC_PITCH_H_

// get frequency from audio buffer using autocorrelation
// if amplitude is too low, return 0
// if frequency is out of reasonable range, return 0
// otherwise return frequency in Hz
float get_freq(const uint16_t *audio_buf, uint16_t buf_len);

#endif /* INC_PITCH_H_ */
