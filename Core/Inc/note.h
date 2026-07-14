/*
 * note.h
 *
 *  Created on: Jul 13, 2026
 *      Author: maxda
 */

#ifndef INC_NOTE_H_
#define INC_NOTE_H_

/* --------------------------------------------------------------------------
 * Guitar Note Frequencies (Hz)
 * Covers common 6-string guitar tunings.
 * -------------------------------------------------------------------------- */

#define NOTE_C2_HZ      65.41f
#define NOTE_CS2_HZ     69.30f
#define NOTE_D2_HZ      73.42f
#define NOTE_DS2_HZ     77.78f
#define NOTE_E2_HZ      82.41f
#define NOTE_F2_HZ      87.31f
#define NOTE_FS2_HZ     92.50f
#define NOTE_G2_HZ      98.00f
#define NOTE_GS2_HZ     103.83f
#define NOTE_A2_HZ      110.00f
#define NOTE_AS2_HZ     116.54f
#define NOTE_B2_HZ      123.47f

#define NOTE_C3_HZ      130.81f
#define NOTE_CS3_HZ     138.59f
#define NOTE_D3_HZ      146.83f
#define NOTE_DS3_HZ     155.56f
#define NOTE_E3_HZ      164.81f
#define NOTE_F3_HZ      174.61f
#define NOTE_FS3_HZ     185.00f
#define NOTE_G3_HZ      196.00f
#define NOTE_GS3_HZ     207.65f
#define NOTE_A3_HZ      220.00f
#define NOTE_AS3_HZ     233.08f
#define NOTE_B3_HZ      246.94f

#define NOTE_C4_HZ      261.63f
#define NOTE_CS4_HZ     277.18f
#define NOTE_D4_HZ      293.66f
#define NOTE_DS4_HZ     311.13f
#define NOTE_E4_HZ      329.63f
#define NOTE_F4_HZ      349.23f

typedef struct{
	const char* name;
	float frequency;
} note_t;

#endif /* INC_NOTE_H_ */
