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

enum FFTOptions { None = 0, Logarithmic = 1, Sigmoid = 2, Autoscale = 4 };

inline FFTOptions operator|(FFTOptions a, FFTOptions b) { return static_cast<FFTOptions>(static_cast<int>(a) | static_cast<int>(b)); }

class FFT
{

public:
	FFT(int log);
	~FFT();

	void Get(short* buffer, int* bins, int bin_count, int sample_rate);
	void Normalize(int** bins, int** normalized_bins, int count, int depth, int total_depth, FFTOptions options);

private:
	int fftLog;
	int mailbox;
	struct GPU_FFT *fft;

	double SigmoidFunction(double value);

};