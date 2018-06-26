#pragma once

#include <math.h>
#include <stdexcept>

#include "gpu_fft.h"
#include "mailbox.h"

// number of fft jobs (always 1?)
#define FFT_JOBS 1

class FFT
{

public:
	FFT(int log);
	~FFT();

	void GetBins(short* buffer, int* bins, int bin_count, int sample_rate);

private:
	int fftLog;
	int mailbox;
	struct GPU_FFT *fft;

};