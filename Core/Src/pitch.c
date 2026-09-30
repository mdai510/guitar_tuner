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
#include <sys/stat.h>
#include <stdio.h>

#define MIN_SIGNAL_LVL       40U

#define SAMPLE_RATE_HZ       MIC_SAMPLE_RATE_HZ

#define TUNER_MIN_FREQ_HZ    50U
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
 * FFT bin resolution
*/
static float bin_resolution = (float)SAMPLE_RATE_HZ / (float)FFT_BUF_SIZE;

/*
 * Indicates whether fft_init() completed successfully.
 */
static uint8_t fft_initialized = 0U;

static float string_freq_mins[6] = {STRING_1_MIN_HZ, STRING_2_MIN_HZ, STRING_3_MIN_HZ, STRING_4_MIN_HZ, STRING_5_MIN_HZ, STRING_6_MIN_HZ};
static float string_freq_maxs[6] = {STRING_1_MAX_HZ, STRING_2_MAX_HZ, STRING_3_MAX_HZ, STRING_4_MAX_HZ, STRING_5_MAX_HZ, STRING_6_MAX_HZ};

/* Private function declarations */

static float fft_run(uint8_t string);

static void center_audio_buffer(const int16_t *audio_buf);
static int32_t ave_audio_buffer(const int16_t *audio_buf);
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
    min_bin = ((uint64_t)TUNER_MIN_FREQ_HZ * FFT_BUF_SIZE) / SAMPLE_RATE_HZ;
    //round the maximum bin upward so the upper frequency limit is included.
    max_bin = (uint32_t)((((uint64_t)TUNER_MAX_FREQ_HZ * FFT_BUF_SIZE) + SAMPLE_RATE_HZ - 1U) /SAMPLE_RATE_HZ);
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
float get_freq_fft(const int16_t *audio_buf, uint8_t string){
    if ((audio_buf == NULL) || (fft_initialized == 0U)) return 0.0f;
    if(string > 6 || string < 1) return 0.0f;
    //to be able to index into array
    string -= 1;

    //convert the unsigned ADC signal into a signed, zero-centered signal.
    center_audio_buffer(audio_buf);

    //reject silence and low-level background noise.
    uint32_t average_amplitude = ave_amplitude();
    printf("avg_abs=%lu\r\n", (unsigned long)average_amplitude);
    if (average_amplitude < MIN_SIGNAL_LVL) return 0.0f;

    return fft_run(string);
}

/*
 * Prepare and run the FFT, find the dominant peak, and return its frequency.
 */
static float fft_run(uint8_t string){
	// Apply the Hann window before transforming the frame.
    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++) {
        fft_in[i] = (kiss_fft_scalar)(
            (float)centered_audio_buf[i] * hann_window[i]);
    }

    kiss_fftr(fft_cfg, fft_in, fft_out);

    uint32_t search_min = (uint32_t)floorf(
        string_freq_mins[string] / bin_resolution);
    uint32_t search_max = (uint32_t)ceilf(
        string_freq_maxs[string] / bin_resolution);

    /* Include one bin outside the nominal range for edge detection. */
    if (search_min > 1U) {
        search_min--;
    }

    uint32_t highest_safe_bin = (FFT_BUF_SIZE / 2U) - 1U;
    if (search_max < highest_safe_bin) {
        search_max++;
    }
    else {
        search_max = highest_safe_bin;
    }

    /* Keep DC out of the pitch search and reject an invalid range. */
    if (search_min < 1U) {
        search_min = 1U;
    }
    if ((search_min > highest_safe_bin) || (search_min > search_max)) {
        return 0.0f;
    }

    uint32_t best_bin = search_min;
    float best_power = get_magnitude_squared(best_bin);

    for (uint32_t bin = search_min + 1U; bin <= search_max; bin++) {
        float power = get_magnitude_squared(bin);

        if (power > best_power) {
            best_power = power;
            best_bin = bin;
        }
    }

    float fractional_bin = interpolate_peak_bin(best_bin);
    return fractional_bin * bin_resolution;
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
	if ((peak_bin == 0U) || (peak_bin >= (FFT_BUF_SIZE / 2U))){
		return (float)peak_bin;
	}

    // Convert the magnitudes of the peak bin and its neighbors to the logarithmic scale for interpolation.
	float left = logf(get_magnitude_squared(peak_bin - 1U) + PARABOLA_EPSILON);
	float center = logf(get_magnitude_squared(peak_bin) + PARABOLA_EPSILON);
	float right = logf(get_magnitude_squared(peak_bin + 1U) + PARABOLA_EPSILON);

    // Calculate the denominator of the parabolic interpolation formula.
	float denominator = left - (2.0f * center) + right;
    // If the denominator is too small, the interpolation would be unreliable.
	if(fabsf(denominator) < PARABOLA_EPSILON){
		return (float)peak_bin;
	}

    // Calculate the offset of the peak within the bin using parabolic interpolation.
	float offset = 0.5f * (left - right) / denominator;
	if (offset > 0.5f) offset = 0.5f;
	else if (offset < -0.5f) offset = -0.5f;

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
static void center_audio_buffer(const int16_t *audio_buf){
    int32_t average = ave_audio_buffer(audio_buf);

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        centered_audio_buf[i] = (int16_t)((int32_t)audio_buf[i] - average);
    }
}

/*
 * Calculate the average ADC value of the frame.
 */
static int32_t ave_audio_buffer(const int16_t *audio_buf){
    int64_t sum = 0;

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        sum += audio_buf[i];
    }

    return (int32_t)(sum / (int64_t)FFT_BUF_SIZE);
}

/*
 * Calculate the mean absolute amplitude of the centered signal.
 */
static uint32_t ave_amplitude(void){
    uint64_t sum = 0;

    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        int32_t sample = centered_audio_buf[i];

        if (sample < 0) sample = -sample;

        sum += sample;
    }

    return (uint32_t)(sum / FFT_BUF_SIZE);
}
