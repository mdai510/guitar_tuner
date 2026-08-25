/*
 * audio_processing.h
 *
 * Bare-metal microphone-frame processing previously owned by AudioTask.
 */

#ifndef INC_AUDIO_PROCESSING_H_
#define INC_AUDIO_PROCESSING_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	bool frequency_valid;
	bool stable_frequency;
	float frequency_hz;
} audio_processing_result_t;

/* Reset the consecutive-frequency detector when starting a new string. */
void audio_processing_reset(void);

/*
 * Process every completed DMA half currently pending.
 * Returns true when at least one fresh audio frame was processed.
 */
bool audio_process_pending(uint8_t string,
					   audio_processing_result_t *result);

#endif /* INC_AUDIO_PROCESSING_H_ */
