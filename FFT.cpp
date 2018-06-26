#include "FFT.h"

FFT::FFT(int fft_log)
{
	fprintf(stderr, "Initializing FFT\n");
	this->fftLog = fft_log;
	this->mailbox = mbox_open();
	int ret = gpu_fft_prepare(this->mailbox, this->fftLog, GPU_FFT_REV, FFT_JOBS, &(this->fft));

	switch (ret)
	{
		case -1: throw std::runtime_error("Unable to enable V3D. Please check your firmware is up to date.\n");
		case -2: throw std::runtime_error("log2_N=%d not supported.  Try between 8 and 22.\n");
		case -3: throw std::runtime_error("Out of memory.  Try a smaller batch or increase GPU memory.\n");
		case -4: throw std::runtime_error("Unable to map Videocore peripherals into ARM memory space.\n");
		case -5: throw std::runtime_error("Can't open libbcm_host.\n");
	}
	fprintf(stderr, "Successfully Initialized FFT\n");
	return;
}

void FFT::GetBins(short* buffer, int* bins, int bin_count, int sample_rate)
{
	// initialize parameters
	int full_count = 1 << this->fftLog;
	float frequencies[bin_count + 1] = { 20.0,50.0,100.0,150.0,200.0,250.0,300.0,350.0,400.0,500.0,600.0,750.0,1000.0,2000.0,3000.0,5000.0,7500.0 };
	float maxs[bin_count];
	float max = 0.0;
	float value = 0.0;
	int i = 0, j = 0;

	// assign fft input
	for (i = 0; i<full_count; i++)
	{
		fft->in[i].re = (float)buffer[i];
		fft->in[i].im = 0.0;
	}
	// reset maxs
	for (j = 0; j<bin_count; j++)
	{
		maxs[j] = 0.0;
	}

	// execute fft
	gpu_fft_execute(this->fft); // call one or many times

	// calculate results
	for (i = 0; i<full_count / 2; i++)
	{
		// calculate frequency
		float frequency = (float)i * ((float)(sample_rate) / (float)(full_count));
		// iterate through frequency bins
		// sort results into discrete frequency bins
		for (j = 0; j<bin_count; j++)
		{
			if (frequency >= frequencies[j] && frequency < frequencies[j + 1])
			{
				// calculate result vector
				value = sqrt(pow(this->fft->out[i].re, 2) + pow(this->fft->out[i].re, 2));
				maxs[j] = fmax(maxs[j], value);
				bins[j] = (int)maxs[j];
				max = fmax(maxs[j], max);
				break;
			}
		}
	}
	return;
}

FFT::~FFT()
{
	// release FFT
	fprintf(stderr, "\tReleasing fft data\n");
	gpu_fft_release(this->fft);
	return;
}