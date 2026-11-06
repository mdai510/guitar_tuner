/*
 * audio_processing.c
 *
 * Bare-metal replacement for the former FreeRTOS AudioTask.
 */

#include "audio_processing.h"
#include "microphone.h"
#include "pitch.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

/* Maximum difference between consecutive filtered readings considered stable. */
#define ALLOWED_SAMPLE_VARIANCE       1.0f

/* Number of consecutive stable filtered readings required to accept a pitch. */
#define NUM_SAMPLES_FOR_DETECTION     2U

/* A three-frame median removes isolated incorrect FFT readings. */
#define MEDIAN_WINDOW_SIZE            3U

static int16_t audio_buf[MIC_HALF_BUF_SIZE];
static float previous_frequency = 0.0f;
static uint8_t stable_sample_count = 0U;

static float frequency_history[MEDIAN_WINDOW_SIZE];
static uint8_t frequency_history_count = 0U;
static uint8_t frequency_history_index = 0U;

static float median_of_three(float a, float b, float c){
	if (a > b){
		float temp = a;
		a = b;
		b = temp;
	}

	if (b > c){
		float temp = b;
		b = c;
		c = temp;
	}

	if (a > b){
		float temp = a;
		a = b;
		b = temp;
	}

	return b;
}

static void reset_detection_state(void){
	previous_frequency = 0.0f;
	stable_sample_count = 0U;
	frequency_history_count = 0U;
	frequency_history_index = 0U;

	for (uint32_t i = 0U; i < MEDIAN_WINDOW_SIZE; i++){
		frequency_history[i] = 0.0f;
	}
}

/*
 * Process a single completed DMA half, compute its FFT frequency, and update
 * the median and consecutive-stability filters.
 */
static bool process_frame(uint32_t ready_flag,
						  uint8_t string,
						  audio_processing_result_t *result){
	if (!microphone_copy_half(ready_flag, audio_buf)){
		return false;
	}

	float frequency = get_freq_fft(audio_buf, string);

	result->frequency_hz = frequency;
	result->frequency_valid = frequency > 0.0f;

	/* An invalid frame breaks both the median window and stable sequence. */
	if (!result->frequency_valid){
		reset_detection_state();
		return true;
	}

	frequency_history[frequency_history_index] = frequency;
	frequency_history_index =
		(frequency_history_index + 1U) % MEDIAN_WINDOW_SIZE;

	if (frequency_history_count < MEDIAN_WINDOW_SIZE){
		frequency_history_count++;
	}

	/* Do not allow motor control until one complete median window exists. */
	if (frequency_history_count < MEDIAN_WINDOW_SIZE){
		return true;
	}

	float filtered_frequency = median_of_three(frequency_history[0],
										 frequency_history[1],
										 frequency_history[2]);

	result->frequency_hz = filtered_frequency;

	if ((stable_sample_count == 0U) ||
		(fabsf(filtered_frequency - previous_frequency) <= ALLOWED_SAMPLE_VARIANCE)){
		stable_sample_count++;
	}
	else{
		stable_sample_count = 1U;
	}

	previous_frequency = filtered_frequency;

	if (stable_sample_count >= NUM_SAMPLES_FOR_DETECTION){
		result->stable_frequency = true;
		stable_sample_count = 0U;
	}

	return true;
}

/**
 * Reset all audio-processing state and discard any pending microphone data.
 */
void audio_processing_reset(void){
	reset_detection_state();
	microphone_discard_pending();
}

/*
 * Process every completed DMA half currently pending.
 * Returns true when at least one fresh audio frame was processed.
 */
bool audio_process_pending(uint8_t string, audio_processing_result_t *result){
	if ((result == NULL) || (string < 1U) || (string > 6U)){
		return false;
	}

	result->frequency_valid = false;
	result->stable_frequency = false;
	result->frequency_hz = 0.0f;

	uint32_t ready_flags = microphone_take_ready_flags();

	if (ready_flags == 0U){
		return false;
	}

	bool processed = false;

	if ((ready_flags & BUF_HALF_READY) != 0U){
		if (process_frame(BUF_HALF_READY, string, result)){
			processed = true;
		}

		if (result->stable_frequency){
			return true;
		}
	}

	if ((ready_flags & BUF_FULL_READY) != 0U){
		if (process_frame(BUF_FULL_READY, string, result)){
			processed = true;
		}
	}

	return processed;
}
