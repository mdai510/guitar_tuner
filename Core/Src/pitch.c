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

#define MIN_SIGNAL_LVL       50U

#define SAMPLE_RATE_HZ       8000U

#define TUNER_MIN_FREQ_HZ    60U
#define TUNER_MAX_FREQ_HZ    400U

#define FFT_BUF_SIZE         MIC_HALF_BUF_SIZE

#define PI_F                 3.14159265358979323846f
#define PARABOLA_EPSILON     1.0e-12f

#define FREQ_ALLOWED_ERR          10
#define FUNDAMENTAL_HARMONIC_PERCENT_THRESHOLD 0.3

#define MIN_LAG (SAMPLE_RATE_HZ / TUNER_MAX_FREQ_HZ)
#define MAX_LAG (SAMPLE_RATE_HZ / TUNER_MIN_FREQ_HZ)

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
static uint32_t min_str_bin = 0U;
static uint32_t max_str_bin = 0U;

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

static void center_audio_buffer(const uint16_t *audio_buf);
static uint32_t ave_audio_buffer(const uint16_t *audio_buf);
static uint32_t ave_amplitude(void);

static void initialize_hann_window(void);
static float get_magnitude_squared(uint32_t bin);
static float interpolate_peak_bin(uint32_t peak_bin);

static int find_period_autocorrelation(const int16_t *centered_buf);
#define PLOT_MIN_FREQ_HZ  40U
#define PLOT_MAX_FREQ_HZ  500U
static void print_fft_spectrum(void);

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
    max_bin = ((uint64_t)TUNER_MAX_FREQ_HZ * FFT_BUF_SIZE) + (SAMPLE_RATE_HZ-1U) / SAMPLE_RATE_HZ;
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
float get_freq_fft(const uint16_t *audio_buf, uint8_t string){
    if ((audio_buf == NULL) || (fft_initialized == 0U)) return 0.0f;
    if(string > 6 || string < 1) return 0.0f;
    //to be able to index into array
    string -= 1;

    //convert the unsigned ADC signal into a signed, zero-centered signal.
    center_audio_buffer(audio_buf);

    //reject silence and low-level background noise.
    uint32_t average_amplitude = ave_amplitude();
    if (average_amplitude < MIN_SIGNAL_LVL) return 0.0f;

    return fft_run(string);
}

float get_freq(const uint16_t *audio_buf, float expected_freq){
	if (audio_buf == NULL) return 0.0f;

	//convert the unsigned ADC signal into a signed, zero-centered signal.
	center_audio_buffer(audio_buf);

	//reject silence and low-level background noise.
	uint32_t average_amplitude = ave_amplitude();
	if (average_amplitude < MIN_SIGNAL_LVL) return 0.0f;

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

static void print_fft_spectrum(void)
{
    uint32_t plot_min_bin =
        (PLOT_MIN_FREQ_HZ * FFT_BUF_SIZE) / SAMPLE_RATE_HZ;

    uint32_t plot_max_bin =
        (PLOT_MAX_FREQ_HZ * FFT_BUF_SIZE) / SAMPLE_RATE_HZ;

    if (plot_max_bin > (FFT_BUF_SIZE / 2U))
    {
        plot_max_bin = FFT_BUF_SIZE / 2U;
    }

    printf("BEGIN\r\n");

    for (uint32_t bin = plot_min_bin; bin <= plot_max_bin; bin++)
    {
        float real = (float)fft_out[bin].r;
        float imag = (float)fft_out[bin].i;

        float magnitude_squared =
            (real * real) + (imag * imag);

        float frequency =
            ((float)bin * (float)SAMPLE_RATE_HZ) /
            (float)FFT_BUF_SIZE;

        printf("%.3f,%.6e\r\n",
               frequency,
               magnitude_squared);
    }

    printf("END\r\n");
}

/*
 * Prepare and run the FFT, find the dominant peak, and return its frequency.
 */
static float fft_run(uint8_t string)
{
	//apply hann window
    for (uint32_t i = 0U; i < FFT_BUF_SIZE; i++){
        fft_in[i] = (kiss_fft_scalar)((float)centered_audio_buf[i] * hann_window[i]);
    }

    //fft_out covers frequencies from zero to Nyquist
    kiss_fftr(fft_cfg, fft_in, fft_out);

    min_str_bin = ((uint64_t)string_freq_mins[string] * FFT_BUF_SIZE) / SAMPLE_RATE_HZ;
    max_str_bin = ((uint64_t)string_freq_maxs[string] * FFT_BUF_SIZE) / SAMPLE_RATE_HZ;

    uint32_t best_bin = min_str_bin;
    float best_magnitude_squared = get_magnitude_squared(min_str_bin);

    //search only the expected guitar-frequency range +- a few bins
    for (uint32_t bin = min_str_bin - 1U; bin <= max_str_bin + 1U; bin++)
    {
        float magnitude_squared = get_magnitude_squared(bin);

        if (magnitude_squared > best_magnitude_squared){
            best_magnitude_squared = magnitude_squared;
            best_bin = bin;
        }
    }

    //If best bin is +- FREQ_ALLOWED_ERR of the expected frequency, interpolate and return the frequency
    //If not, check the half frequency (best_bin / 2) and the bins around it
    //If there is a peak there that is above <threshold> percent of the best bin, return that frequency instead
    //If that is still not within the expected range, return 0.0f
    /*
    float best_freq = (float)best_bin * bin_resolution;
    printf("best freq: %f\r\n", best_freq);
    //check if outside expected range
    if((best_freq < expected_freq - FREQ_ALLOWED_ERR) || (best_freq > expected_freq + FREQ_ALLOWED_ERR)){
    	uint32_t half_bin = best_bin / 2U;

		uint32_t error_bins =
			(uint32_t)ceilf(
				(float)FREQ_ALLOWED_ERR / bin_resolution
			);

		uint32_t half_min_bin =
			(half_bin > error_bins)
				? half_bin - error_bins
				: 1U;

		uint32_t half_max_bin = half_bin + error_bins;

		/*
		 * Keep the temporary search inside the permanent
		 * tuner frequency range.
		 */
         /*
		if (half_min_bin < min_bin)
		{
			half_min_bin = min_bin;
		}

		if (half_max_bin > max_bin)
		{
			half_max_bin = max_bin;
		}

		/*
		 * If the half-frequency region is below the supported
		 * tuner range, reject it.
		 */
         /*
		if (half_min_bin > half_max_bin)
		{
			return 0.0f;
		}

		uint32_t fundamental_bin = half_min_bin;
		float fundamental_mag_squared =
			get_magnitude_squared(fundamental_bin);

		for (uint32_t bin = half_min_bin + 1U;
			 bin <= half_max_bin;
			 bin++)
		{
			float magnitude_squared =
				get_magnitude_squared(bin);

			if (magnitude_squared > fundamental_mag_squared)
			{
				fundamental_mag_squared = magnitude_squared;
				fundamental_bin = bin;
			}
		}

		if (fundamental_mag_squared >
			best_magnitude_squared *
			FUNDAMENTAL_HARMONIC_PERCENT_THRESHOLD)
		{
			best_bin = fundamental_bin;
		}
		else
		{
			return 0.0f;
		}
    } */

    //Interpolate the original or halved bin to get a more accurate frequency estimate
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
