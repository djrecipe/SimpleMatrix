#pragma once

#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <stdexcept>

#include <sstream>

// capture sample rate
#define SAMP_RATE 11025

class Microphone
{

public:
	Microphone(char* device);
	~Microphone();

	int GetData(short* buffer, int buffer_size);

private:
	snd_pcm_t * pcm_handle;
};