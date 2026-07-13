/*
 * pitch.c
 *
 *  Created on: Jul 7, 2026
 *      Author: maxda
 */

#include "pitch.h"
#include "microphone.h"
#include "kiss_fftr.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define MIN_SIGNAL_LVL       50U

#define SAMPLE_RATE_HZ       8000U

#define TUNER_MIN_FREQ_HZ    60U
#define TUNER_MAX_FREQ_HZ    400U

#define FFT_BUF_SIZE         MIC_HALF_BUF_SIZE

#define PI_F                 3.14159265358979323846f
#define PARABOLA_EPSILON     1.0e-12f

/*
 * Centered time-domain samples.
 */
static int16_t centered_audio_buf[FFT_BUF_SIZE];

/*
 * FFT input and output arrays.
 *
 * A real FFT with FFT_BUF_SIZE inputs produces:
 * FFT_BUF_SIZE / 2 + 1 unique complex bins.
 */
static kiss_fft_scalar fft_in[FFT_BUF_SIZE];
static kiss_fft_cpx fft_out[(FFT_BUF_SIZE / 2U) + 1U];

/*
 * Hann window coefficients.
 */
static float hann_window[FFT_BUF_SIZE];

/*
 * KISS FFT configuration.
 *
 * fft_cfg points into fft_cfg_buffer, so fft_cfg_buffer must remain allocated
 * for as long as fft_cfg is used.
 */
static kiss_fftr_cfg fft_cfg = NULL;
static void *fft_cfg_buffer = NULL;

/*
 * FFT bin search range.
 */
static uint32_t min_bin = 0U;
static uint32_t max_bin = 0U;

/*
 * Indicates whether fft_init() completed successfully.
 */
static uint8_t fft_initialized = 0U;


/* Private function declarations */

static float fft_run(void);

static void center_audio_buffer(const uint16_t *audio_buf);
static uint32_t ave_audio_buffer(const uint16_t *audio_buf);
static uint32_t ave_amplitude(void);

static void initialize_hann_window(void);
static float get_magnitude_squared(uint32_t bin);
static float interpolate_peak_bin(uint32_t peak_bin);

/*
 * Initialize the FFT.
 *
 * Call this once before calling get_freq_fft().
 *
 * Returns:
 *   true  - initialization succeeded
 *   false - allocation or configuration failed
 */
bool fft_init(void){
    size_t required_size = 0U;

    //Ask KISS FFT how many bytes its configuration requires.
    kiss_fftr_alloc(FFT_BUF_SIZE, 0, NULL, &required_size);
    if (required_size == 0U) return false;

    fft_cfg_buffer = malloc(required_size);
    if (fft_cfg_buffer == NULL) return false;

    fft_cfg = kiss_fftr_alloc(FFT_BUF_SIZE, 0, fft_cfg_buffer, &required_size);
    if (fft_cfg == NULL){
        free(fft_cfg_buffer);
        fft_cfg_buffer = NULL;
        return false;
    }

    //find the bins corresponding to the tuner frequency range.
    //since f = kFs/N --> k = fN/Fs
    min_bin = ((uint32_t)TUNER_MIN_FREQ_HZ * (uint32_t)FFT_BUF_SIZE) / (uint32_t)SAMPLE_RATE_HZ;
    //round the maximum bin upward so the upper frequency limit is included.
    max_bin = (((uint32_t)TUNER_MAX_FREQ_HZ * (uint32_t)FFT_BUF_SIZE) + ((uint32_t)SAMPLE_RATE_HZ - 1U)) / (uint32_t)SAMPLE_RATE_HZ;
    //bin zero is DC and should not be considered as a pitch.
    if (min_bin < 1U) min_bin = 1U;

    /*
     * Parabolic interpolation reads peak_bin + 1, so max_bin must remain
     * below the Nyquist bin.
     */
    uint32_t highest_safe_bin = (FFT_BUF_SIZE / 2U) - 1U;

    if (max_bin > highest_safe_bin) max_bin = highest_safe_bin;

    if (min_bin > max_bin){
        free(fft_cfg_buffer);
        fft_cfg_buffer = NULL;
        fft_cfg = NULL;

        return false;
    }

    initialize_hann_window();
    fft_initialized = 1U;

    return true;
}

/*
 * Release the FFT configuration memory.
 *
 * A continuously running tuner generally does not need to call this.
 */
void fft_deinit(void)
{
    fft_initialized = 0U;
    fft_cfg = NULL;

    if (fft_cfg_buffer != NULL){
        free(fft_cfg_buffer);
        fft_cfg_buffer = NULL;
    }
}

