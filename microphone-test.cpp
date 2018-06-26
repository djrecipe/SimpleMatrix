#include <cstdint>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdexcept>

#include "Microphone.h"

#define FFT_LOG 9
// capture sample rate
#define SAMP_RATE 11025

using namespace std;

int main(int argc, char** argv)
{
	Microphone * microphone = new Microphone("plughw:1,0");
	int err;
	int buffer_size = 1 << FFT_LOG;
	short buf[buffer_size];
	while ((err = microphone->GetData(buf, buffer_size)) == buffer_size)
	{
		float max = 0.0;
		for (int i = 0; i < buffer_size; i++)
		{
			max = fmax(max, buf[i]);
		}
		fprintf(stderr, "\tMax: %f\n", max);
	}
	fprintf(stderr, "\tERROR: %d\n", err);
	delete microphone;
	return 0;
}