#include "Microphone.h"

using namespace std;

Microphone::Microphone(std::string device)
{
	fprintf(stderr, "Initializing Audio Device\n");
	const char * device_str = (const char *)device.c_str();
	int err;
	/* Open the PCM device in playback mode */
	if ((err = snd_pcm_open(&pcm_handle, device_str, SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		fprintf(stderr, "cannot open audio device %s (%s)\n", device_str, snd_strerror(err));
		exit(1);
	}

	/* Allocate parameters object and fill it with default values*/
	fprintf(stderr, "\tAllocating parameters object\n");
	snd_pcm_hw_params_t *params;
	if ((err = snd_pcm_hw_params_malloc(&params)) < 0)
	{
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		exit(1);
	}
	fprintf(stderr, "Initializing parameters object\n");
	if ((err = snd_pcm_hw_params_any(pcm_handle, params)) < 0)
	{
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
			snd_strerror(err));
	}
	/* Set parameters */
	if ((err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
	{
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		exit(1);
	}
	if ((err = snd_pcm_hw_params_set_channels(pcm_handle, params, 1)) < 0)
	{
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_rate(pcm_handle, params, SAMP_RATE, 0)) < 0)
	{
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		exit(1);
	}
	if ((err = snd_pcm_hw_params(pcm_handle, params)) < 0)
	{
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		exit(1);
	}

	snd_pcm_hw_params_free(params);

	if ((err = snd_pcm_prepare(pcm_handle)) < 0)
	{
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		exit(1);
	}

	fprintf(stderr, "Successfully Initialized Audio Device\n");
	return;
}

Microphone::~Microphone()
{
	// close audio device
	fprintf(stderr, "\tReleasing audio device\n");
	snd_pcm_drain(this->pcm_handle);
	snd_pcm_close(this->pcm_handle);
	return;
}

int Microphone::GetData(short* buffer, int buffer_size)
{
	int err = snd_pcm_readi(this->pcm_handle, buffer, buffer_size);
	return err;
}