/*
 * Estimate the dominant frequency in an ADC audio frame.
 *
 * audio_buf must point to FFT_BUF_SIZE valid uint16_t samples.
 *
 * Returns:
 *   estimated frequency in Hz
 *   0.0f if the signal is too quiet or the FFT is not initialized
 */
float get_freq_fft(const uint16_t *audio_buf){
    if ((audio_buf == NULL) || (fft_initialized == 0U)) return 0.0f;

    //convert the unsigned ADC signal into a signed, zero-centered signal.
    center_audio_buffer(audio_buf);

    //reject silence and low-level background noise.
    uint32_t average_amplitude = ave_amplitude();
    if (average_amplitude < MIN_SIGNAL_LVL) return 0.0f;

    return fft_run();
}

/*
 * Prepare and run the FFT, find the dominant peak, and return its frequency.
 */
static float fft_run(void)
{
    /*
     * Apply the precomputed Hann window.
     *
     * The window reduces spectral leakage when the sample frame does not
     * contain an integer number of waveform periods.
     */
    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        fft_in[i] = (kiss_fft_scalar)((float)centered_audio_buf[i] *hann_window[i]);
    }

    //fft_out covers frequencies from zero to Nyquist
    kiss_fftr(fft_cfg, fft_in, fft_out);

    uint32_t best_bin = min_bin;
    float best_magnitude_squared = get_magnitude_squared(min_bin);

    //search only the expected guitar-frequency range.
    for (uint32_t bin = min_bin + 1U; bin <= max_bin; bin++)
    {
        float magnitude_squared = get_magnitude_squared(bin);

        if (magnitude_squared > best_magnitude_squared){
            best_magnitude_squared = magnitude_squared;
            best_bin = bin;
        }
    }

    /*
     * Find a fractional bin location using the bins immediately beside the
     * peak. This gives better resolution than returning only integer bins.
     */
    float fractional_bin = interpolate_peak_bin(best_bin);

    return fractional_bin * (float)SAMPLE_RATE_HZ / (float)FFT_BUF_SIZE;
}

/*
 * Calculate the magnitude squared of one complex FFT bin.
 *
 * sqrtf() is unnecessary for peak comparison because:
 *
 *   if A > B, then sqrt(A) > sqrt(B)
 */
static float get_magnitude_squared(uint32_t bin){
    float real = (float)fft_out[bin].r;
    float imag = (float)fft_out[bin].i;

    return (real * real) + (imag * imag);
}

/*
 * Estimate the FFT peak between bins using parabolic interpolation.
 *
 * The estimated offset will normally be in the range -0.5 to +0.5 bins.
 */
static float interpolate_peak_bin(uint32_t peak_bin){
    /*
     * The bins on both sides must exist and should be inside the search
     * region.
     */
    if ((peak_bin <= min_bin) || (peak_bin >= max_bin)){
        return (float)peak_bin;
    }

    float left = get_magnitude_squared(peak_bin - 1U);
    float center = get_magnitude_squared(peak_bin);
    float right = get_magnitude_squared(peak_bin + 1U);

    float denominator = left - (2.0f * center) + right;

    if (fabsf(denominator) < PARABOLA_EPSILON){
        return (float)peak_bin;
    }

    float offset = 0.5f * (left - right) / denominator;

    /*
     * A valid local parabolic estimate should not move more than half a bin.
     */
    if (offset > 0.5f){
        offset = 0.5f;
    }
    else if (offset < -0.5f){
        offset = -0.5f;
    }

    return (float)peak_bin + offset;
}

/*
 * Precompute the Hann window once during initialization.
 */
static void initialize_hann_window(void){
    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        float phase = (2.0f * PI_F * (float)i) / (float)(FFT_BUF_SIZE - 1U);

        hann_window[i] = 0.5f * (1 - cosf(phase));
    }
}

/*
 * Center the unsigned ADC frame around zero by subtracting its mean.
 */
static void center_audio_buffer(const uint16_t *audio_buf){
    uint32_t average = ave_audio_buffer(audio_buf);

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        centered_audio_buf[i] = (int16_t)((int32_t)audio_buf[i] -(int32_t)average);
    }
}

/*
 * Calculate the average ADC value of the frame.
 */
static uint32_t ave_audio_buffer(const uint16_t *audio_buf){
    uint32_t sum = 0U;

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        sum += audio_buf[i];
    }

    return sum / FFT_BUF_SIZE;
}

/*
 * Calculate the mean absolute amplitude of the centered signal.
 */
static uint32_t ave_amplitude(void){
    uint32_t sum = 0U;

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        int32_t sample = centered_audio_buf[i];

        if (sample < 0) sample = -sample;

        sum += (uint32_t)sample;
    }

    return sum / FFT_BUF_SIZE;
}
