#include "FFT.h"

using namespace std;

FFT::FFT(int fft_log, int sample_rate)
{
	this->binCount = 0;
	this->binDepth = 0;
	this->bins = NULL;
	this->eventResponseOccurred = 0.0;
	this->normalizedBins = NULL;
	this->fftEvents = NoneFFTEvent;
	this->fftEventState = StandardFFTEventState;
	this->fftEventStatePending = StandardFFTEventState;
	this->fftLog = fft_log;
	this->eventInvalidated = 0.0;
	this->sampleRate = sample_rate;
	this->mailbox = mbox_open();
	this->minimumStateDuration = 0.00001;
	int ret = gpu_fft_prepare(this->mailbox, this->fftLog, GPU_FFT_REV, FFT_JOBS, &(this->fft));

	switch (ret)
	{
		case -1: throw runtime_error("Unable to enable V3D. Please check your firmware is up to date.\n");
		case -2: throw runtime_error("log2_N=%d not supported.  Try between 8 and 22.\n");
		case -3: throw runtime_error("Out of memory.  Try a smaller batch or increase GPU memory.\n");
		case -4: throw runtime_error("Unable to map Videocore peripherals into ARM memory space.\n");
		case -5: throw runtime_error("Can't open libbcm_host.\n");
	}
	return;
}

void FFT::Analyze(int* bins, int count, int& min, int& max, int& avg)
{
	// reset output variables
	min = 9999999, max = -9999999, avg = 0;

	// iterate through all frequency bins of a single bin depth
	for(int i=0; i<count; i++)
	{
		// calculate max among all frequency bins of this depth
		max = fmax(max, bins[i]);
		// calculate min among all frequency bins of this depth
		min = fmin(min, bins[i]);
		// calculate average among all frequency bins of this depth
		avg += bins[i];
	}

	// finish calculating average
	avg /= count;
	return;
}

