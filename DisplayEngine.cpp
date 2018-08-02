#include "DisplayEngine.h"

using namespace std;
using namespace rgb_matrix;

DisplayEngine::DisplayEngine(Config config)
{
	// flag as not running
	this->running = false;

	// initialize helper classes
	this->InitializeBitmaps(config);
	this->InitializeAudioDevice(config.GetAudioDevice());
	this->InitializeFFT();
	this->InitializeMatrix();
	return;
}

void DisplayEngine::InitializeAudioDevice(std::string device)
{
	fprintf(stderr, "Initializing audio device...\n");
	this->microphone = Microphone(device);
	return;
}

void DisplayEngine::InitializeBitmaps(Config config)
{
	fprintf(stderr, "Initializing bitmaps...\n");
	this->bitmaps = BitmapManager();
	for (int i = 0; i < config.GetImageSetCount(); i++)
	{
		float animation_duration = config.GetAnimationDuration(i);
		this->bitmaps.CreateSet(animation_duration);
		for (int j = 0; j < config.GetImageCount(i); j++)
		{
			this->bitmaps.AddImage(i, config.GetImage(i, j));
		}
	}
	return;
}

void DisplayEngine::InitializeFFT()
{
	fprintf(stderr, "Initializing FFT processor...\n")
		this->fft = FFT(FFT_LOG, SAMP_RATE);
	this->fft.Create(BIN_COUNT, TOTAL_BIN_DEPTH);
	return;
}

void DisplayEngine::InitializeMatrix(Config config)
{
	// get LED matrix parameters
	int width = config.GetPanelWidth();
	int height = config.GetPanelHeight();
	int chain_length = config.GetChainLength();
	int parallel_count = config.GetParallelCount();
	int display_width = config.GetDisplayWidth();
	int display_height = config.GetDisplayHeight();

	// initialize LED matrix
	this->canvas = RGBMatrix(height, chain_length, parallel_count);
	this->matrix = GridTransformer(display_width, display_height, width, height, chain_length, config.GetPanels(), canvas);
	this->matrix.SetCutoff(config.GetLEDCutoff());
	this->matrix.SetMaxBrightness(config.GetLEDMaxBrightness());
	this->matrix.Fill(0, 0, 0);
	return;
}

void DisplayEngine::PrintBitmap(Bitmap* bitmap, int** bins, int bin_count, int bin_depth)
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
			this->matrix.SetPixel(x, bitmap->GetHeight() - y - 1, r, g, b);
		}
	}
	return;
}

void DisplayEngine::PrintBorder()
{
	// disable minimum brightness cutoff
	this->matrix.EnableCutoff(false);

	// print horizontal border lines
	for (int x = 0; x < this->matrix.width(); x++)
	{
		// print vertical border lines
		for (int y = 0; y < this->matrix.height(); y++)
		{
			int x_dist = x - this->matrix.width() / 2;
			int y_dist = y - this->matrix.height() / 2;
			int color_val = fmax(pow(sqrt(pow(x_dist, 2) + pow(y_dist, 2)), 2) / 32 - 10, 0);
			this->matrix.SetPixel(x, y, fmin(pow(color_val, 2) / 4, 255), color_val, color_val);
		}
	}

	// re-enable minimum brightness cutoff
	this->matrix.EnableCutoff(true);
	return;
}

void DisplayEngine::PrintCanvas(int x, int y, const string& message, int r = 255, int g = 255, int b = 255)
{
	// Loop through all the characters and print them starting at the provided
	// coordinates.
	for (auto c : message)
	{
		// Loop through each column of the character.
		for (int i = 0; i<5; ++i)
		{
			unsigned char col = glcdfont[c * 5 + i];
			x += 1;
			// Loop through each row of the column.
			for (int j = 0; j<8; ++j)
			{
				// Put a pixel for each 1 in the column byte.
				if ((col >> j) & 0x01) {
					this->matrix.SetPixel(x, y + j, r, g, b);
				}
			}
		}
		// Add a column of padding between characters.
		x += 1;
	}
	return;
}

void DisplayEngine::PrintIdentification()
{
	// print test
	int panel_rows = this->matrix.getRows();
	int panel_columns = this->matrix.getColumns();
	int panel_width = this->matrix.width() / panel_columns;
	int panel_height = this->matrix.height() / panel_rows;

	// iterate through each panel
	for (int j = 0; j<panel_rows; ++j)
	{
		for (int i = 0; i<panel_columns; ++i)
		{
			// Compute panel origin position.
			int x = i * panel_width;
			int y = j * panel_height;
			// Print the current grid position to the top left (origin) of the panel.
			stringstream pos;
			pos << i << "," << j;
			this->PrintCanvas(x, y, pos.str());
		}
	}
	return;
}

void DisplayEngine::PrintSparkles(float seconds)
{
	return;
}

void DisplayEngine::Start()
{
	// flag as running
	this->running = true;
	const clock_t start_time = clock();

	// create buffers
	int buffer_size = 1 << FFT_LOG;
	short buf[buffer_size];

	// intialize values
	int bitmap_set_index = 0;
	float last_bitmap_change = 0;
	DisplayModes mode = BitmapDisplayMode;

	// print identification
	this->PrintIdentification();
	sleep(1);

	// start loop
	while (this->running)
	{
		// get new time
		float seconds = (float)(clock() - start_time) / (float)CLOCKS_PER_SEC;

		// get microphone data
		this->microphone.GetData(buf, buffer_size);

		// process data
		int** bins = this->fft.Cycle(buf, BIN_DEPTH, seconds);

		// respond to events
		FFTEvents fft_event = this->fft.GetEvents();
		switch (fft_event)
		{
		case DecreasedAmplitudeFFTEvent:
			if (seconds - last_bitmap_change > 8.0)
			{
				bitmap_set_index = (bitmap_set_index + 1) % this->bitmaps.GetSetCount();
				last_bitmap_change = seconds;
			}
			mode = LowAmplitudeDisplayMode;
			break;
		case ReturnToLevelFFTEvent:
			mode = BitmapDisplayMode;
			break;
		case IncreasedAmplitudeFFTEvent:
			mode = HighAmplitudeDisplayMode;
			break;
		default:
		case NoneFFTEvent:
			break;
		}

		// print to LEDs
		int image_index = this->bitmaps.GetIndex(bitmap_set_index, seconds);
		switch (mode)
		{
		case LowAmplitudeDisplayMode:
			this->PrintSparkles(seconds);
			break;
		case HighAmplitudeDisplayMode:
			this->PrintBitmap(bitmaps.Get(bitmap_set_index, image_index), bins, BIN_COUNT, BIN_DEPTH);
			this->PrintBorder();
			break;
		default:
		case BitmapDisplayMode:
			this->PrintBitmap(bitmaps.Get(bitmap_set_index, image_index), bins, BIN_COUNT, BIN_DEPTH);
			break;
		}

		// wait for next loop
		this->matrix.ResetScreen();
		usleep(1000);
	}

	// clean-up
	this->matrix.Clear();

	return;
}

void DisplayEngine::Stop()
{
	this->running = false;
	return;
}