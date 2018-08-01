#include "FFT.h"

FFT::FFT(int fft_log, int sample_rate)
{
	this->binCount = 0;
	this->binDepth = 0;
	this->bins = NULL;
	this->eventResponseOccurred = 0.0;
	this->normalizedBins = NULL;
	this->fftEvents = NoneFFTEvent;
	this->fftLog = fft_log;
	this->sampleRate = sample_rate;
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
	return;
}

void FFT::Archive(int** bins, int count, int depth)
{
	for (int i = depth - 1; i>0; i--)
	{
		for (int j = 0; j<count; j++)
		{
			bins[i][j] = bins[i - 1][j];
		}
	}
	return;
}

void FFT::Create(int count, int depth)
{
	this->DeleteBins();
	this->binCount = count;
	this->binDepth = depth;
	this->bins = new int*[depth];
	this->normalizedBins = new int*[depth];
	int i = 0, j = 0;
	for (i = 0; i<depth; i++)
	{
		this->bins[i] = new int[count];
		this->normalizedBins[i] = new int[count];
		for (j = 0; j<count; j++)
		{
			this->bins[i][j] = this->normalizedBins[i][j] = 0;
		}
	}
	return;
}

int** FFT::Cycle(short* data, int display_depth, float seconds)
{
	this->Archive(this->bins, this->binCount, this->binDepth);
	this->Get(data, this->bins[0], this->binCount, this->sampleRate);
	FFTOptions options = Logarithmic | Autoscale | Sigmoid;
	int min = 0, max = 0, avg = 0;
	this->Normalize(this->bins, this->normalizedBins, this->binCount, display_depth, this->binDepth, options, min, max, avg);
	if (this->DetectRangeEvent(min, max, avg, seconds))
		this->fftEvents = LimitedRangeFFTEvent;
	return this->normalizedBins;
}

void FFT::DeleteBins()
{
	for (int i = 0; i < this->binDepth; i++)
	{
		delete this->bins[i];
		delete this->normalizedBins[i];
	}
	delete this->bins;
	delete this->normalizedBins;
	this->bins = NULL;
	this->normalizedBins = NULL;
	this->binCount = 0;
	this->binDepth = 0;
	return;
}

bool FFT::DetectRangeEvent(int min, int max, int avg, float seconds)
{
	//fprintf(stderr, "%f\n", seconds);
	if (seconds - this->eventResponseOccurred < 10.0)
	{
		this->rangeEventInvalidated = seconds;
		return false;
	}
	//fprintf(stderr, "Min: %d | Max: %d | Avg: %d\n", min, max, avg);
	if (avg > 10)
		this->rangeEventInvalidated = seconds;
	if (seconds - this->rangeEventInvalidated > 2)
	{
		this->rangeEventInvalidated = seconds;
		this->eventResponseOccurred = seconds;
		return true;
	}
	return false;
}

void FFT::Get(short* buffer, int* bins, int bin_count, int sample_rate)
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

FFTEvents FFT::GetEvents()
{
	FFTEvents value = this->fftEvents;
	this->fftEvents = NoneFFTEvent;
	return value;
}

void FFT::Normalize(int** bins, int** normalized_bins, int count, int depth, int total_depth, FFTOptions options, int& min, int& max, int& avg)
{
	// initialize parameters
	int i = 0, j = 0;
	int full_min = 999999999, full_max = -999999999;
	// calculate highest and lowest peaks of a given frequency bin
	for (j = 0; j<count; j++)
	{
		// reset current frequency bin max
		int bin_max = 0;
		// iterate through all bin history (even not displayed ones)
		for (i = 0; i<total_depth; i++)
		{
			// convert to db
			if ((options & Logarithmic) != 0)
			{
				normalized_bins[i][j] = 20.0 * log10(bins[i][j]);
			}
			// ignore negative values/db
			normalized_bins[i][j] = fmax(normalized_bins[i][j], 0);
			// calculate max for all history of current frequency bin
			bin_max = fmax(bin_max, normalized_bins[i][j]);
			// calculate max for all history of all frequency bins
			full_max = fmax(full_max, normalized_bins[i][j]);
		}
		// calculate smallest peak occurring to any given frequency bin over all history
		full_min = fmin(full_min, bin_max);
	}
	// calculate range 
	int range = full_max - full_min;
	avg = 0;
	min = 999999999, max = -999999999;
	// normalize displayed bins only
	for (i = 0; i<depth; i++)
	{
		// reset current frequency bin max
		int bin_max = 0;
		for (j = 0; j<count; j++)
		{

			// autoscale
			if ((options & Autoscale) != 0)
			{
				// calculate ratio (0.0 -> 1.0)
				float ratio = (float)(normalized_bins[i][j] - full_min) / (float)range;
				// cull negative values (i.e. amplitudes which are underrange)
				ratio = fmax(ratio, 0.0);
				//fprintf(stderr, "Ratio: %f\n", ratio);
				normalized_bins[i][j] = FULL_SCALE * ratio;
			}

			// calculate max for all history of current frequency bin
			bin_max = fmax(bin_max, normalized_bins[i][j]);
			// calculate max for all history of all frequency bins
			max = fmax(max, normalized_bins[i][j]);
			avg += normalized_bins[i][j];

			// apply sigmoid approximation (emphasize peaks and scale 0.0-100.0)
			if ((options & Sigmoid) != 0)
			{
				normalized_bins[i][j] = SigmoidFunction((double)normalized_bins[i][j]);
			}
		}
		// calculate smallest peak occurring to any given frequency bin over all history
		min = fmin(min, bin_max);
	}
	avg /= (depth * count);
	return;
}

double FFT::SigmoidFunction(double value)
{
	/* in order to approach a desired full scale value, the left-hand side constant (in the demoninator)
	needs to be equal to the numerator divided by the desired full scale */
	double constant = SIGMOID_NUMERATOR / FULL_SCALE;
	return SIGMOID_NUMERATOR / (constant + pow(M_E, -1.0*((value - SIGMOID_OFFSET) / SIGMOID_SLOPE)));
}

FFT::~FFT()
{
	gpu_fft_release(this->fft);
	this->DeleteBins();
	return;
}