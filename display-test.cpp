// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Program to aid in the testing of LED matrix chains.
// Author: Tony DiCola
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <led-matrix.h>
#include <signal.h>
#include <unistd.h>

#include "Config.h"
#include "glcdfont.h"
#include "GridTransformer.h"
#include "BitmapSet.h"
#include "Microphone.h"
#include "FFT.h"

#define FFT_LOG 9
// capture sample rate
#define SAMP_RATE 11025
// # of frequency bins
#define BIN_COUNT 16
// history count for each frequency bin (for normalization)
#define TOTAL_BIN_DEPTH 64
// history count for each frequency bin (for display)
#define BIN_DEPTH 8
#define FULL_SCALE 100.0
// sigmoid numerator value
// with logarithmic also enabled, decreasing this value allows you to stretch the sigmoid shape along the x-axis
#define SIGMOID_NUMERATOR 10.0
// sigmoid X offset (higher = more low frequency attenuation, more amplitude required to hit gain threshold)
// with logarithmic also enabled, increasing this number will exponentially increase the amount of amplitude required to hit full scale db
#define SIGMOID_OFFSET 25.0
// sigmoid sloep (higher = slopes slower, less effect of attenuation/gain)
// with logarithmic also enabled, increasing this number will result in a sharper corner and a closer resemblance to the 20log10 function
// *** this parameter is of great interest
#define SIGMOID_SLOPE 12.0

using namespace std;
using namespace rgb_matrix;


enum FFTOptions { None = 0, Logarithmic = 1, Sigmoid = 2, Autoscale = 4 };

inline FFTOptions operator|(FFTOptions a, FFTOptions b) { return static_cast<FFTOptions>(static_cast<int>(a) | static_cast<int>(b)); }

// Global to keep track of if the program should run.
// Will be set false by a SIGINT handler when ctrl-c is
// pressed, then the main loop will cleanly exit.
volatile bool running = true;

void printCanvas(GridTransformer* canvas, int x, int y, const string& message,
                 int r = 255, int g = 255, int b = 255) {
  // Loop through all the characters and print them starting at the provided
  // coordinates.
  for (auto c: message) {
    // Loop through each column of the character.
    for (int i=0; i<5; ++i) {
      unsigned char col = glcdfont[c*5+i];
      x += 1;
      // Loop through each row of the column.
      for (int j=0; j<8; ++j) {
        // Put a pixel for each 1 in the column byte.
        if ((col >> j) & 0x01) {
          canvas->SetPixel(x, y+j, r, g, b);
        }
      }
    }
    // Add a column of padding between characters.
    x += 1;
  }
}

void PrintBitmap(GridTransformer* canvas, Bitmap* bitmap, int bins[][BIN_COUNT])
{
	// initialize parameters
	int x = 0, y = 0, r = 0, g = 0, b = 0, i = 0, j = 0;
	float decay = 0.0, bin_gain = 0.0, red_gain = 0.0, green_gain = 0.0, blue_gain = 0.0;
	unsigned char* data = bitmap->GetData();
	// calculate color gains based on audio frequency content
	// iterate through bins, depthwise
	for (i = 0; i<BIN_DEPTH; i++)
	{
		// calculate decay (based on age)
		decay = (float)(BIN_DEPTH - i) / (float)BIN_DEPTH;
		// iterate through all frequency bins (of a given depth)
		for (j = 0; j<BIN_COUNT; j++)
		{
			// calculate base gain (based on bin amplitude) (0.0 -> 2.0)
			bin_gain = (float)bins[i][j] / (FULL_SCALE / 2.0);
			// increases with bin frequency (0.1 -> 1.0)
			blue_gain = fmax(bin_gain * ((float)(j + 1) / (float)BIN_COUNT)*decay, blue_gain);
			// increases towards center frequency (0.1 -> 1.0 -> 0.1)
			green_gain = fmax(bin_gain * ((float)(BIN_COUNT / 2 - abs((j + 1) - BIN_COUNT / 2)) / (float)(BIN_COUNT / 2))*decay, green_gain);
			// decreases with bin frequency (1.0 -> 0.1)
			red_gain = fmax(bin_gain * ((float)(BIN_COUNT - j) / (float)BIN_COUNT)*decay, red_gain);
		}
	}
	// iterate through each pixel in the bitmap
	for (x = 0; x<bitmap->GetWidth(); x++)
	{
		for (y = 0; y<bitmap->GetHeight(); y++)
		{
			// check if pixel is already occupied
			//if (this->grid->GetPixelState(x, y))
			//	continue;
			// calculate index into single dimensional array of pixel data
			int index = y * bitmap->GetWidth() * 3 + x * 3;
			// check for black pixel (reduce calculation time)
			if (((int)data[index] < 1) && ((int)data[index + 1] < 1) && ((int)data[index + 2] < 1))
			{
				canvas->SetPixel(x, y, 0, 0, 0);
				continue;
			}
			// calculate color
			r = (float)data[index] * red_gain;
			g = (float)data[index + 1] * green_gain;
			b = (float)data[index + 2] * blue_gain;
			// draw
			canvas->SetPixel(x, y, r, g, b);
		}
	}
	return;
}

static void sigintHandler(int s)
{
  running = false;
}

double SigmoidFunction(double value)
{
	/*
	through testing I have found that in order to approach a desired full scale value, the left-hand side constant (in the demoninator)
	needs to be equal to the numerator divided by the desired full scale
	*/
	double constant = SIGMOID_NUMERATOR / FULL_SCALE;
	return SIGMOID_NUMERATOR / (constant + pow(M_E, -1.0*((value - SIGMOID_OFFSET) / SIGMOID_SLOPE)));
}

