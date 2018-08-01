#pragma once

#include <math.h>
#include <stdexcept>

#include "gpu_fft.h"
#include "mailbox.h"

// number of fft jobs (always 1?)
#define FFT_JOBS 1
#define FULL_SCALE 100.0
// sigmoid numerator value
// with logarithmic also enabled, decreasing this value allows you to stretch the sigmoid shape along the x-axis
#define SIGMOID_NUMERATOR 10.0
// sigmoid X offset (higher = more low frequency attenuation, more amplitude required to hit gain threshold)
// with logarithmic also enabled, increasing this number will exponentially increase the amount of amplitude required to hit full scale db
#define SIGMOID_OFFSET 25.0
// sigmoid sloep (higher = slopes slower, less effect of attenuation/gain)
// with logarithmic also enabled, increasing this number will result in a sharper corner and a closer resemblance to the 20log10 function
// *** this parameter is of great interest
#define SIGMOID_SLOPE 12.0

enum FFTEvents { NoneFFTEvent = 0, DecreasedAmplitudeFFTEvent = 1, IncreasedAmplitudeFFTEvent = 2, ReturnToLevelFFTEvent = 3 };
enum FFTEventStates { StandardFFTEventState = 0, QuietFFTEventState = 1, LoudFFTEventState = 2};
enum FFTOptions { None = 0, Logarithmic = 1, Sigmoid = 2, Autoscale = 4 };

inline FFTOptions operator|(FFTOptions a, FFTOptions b) { return static_cast<FFTOptions>(static_cast<int>(a) | static_cast<int>(b)); }
inline FFTEvents operator|(FFTEvents a, FFTEvents b) { return static_cast<FFTEvents>(static_cast<int>(a) | static_cast<int>(b)); }

class FFT
{

public:
	FFT(int log, int sample_rate);
	~FFT();

	void Archive(int** bins, int count, int depth);
	void Create(int count, int depth);
	int** Cycle(short* buffer, int display_depth, float seconds);
	void Get(short* buffer, int* bins, int bin_count, int sample_rate);
	FFTEvents GetEvents();
	void Normalize(int** bins, int** normalized_bins, int count, int depth, int total_depth, FFTOptions options, int& min, int& max, int& avg);

private:
	int fftLog;
	int sampleRate;
	int mailbox;
	struct GPU_FFT *fft;

	int binCount;
	int binDepth;
	int** bins;
	int** normalizedBins;

	float eventInvalidated = 0.0;
	float eventResponseOccurred = 0.0;
	FFTEvents fftEvents;
	FFTEventStates fftEventState;
	FFTEventStates fftEventStatePending;

	void DeleteBins();
	FFTEventStates DetectEventState(int min, int max, int avg, float seconds);
	FFTEvents DetectEventTransition(FFTEventStates old_state, FFTEventStates new_state);
	double SigmoidFunction(double value);

};