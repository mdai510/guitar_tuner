/*
 * pitch.c
 *
 *  Created on: Jul 7, 2026
 *      Author: maxda
 */

#include <stdint.h>
#include "pitch.h"
#include "microphone.h"

#define MIN_SIGNAL_LVL 60U

#define SAMPLE_RATE_HZ 8000U

#define TUNER_MIN_FREQ_HZ 60U
#define TUNER_MAX_FREQ_HZ 400U

#define MIN_LAG (SAMPLE_RATE_HZ / TUNER_MAX_FREQ_HZ)
#define MAX_LAG (SAMPLE_RATE_HZ / TUNER_MIN_FREQ_HZ)

//centered audio buffer
static int16_t centered_audio_buf[MIC_HALF_BUF_SIZE];

//static helper functions
static int find_period_autocorrelation(const int16_t *centered_buf);
static int16_t ave_amplitude(int16_t *centered_audio_buf, uint16_t buf_len);
static void center_audio_buffer(uint16_t *audio_buf, uint16_t buf_len, int16_t *centered_audio_buf);
static uint32_t ave_audio_buffer(const uint16_t *audio_buf, uint16_t buf_len);

// get frequency from audio buffer using autocorrelation
// if amplitude is too low, return 0
// if frequency is out of reasonable range, return 0
// otherwise return frequency in Hz
float get_freq(const uint16_t *audio_buf, uint16_t buf_len){
    //center audio buffer around zero
    center_audio_buffer((uint16_t*)audio_buf, buf_len, centered_audio_buf);

    int16_t audio_ave_amp = ave_amplitude(centered_audio_buf, buf_len);
    if(audio_ave_amp < MIN_SIGNAL_LVL){
    	return 0.0;
    }

    int bestLag = find_period_autocorrelation(centered_audio_buf);
    if(bestLag <= 0.0){
    	return 0.0;
    }

    return (float)SAMPLE_RATE_HZ / (float)bestLag;
}

//helper to calculate the best lag using autocorrelation
static int find_period_autocorrelation(const int16_t *centered_buf){
    int bestLag = 0;
    int64_t bestCorrelation = INT64_MIN;

    //only interested in frequencies for open string guitar tunings
    for (int lag = MIN_LAG; lag <= MAX_LAG; lag++){
        int64_t correlation = 0;

        for (uint32_t i = 0; i < MIC_HALF_BUF_SIZE - lag; i++){
            correlation += (int32_t)centered_buf[i] *
                           (int32_t)centered_buf[i + lag];
        }

        if (correlation > bestCorrelation){
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    return bestLag;
}

//helper to get the average amplitude of the centered signal
static int16_t ave_amplitude(int16_t *centered_audio_buf, uint16_t buf_len){
	uint32_t sum = 0;
	for(uint32_t i = 0; i < buf_len; i++){
		if(centered_audio_buf[i] < 0){
			sum += -centered_audio_buf[i];
		}else{
			sum += centered_audio_buf[i];
		}
	}
	return (int16_t)(sum / buf_len);
}

//helper to center audio buffer around zero
static void center_audio_buffer(uint16_t *audio_buf, uint16_t buf_len, int16_t *centered_audio_buf){
    uint32_t ave = ave_audio_buffer(audio_buf, buf_len);
    for(uint32_t i = 0; i < buf_len; i++){
        centered_audio_buf[i] = (int16_t)audio_buf[i] - (int16_t)ave;
    }
}

//helper to calculate average of audio buffer
static uint32_t ave_audio_buffer(const uint16_t *audio_buf, uint16_t buf_len){
    uint32_t sum = 0;
    for(uint32_t i = 0; i < buf_len; i++){
        sum += audio_buf[i];
    }
    return sum / buf_len;
}
