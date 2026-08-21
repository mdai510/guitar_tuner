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

#define NOTE_A1_HZ       55.00f
#define NOTE_AS1_HZ      58.27f
#define NOTE_B1_HZ       61.74f

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
#define NOTE_FS4_HZ 	369.99f
#define NOTE_G4_HZ 		392.00f

typedef struct{
	const char* note_name;
	float frequency;
} note_t;

static const note_t a1 = {.note_name = "A1", .frequency = NOTE_A1_HZ};
static const note_t as1 = {.note_name = "A#1", .frequency = NOTE_AS1_HZ};
static const note_t b1 = {.note_name = "B1", .frequency = NOTE_B1_HZ};
static const note_t c2 = {.note_name = "C2", .frequency = NOTE_C2_HZ};
static const note_t cs2 = {.note_name = "C#2", .frequency = NOTE_CS2_HZ};
static const note_t d2 = {.note_name = "D2", .frequency = NOTE_D2_HZ};
static const note_t ds2 = {.note_name = "D#2", .frequency = NOTE_DS2_HZ};
static const note_t e2 = {.note_name = "E2", .frequency = NOTE_E2_HZ};
static const note_t f2 = {.note_name = "F2", .frequency = NOTE_F2_HZ};
static const note_t fs2 = {.note_name = "F#2", .frequency = NOTE_FS2_HZ};
static const note_t g2 = {.note_name = "G2", .frequency = NOTE_G2_HZ};
static const note_t gs2 = {.note_name = "G#2", .frequency = NOTE_GS2_HZ};
static const note_t a2 = {.note_name = "A2", .frequency = NOTE_A2_HZ};
static const note_t as2 = {.note_name = "A#2", .frequency = NOTE_AS2_HZ};
static const note_t b2 = {.note_name = "B2", .frequency = NOTE_B2_HZ};
static const note_t c3 = {.note_name = "C3", .frequency = NOTE_C3_HZ};
static const note_t cs3 = {.note_name = "C#3", .frequency = NOTE_CS3_HZ};
static const note_t d3 = {.note_name = "D3", .frequency = NOTE_D3_HZ};
static const note_t ds3 = {.note_name = "D#3", .frequency = NOTE_DS3_HZ};
static const note_t e3 = {.note_name = "E3", .frequency = NOTE_E3_HZ};
static const note_t f3 = {.note_name = "F3", .frequency = NOTE_F3_HZ};
static const note_t fs3 = {.note_name = "F#3", .frequency = NOTE_FS3_HZ};
static const note_t g3 = {.note_name = "G3", .frequency = NOTE_G3_HZ};
static const note_t gs3 = {.note_name = "G#3", .frequency = NOTE_GS3_HZ};
static const note_t a3 = {.note_name = "A3", .frequency = NOTE_A3_HZ};
static const note_t as3 = {.note_name = "A#3", .frequency = NOTE_AS3_HZ};
static const note_t b3 = {.note_name = "B3", .frequency = NOTE_B3_HZ};
static const note_t c4 = {.note_name = "C4", .frequency = NOTE_C4_HZ};
static const note_t cs4 = {.note_name = "C#4", .frequency = NOTE_CS4_HZ};
static const note_t d4 = {.note_name = "D4", .frequency = NOTE_D4_HZ};
static const note_t ds4 = {.note_name = "D#4", .frequency = NOTE_DS4_HZ};
static const note_t e4 = {.note_name = "E4", .frequency = NOTE_E4_HZ};
static const note_t f4 = {.note_name = "F4", .frequency = NOTE_F4_HZ};
static const note_t fs4 = {.note_name = "F#4", .frequency = NOTE_FS4_HZ};
static const note_t g4 = {.note_name = "G4", .frequency = NOTE_G4_HZ};

typedef struct{
	const char* tuning_name;
	note_t notes[6];
} tuning_t;

#define NUM_TUNINGS 3U

static const tuning_t tunings[NUM_TUNINGS] = {
		{.tuning_name = "E Standard", .notes = {e2, a2, d3, g3, b3, e4}},
		{.tuning_name = "Drop D", .notes = {d2, a2, d3, g3, b3, e4}},
		{.tuning_name = "D Standard", .notes = {d2, g2, c3, f3, a3, d4}}
};

#endif /* INC_NOTE_H_ */
