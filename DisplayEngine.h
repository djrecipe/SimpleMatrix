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
		DisplayEngine(Config config);
		void Start();
		void Stop();
	private:
		BitmapManager bitmaps;
		Microphone microphone;
		FFT fft;
		RGBMatrix canvas;
		GridTransformer matrix;
		bool running;

		void InitializeAudioDevice(std::string device);
		void InitializeBitmaps(Config config);
		void InitializeFFT();
		void InitializeMatrix(Config config);

		void PrintBitmap(Bitmap* bitmap, int** bins, int bin_count, int bin_depth);
		void PrintBorder();
		void PrintCanvas(int x, int y, const string& message, int r = 255, int g = 255, int b = 255);
		void PrintIdentification();
		void PrintSparkles(float seconds);
};