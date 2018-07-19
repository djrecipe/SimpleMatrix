#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "FFT.h"

#define SAMP_RATE 11025
#define FFT_LOG 9
// # of frequency bins
#define BIN_COUNT 16

using namespace std;

int main(int argc, char** argv)
{
	FFT * fft = new FFT(FFT_LOG, SAMP_RATE);
	delete fft;
	return 0;
}