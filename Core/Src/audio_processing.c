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

/* Maximum difference between consecutive readings considered stable. */
#define ALLOWED_SAMPLE_VARIANCE       1.0f

/* Number of consecutive stable readings required to accept a pitch. */
#define NUM_SAMPLES_FOR_DETECTION     4U

static int16_t audio_buf[MIC_HALF_BUF_SIZE];
static float previous_frequency = 0.0f;
static uint8_t stable_sample_count = 0U;

/*
 * Process a single frame of audio data.
 * Copies the source buffer into the internal audio buffer,
 * computes the frequency using FFT, and updates the result struct
 */
static bool process_frame(uint32_t ready_flag, uint8_t string, audio_processing_result_t *result){
	/*
	 * This copies only the selected I2S channel and converts its
	 * 24-bit samples into signed 16-bit samples.
	 */
	if (!microphone_copy_half(ready_flag, audio_buf)) {
		return false;
	}

	//get the frequency of the current audio frame
	float frequency = get_freq_fft(audio_buf, string);
	result->frequency_hz = frequency;
	result->frequency_valid = frequency > 0.0f;

	//if the frequency is not valid, reset the previous frequency and stable sample count
	if (!result->frequency_valid){
		previous_frequency = 0.0f;
		stable_sample_count = 0U;
		return true;
	}
 
	//either start new stable sequence or continue the current one if within allowed variance
	if ((stable_sample_count == 0U) || (fabsf(frequency - previous_frequency) <= ALLOWED_SAMPLE_VARIANCE)){
		stable_sample_count++;
	}
	else{
		stable_sample_count = 1U;
	}

	previous_frequency = frequency;

	//check if the current sequence of stable samples is long enough to consider the frequency stable
	if (stable_sample_count >= NUM_SAMPLES_FOR_DETECTION){
		result->stable_frequency = true;
		stable_sample_count = 0U;
	}
	return true;
}

/**
 * Reset the audio processing state.
 * Clears the previous frequency and stable sample count,
 * and discards any pending microphone data.
 */
void audio_processing_reset(void){
	previous_frequency = 0.0f;
	stable_sample_count = 0U;
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

	//determine which DMA halves are ready for processing
	uint32_t ready_flags = microphone_take_ready_flags();
	if (ready_flags == 0U){
		return false;
	}

	bool processed = false;

	if ((ready_flags & BUF_HALF_READY) != 0U){
		if(process_frame(BUF_HALF_READY, string, result)) processed = true;

		if (result->stable_frequency){
			return true;
		}
	}
	if ((ready_flags & BUF_FULL_READY) != 0U){
		if(process_frame(BUF_FULL_READY, string, result)) processed = true;
	}

	return processed;
}
