/*
 * audio_task.c
 *
 *  Created on: Jul 6, 2026
 *      Author: maxda
 */

#include "microphone.h"
#include "FreeRTOS.h"
#include "task.h"

// buffer to hold microphone data
uint32_t audio_buf[MIC_HALF_BUF_SIZE];

void vTaskAudio(void *argument){
	microphone_init(&hadc1);
	microphone_start();
	for(;;){
		//wait for notification from microphone DMA interrupt
		uint32_t notif_value;

		xTaskNotifyWait(0x00, BUF_HALF_READY | BUF_FULL_READY, &notif_value, portMAX_DELAY);

		//first half of buffer ready
		if(notif_value & BUF_HALF_READY){
			//copy first half of microphone buffer to audio buffer
			const uint16_t *mic_buf = microphone_get_buffer();
			for(uint32_t i = 0; i < MIC_HALF_BUF_SIZE; i++){
				audio_buf[i] = mic_buf[i];
			}
			printf("Half buffer ready\n\r");
		}
		else if(notif_value & BUF_FULL_READY){
			//copy second half of microphone buffer to audio buffer
			const uint16_t *mic_buf = microphone_get_buffer();
			for(uint32_t i = 0; i < MIC_HALF_BUF_SIZE; i++){
				audio_buf[i] = mic_buf[i + MIC_HALF_BUF_SIZE];
			}
			printf("Full buffer ready\n\r");
		}
	}
}