void NormalizeBins(int bins[][BIN_COUNT], int normalized_bins[][BIN_COUNT], FFTOptions options)
{
	// initialize parameters
	int i = 0, j = 0;
	int min = 999999999, max = -999999999, bin_max = -999999999;
	// calculate highest and lowest peaks of a given frequency bin
	for (j = 0; j<BIN_COUNT; j++)
	{
		// reset current frequency bin max
		bin_max = 0;
		// iterate through all bin history (even not displayed ones)
		for (i = 0; i<TOTAL_BIN_DEPTH; i++)
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
			max = fmax(max, normalized_bins[i][j]);
		}
		// calculate smallest peak occurring to any given frequency bin over all history
		min = fmin(min, bin_max);
	}
	// calculate range 
	int range = max - min;
	float ratio = 0.5;
	// normalize displayed bins only
	int abs_max = 0;
	for (i = 0; i<BIN_DEPTH; i++)
	{
		for (j = 0; j<BIN_COUNT; j++)
		{
			// autoscale
			if ((options & Autoscale) != 0)
			{
				// calculate ratio (0.0 -> 1.0)
				ratio = (float)(normalized_bins[i][j] - min) / (float)range;
				// cull negative values (i.e. amplitudes which are underrange)
				ratio = fmax(ratio, 0.0);
				//fprintf(stderr, "%f\n", ratio);
				normalized_bins[i][j] = FULL_SCALE * ratio;
			}
			// apply sigmoid approximation (emphasize peaks and scale 0.0-100.0)
			if ((options & Sigmoid) != 0)
			{
				normalized_bins[i][j] = SigmoidFunction((double)normalized_bins[i][j]);
				abs_max = fmax(normalized_bins[i][j], abs_max);
			}
		}
	}
	return;
}

int main(int argc, char** argv)
{
  try
  {
	const clock_t begin_time = clock();
    Config config(argc >= 2 ? argv[1] : "/dev/null");


	// Initialize GPIO
	rgb_matrix::GPIO io;

	if (!io.Init())
	{
		throw runtime_error("Error while initializing rpi-led-matrix library");
	}
	int width = config.GetPanelWidth();
	int height = config.GetPanelHeight();
	int chain_length = config.GetChainLength();
	int parallel_count = config.getParallelCount();
	int display_width = config.GetDisplayWidth();
	int display_height = config.GetDisplayHeight();

	float animation_duration = config.getAnimationDuration(0);
	BitmapSet bitmap_set = BitmapSet(animation_duration);
	for (int i = 0; i < config.getImageCount(1); i++)
	{
		Bitmap* bitmap = new Bitmap(config.getImage(1, i), width*chain_length, height);
		bitmap_set.Add(bitmap);
	}

	Microphone * microphone = new Microphone("plughw:1,0");
	FFT * fft = new FFT(FFT_LOG);

    RGBMatrix *canvas = new RGBMatrix(&io, height, chain_length, parallel_count);

    int panel_rows = config.getParallelCount();
    int panel_columns = config.GetChainLength();
    GridTransformer* grid = new GridTransformer(display_width, display_height, width, height, chain_length, config.GetPanels(), canvas);
    panel_rows = grid->getRows();
    panel_columns = grid->getColumns();
	grid->SetCutoff(config.GetLEDCutoff());
	grid->SetMaxBrightness(config.GetLEDMaxBrightness());

	canvas->Fill(0, 0, 0);
    for (int j=0; j<panel_rows; ++j)
	{
      for (int i=0; i<panel_columns; ++i)
	  {
        // Compute panel origin position.
        int x = i*config.GetPanelWidth();
        int y = j*config.GetPanelHeight();
        // Print the current grid position to the top left (origin) of the panel.
        stringstream pos;
        pos << i << "," << j;
        printCanvas(grid, x, y, pos.str());
      }
    }
	sleep(1);

	int bins[TOTAL_BIN_DEPTH][BIN_COUNT];
	int normalized_bins[TOTAL_BIN_DEPTH][BIN_COUNT];
	int i = 0, j = 0;
	for (i = 0; i<TOTAL_BIN_DEPTH; i++)
	{
		for (j = 0; j<BIN_COUNT; j++)
		{
			bins[i][j] = normalized_bins[i][j] = 0;
		}
	}

    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;
	int buffer_size = 1 << FFT_LOG;
	short buf[buffer_size];
    while (running)
	{
		// move bins
		for (i = TOTAL_BIN_DEPTH - 1; i>0; i--)
		{
			for (j = 0; j<BIN_COUNT; j++)
			{
				bins[i][j] = bins[i - 1][j];
			}
		}
		microphone->GetData(buf, buffer_size);
		fft->GetBins(buf, &bins[0][0], BIN_COUNT, SAMP_RATE);
		FFTOptions options = Logarithmic | Autoscale | Sigmoid;
		NormalizeBins(bins, normalized_bins, options);
		float seconds = (float)(clock() - begin_time) / (float)CLOCKS_PER_SEC;
		int bitmap_index = bitmap_set.GetIndex(seconds);
		PrintBitmap(grid, bitmap_set.Get(bitmap_index), normalized_bins);
		usleep(1000);
    }
    canvas->Clear();
    delete canvas;
	delete microphone;
	delete fft;
  }
  catch (const exception& ex)
  {
    cerr << ex.what() << endl;
    return -1;
  }
  return 0;
}


