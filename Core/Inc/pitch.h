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

#define STRING_6_MIN_HZ    55.000f   /* A1 */
#define STRING_6_MAX_HZ    82.407f   /* E2 */

#define STRING_5_MIN_HZ    82.407f   /* E2 */
#define STRING_5_MAX_HZ   123.471f   /* B2 */

#define STRING_4_MIN_HZ   110.000f   /* A2 */
#define STRING_4_MAX_HZ   164.814f   /* E3 */

#define STRING_3_MIN_HZ   146.832f   /* D3 */
#define STRING_3_MAX_HZ   220.000f   /* A3 */

#define STRING_2_MIN_HZ   184.997f   /* F#3 */
#define STRING_2_MAX_HZ   329.628f   /* E4 */

#define STRING_1_MIN_HZ   246.942f   /* B3 */
#define STRING_1_MAX_HZ   391.995f   /* G4 */

//initialize fft config and buffers
bool fft_init(void);

void fft_deinit(void);

//float get_freq_fft(const uint16_t *audio_buf, float expected_freq);
float get_freq_fft(const uint16_t *audio_buf, uint8_t string);

#endif /* INC_PITCH_H_ */
