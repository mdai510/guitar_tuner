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

#define NUM_NOTES 30U

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
	const char* note_name;
	float frequency;
} note_t;

/*
static const note_t NoteTable[NUM_NOTES] = {
		[NOTE_A1]  = { "A1",  55.000f },
		[NOTE_AS1] = { "A#1", 58.270f },
		[NOTE_B1]  = { "B1",  61.735f },
		[NOTE_C2]  = { "C2",  NOTE_C2_HZ},
		[NOTE_CS2] = { "C#2", NOTE_CS2_HZ },
		[NOTE_D2]  = { "D2",  NOTE_D2_HZ },
		[NOTE_DS2] = { "D#2", NOTE_DS2_HZ },
		[NOTE_E2]  = { "E2",  NOTE_E2_HZ },

		[NOTE_F2]  = { "F2",  NOTE_F2_HZ },
		[NOTE_FS2] = { "F#2", NOTE_FS2_HZ },
		[NOTE_G2]  = { "G2",  NOTE_G2_HZ },
		[NOTE_GS2] = { "G#2", NOTE_GS2_HZ },

		[NOTE_A2]  = { "A2",  NOTE_A2_HZ },
		[NOTE_AS2] = { "A#2", NOTE_AS2_HZ },
		[NOTE_B2]  = { "B2",  NOTE_B2_HZ },
		[NOTE_C3]  = { "C3",  NOTE_C3_HZ },
		[NOTE_CS3] = { "C#3", NOTE_CS3_HZ },
		[NOTE_D3]  = { "D3",  NOTE_D3_HZ },
		[NOTE_DS3] = { "D#3", NOTE_DS3_HZ },
		[NOTE_E3]  = { "E3",  NOTE_E3_HZ },

		[NOTE_F3]  = { "F3",  NOTE_F3_HZ },
		[NOTE_FS3] = { "F#3", NOTE_FS3_HZ },
		[NOTE_G3]  = { "G3",  NOTE_G3_HZ },
		[NOTE_GS3] = { "G#3", NOTE_GS3_HZ },

		[NOTE_A3]  = { "A3",  NOTE_A3_HZ },
		[NOTE_AS3] = { "A#3", NOTE_AS3_HZ },
		[NOTE_B3]  = { "B3",  NOTE_B3_HZ },
		[NOTE_C4]  = { "C4",  NOTE_C4_HZ },
		[NOTE_CS4] = { "C#4", NOTE_CS4_HZ },
		[NOTE_D4]  = { "D4",  NOTE_D4_HZ },
		[NOTE_DS4] = { "D#4", NOTE_DS4_HZ },
		[NOTE_E4]  = { "E4",  NOTE_E4_HZ },

		[NOTE_F4]  = { "F4",  NOTE_F4_HZ },
		[NOTE_FS4] = { "F#4", NOTE_FS4_HZ },
		[NOTE_G4]  = { "G4",  NOTE_G4_HZ }
};*/

typedef struct{
	const char* tuning_name;
	note_t notes[6];
} tuning_t;

#define NUM_TUNINGS 3U

#endif /* INC_NOTE_H_ */
