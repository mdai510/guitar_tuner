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

// buffer to hold microphone data
static uint16_t audio_buf[MIC_HALF_BUF_SIZE];

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
		printf("Frequency: %.2f Hz\r\n", frequency);

		if(b1_pressed_debounced()){
			if(string <= 1) {
				printf("string 6\r\n");
				string = 6;
				continue;
			}
			string--;
			printf("string %i\r\n", string);
		}
	}
}
