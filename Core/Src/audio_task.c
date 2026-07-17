/*
 * audio_task.c
 *
 *  Created on: Jul 6, 2026
 *      Author: maxda
 */

#include "microphone.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pitch.h"
#include "button.h"
#include "note.h"
#include "math.h"

// +- frequency the note played is allowed to vary by to still be considered
#define ALLOWED_SAMPLE_VARIANCE 1.0f
//number of successive samples of ~same frequency needed to register a note played
//must be 1 or more
#define NUM_SAMPLE_FOR_DETECTION 4U

// buffer to hold microphone data
static uint16_t audio_buf[MIC_HALF_BUF_SIZE];
//holds the last 6 frequency samples
static float prev_samples[NUM_SAMPLE_FOR_DETECTION] = {0};

void vTaskAudio(void *argument){
	//suspend task, wait till guitar notes are selected and button is pressed
	vTaskSuspend(NULL);

	//ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	uint8_t string = 6;

	for(;;){
		uint32_t notif_value;

		xTaskNotifyWait(0x00, BUF_HALF_READY | BUF_FULL_READY, &notif_value, portMAX_DELAY);

		//first half of buffer ready
		if(notif_value & BUF_HALF_READY){
			//copy first half of microphone buffer to audio buffer
			const uint16_t *mic_buf = microphone_get_buffer();
			for(uint16_t i = 0; i < MIC_HALF_BUF_SIZE; i++){
				audio_buf[i] = mic_buf[i];
			}
		}
		else if(notif_value & BUF_FULL_READY){
			//copy second half of microphone buffer to audio buffer
			const uint16_t *mic_buf = microphone_get_buffer();
			for(uint16_t i = 0; i < MIC_HALF_BUF_SIZE; i++){
				audio_buf[i] = mic_buf[i + MIC_HALF_BUF_SIZE];
			}
		}

		float frequency = get_freq_fft(audio_buf, string);
		//printf("Frequency: %.2f Hz\r\n", frequency);
		if(frequency == 0.0) continue;

		uint8_t freq_match_count = 0;
		bool is_still_valid = true;
		for(uint8_t i = 0; i < NUM_SAMPLE_FOR_DETECTION; i++){
			if(i < NUM_SAMPLE_FOR_DETECTION - 1){
				prev_samples[i] = prev_samples[i+1];
				if(is_still_valid){
					if(fabsf(prev_samples[i] - frequency) <= ALLOWED_SAMPLE_VARIANCE){
						freq_match_count++;
					} else{
						is_still_valid = false;
					}
				}
			}
			else{
				prev_samples[i] = frequency;
				if(is_still_valid) freq_match_count++;
			}
		}
		if(freq_match_count == NUM_SAMPLE_FOR_DETECTION){
			/*
			//average prev_samples
			for(uint_8 j = 0; j < NUM_SAMPLE_FOR_DETECTION; j++){

			}*/

			//for now just print last sample and move to next string
			if(string <= 1) {
				printf("string 1: %.2f\r\n", frequency);
				string = 6;
				vTaskDelay(1000);
			}
			else{
				printf("string %i: %.2f\r\n", string, frequency);
				string--;
				vTaskDelay(1000);
			}
		}

		/*
		if(b1_pressed_debounced()){
			if(string <= 1) {
				printf("string 6\r\n");
				string = 6;
				continue;
			}
			string--;
			printf("string %i\r\n", string);
		}*/
	}
}