void FFT::Archive(int** bins, int count, int depth)
{
	// iterate through all bin depths
	for (int i = depth - 1; i>0; i--)
	{
		// iterate through all frequencies of a given bin depth
		for (int j = 0; j<count; j++)
		{
			// move new entries (high index) backwards (towards index 0)
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
	// make space for new bin acquisition
	this->Archive(this->bins, this->binCount, this->binDepth);
	// acquire new bin values
	this->Get(data, this->bins[0], this->binCount, this->sampleRate);

	// normalize bins
	FFTOptions options = Logarithmic | Autoscale | Sigmoid;
	int min = 0, max = 0, avg = 0;
	this->Normalize(this->bins, this->normalizedBins, this->binCount, display_depth, this->binDepth, options);

	// perform analysis and detect events
	this->Analyze(this->normalizedBins[0], this->binCount, min, max, avg);
	FFTEventStates new_event_state = this->DetectEventState(min, max, avg, seconds);
	FFTEvents new_event = this->DetectEventTransition(this->fftEventState, new_event_state);
	this->fftEvents = new_event;
	this->fftEventState = new_event_state;
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

FFTEventStates FFT::DetectEventState(int min, int max, int avg, float seconds)
{
	FFTEventStates previous_pending = this->fftEventStatePending;
	//fprintf(stderr, "Min: %d | Max: %d | Avg: %d | ", min, max, avg);

	float min_sustain_time = 0.001;		// minimum amount of time current behavior needs to be sustained in order to switch states
	float new_min_state_duration = 0.001;	// minimum amount of time new state will last IF new state is confirmed
	
	// detect events
	if (max < 20)
	{	// check for quiet
		this->fftEventStatePending = QuietFFTEventState;
		min_sustain_time = 0.1;
		new_min_state_duration = 0.001;
		//fprintf(stderr, "Quiet\n");
	}
	else if (max > 85)
	{	// check for loud
		this->fftEventStatePending = LoudFFTEventState;
		min_sustain_time = 0.01;
		new_min_state_duration = 0.2;
		//fprintf(stderr, "Loud\n");
	}
	else
	{	// default to standard
		this->fftEventStatePending = StandardFFTEventState;
		min_sustain_time = 0.001;
		new_min_state_duration = 0.001;
		//fprintf(stderr, "Standard\n");
	}
	
	// reset timer upon change
	if (this->fftEventStatePending != previous_pending)
	{
		this->eventInvalidated = seconds;
	}

	// confirm new state
	if (seconds - this->eventInvalidated > min_sustain_time && seconds - this->eventResponseOccurred > this->minimumStateDuration)
	{
		this->eventInvalidated = seconds;
		this->eventResponseOccurred = seconds;
		this->minimumStateDuration = new_min_state_duration;
		return this->fftEventStatePending;
	}
	return this->fftEventState;
}

FFTEvents FFT::DetectEventTransition(FFTEventStates old_state, FFTEventStates new_state)
{
	FFTEvents value = NoneFFTEvent;
	switch (old_state)
	{
		default:
		case StandardFFTEventState:
			switch (new_state)
			{
				default:
				case StandardFFTEventState:
					break;
				case QuietFFTEventState:
					value = DecreasedAmplitudeFFTEvent; // medium -> low
					break;
				case LoudFFTEventState:
					value = IncreasedAmplitudeFFTEvent; // medium -> high
					break;
			}
			break;
		case QuietFFTEventState:
			switch (new_state)
			{
				default:
				case StandardFFTEventState:
					value = ReturnToLevelFFTEvent;
					break;
				case QuietFFTEventState:
					break;
				case LoudFFTEventState:
					value = IncreasedAmplitudeFFTEvent;
					break;
			}
			break;
		case LoudFFTEventState:
			switch (new_state)
			{
				default:
				case StandardFFTEventState:
					value = ReturnToLevelFFTEvent;
					break;
				case QuietFFTEventState:
					value = DecreasedAmplitudeFFTEvent;
					break;
				case LoudFFTEventState:
					break;
			}
			break;
	}
	return value;
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

void FFT::GetColorGains(float& red_gain, float& green_gain, float& blue_gain)
{
	red_gain = this->redGain;
	green_gain = this->greenGain;
	blue_gain = this->blueGain;
	return;
}

FFTEvents FFT::GetEvents()
{
	FFTEvents value = this->fftEvents;
	this->fftEvents = NoneFFTEvent;
	return value;
}

void FFT::Normalize(int** bins, int** normalized_bins, int count, int depth, int total_depth, FFTOptions options)
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
			else
			{
				normalized_bins[i][j] = bins[i][j];
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
	float red_gain = 0.0, green_gain = 0.0, blue_gain = 0.0;
	// normalize displayed bins only
	for (i = 0; i<depth; i++)
	{
		// calculate gain decay (based on age)
		float decay = (float)(depth - i) / (float)depth;

		// iterate through freq bins of a given depth
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

			// apply sigmoid approximation (emphasize peaks and scale 0.0-100.0)
			if ((options & Sigmoid) != 0)
			{
				normalized_bins[i][j] = SigmoidFunction((double)normalized_bins[i][j]);
			}

			// calculate base gain (based on bin amplitude) (0.0 -> 2.0)
			float bin_gain = (float)bins[i][j] / (FULL_SCALE / 2.0);
			// increases with bin frequency (0.1 -> 1.0)
			blue_gain = fmax(bin_gain * ((float)(j + 1) / (float)count)*decay, blue_gain);
			// increases towards center frequency (0.1 -> 1.0 -> 0.1)
			green_gain = fmax(bin_gain * ((float)(count / 2 - abs((j + 1) - count / 2)) / (float)(count / 2))*decay, green_gain);
			// decreases with bin frequency (1.0 -> 0.1)
			red_gain = fmax(bin_gain * ((float)(count - j) / (float)count)*decay, red_gain);
		}
	}

	// remember color gains
	this->redGain = red_gain;
	this->greenGain = green_gain;
	this->blueGain = blue_gain;
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