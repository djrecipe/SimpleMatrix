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

using namespace std;
using namespace rgb_matrix;


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

void PrintBitmap(GridTransformer* canvas, Bitmap* bitmap, int** bins, int bin_count, int bin_depth)
{
	// initialize parameters
	int x = 0, y = 0, r = 0, g = 0, b = 0, i = 0, j = 0;
	float decay = 0.0, bin_gain = 0.0, red_gain = 0.0, green_gain = 0.0, blue_gain = 0.0;
	unsigned char* data = bitmap->GetData();
	// calculate color gains based on audio frequency content
	// iterate through bins, depthwise
	for (i = 0; i<bin_depth; i++)
	{
		// calculate decay (based on age)
		decay = (float)(BIN_DEPTH - i) / (float)bin_depth;
		// iterate through all frequency bins (of a given depth)
		for (j = 0; j<bin_count; j++)
		{
			// calculate base gain (based on bin amplitude) (0.0 -> 2.0)
			bin_gain = (float)bins[i][j] / (FULL_SCALE / 2.0);
			// increases with bin frequency (0.1 -> 1.0)
			blue_gain = fmax(bin_gain * ((float)(j + 1) / (float)bin_count)*decay, blue_gain);
			// increases towards center frequency (0.1 -> 1.0 -> 0.1)
			green_gain = fmax(bin_gain * ((float)(BIN_COUNT / 2 - abs((j + 1) - bin_count / 2)) / (float)(BIN_COUNT / 2))*decay, green_gain);
			// decreases with bin frequency (1.0 -> 0.1)
			red_gain = fmax(bin_gain * ((float)(BIN_COUNT - j) / (float)bin_count)*decay, red_gain);
		}
	}
	// iterate through each pixel in the bitmap
	for (x = 0; x<bitmap->GetWidth(); x++)
	{
		for (y = 0; y<bitmap->GetHeight(); y++)
		{
			// calculate index into single dimensional array of pixel data
			int index = y * bitmap->GetWidth() * 3 + x * 3;
			// calculate color
			r = (float)data[index] * red_gain;
			g = (float)data[index + 1] * green_gain;
			b = (float)data[index + 2] * blue_gain;
			// draw
			canvas->SetPixel(bitmap->GetWidth() - x, bitmap->GetHeight() - y, r, g, b);
		}
	}
	return;
}

static void sigintHandler(int s)
{
  running = false;
}



int main(int argc, char** argv)
{
  try
  {
	const clock_t begin_time = clock();
    Config config(argc >= 2 ? argv[1] : "/dev/null");

	int width = config.GetPanelWidth();
	int height = config.GetPanelHeight();
	int chain_length = config.GetChainLength();
	int parallel_count = config.getParallelCount();
	int display_width = config.GetDisplayWidth();
	int display_height = config.GetDisplayHeight();

	float animation_duration = config.getAnimationDuration(0);
	BitmapSet bitmap_set = BitmapSet(animation_duration);
	for (int i = 0; i < config.getImageCount(0); i++)
	{
		Bitmap* bitmap = new Bitmap(config.getImage(0, i), width*chain_length, height);
		bitmap_set.Add(bitmap);
	}

	Microphone * microphone = new Microphone("plughw:1,0");
	FFT * fft = new FFT(FFT_LOG, SAMP_RATE);
	fft->Create(BIN_COUNT, TOTAL_BIN_DEPTH);

    RGBMatrix *canvas = new RGBMatrix(height, chain_length, parallel_count);

    GridTransformer* grid = new GridTransformer(display_width, display_height, width, height, chain_length, config.GetPanels(), canvas);
	grid->SetCutoff(config.GetLEDCutoff());
	grid->SetMaxBrightness(config.GetLEDMaxBrightness());
	grid->Fill(0, 0, 0);

	// print test
	int panel_rows = grid->getRows();
	int panel_columns = grid->getColumns();
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


    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;
	int buffer_size = 1 << FFT_LOG;
	short buf[buffer_size];
    while (running)
	{
		microphone->GetData(buf, buffer_size);
		int** bins = fft->Cycle(buf, BIN_DEPTH);
		float seconds = (float)(clock() - begin_time) / (float)CLOCKS_PER_SEC;
		int bitmap_index = bitmap_set.GetIndex(seconds);
		PrintBitmap(grid, bitmap_set.Get(bitmap_index), bins, BIN_COUNT, BIN_DEPTH);
		grid->ResetScreen();
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


