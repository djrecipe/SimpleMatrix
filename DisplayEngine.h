#pragma once

#include "BitmapManager.h"
#include "Config.h"
#include "FFT.h"
#include "glcdfont.h"
#include "GridTransformer.h"
#include "Microphone.h"

#include <cstdint>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <time.h>
#include <unistd.h>

#include <led-matrix.h>

using namespace std;
using namespace rgb_matrix;

#define FFT_LOG 9
// capture sample rate
#define SAMP_RATE 11025
// # of frequency bins
#define BIN_COUNT 16
// history count for each frequency bin (for normalization)
#define TOTAL_BIN_DEPTH 64
// history count for each frequency bin (for display)
#define BIN_DEPTH 8

enum DisplayModes { BitmapDisplayMode = 0, LowAmplitudeDisplayMode = 1, HighAmplitudeDisplayMode = 2 };

class DisplayEngine
{
	public:
		DisplayEngine(Config& config);
		~DisplayEngine();
		void Start();
		void Stop();
	private:
		BitmapManager* bitmaps;
		Microphone* microphone;
		FFT* fft;
		RGBMatrix* canvas;
		GridTransformer* matrix;
		bool running;
		
		float contractingCircleReset = 0.0;

		void InitializeAudioDevice(std::string device);
		void InitializeBitmaps(Config& config);
		void InitializeFFT();
		void InitializeMatrix(Config& config);

		void PrintBitmap(Bitmap* bitmap, float red_gain, float green_gain, float blue_gain);
		void PrintBorder(float red_gain, float green_gain, float blue_gain);
		void PrintCanvas(int x, int y, const string& message, int r = 255, int g = 255, int b = 255);
		void PrintContractingCircle(float seconds, float red_gain, float green_gain, float blue_gain);
		void PrintIdentification();
};