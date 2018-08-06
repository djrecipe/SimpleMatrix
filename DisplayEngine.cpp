#include "DisplayEngine.h"


DisplayEngine::DisplayEngine(Config& config)
{
	// flag as not running
	this->running = false;

	// initialize helper classes
	this->InitializeBitmaps(config);
	this->InitializeAudioDevice(config.GetAudioDevice());
	this->InitializeFFT();
	this->InitializeMatrix(config);
	fprintf(stderr, "Done Initializing Display Engine\n");
	return;
}

DisplayEngine::~DisplayEngine()
{
	fprintf(stderr, "Entering Display Engine destructor...");
	delete this->bitmaps;
	delete this->microphone;
	delete this->fft;
	delete this->matrix;
	delete this->canvas;
	return;
}

void DisplayEngine::InitializeAudioDevice(std::string device)
{
	fprintf(stderr, "Initializing audio device...\n");
	this->microphone = new Microphone(device);
	return;
}

void DisplayEngine::InitializeBitmaps(Config& config)
{
	fprintf(stderr, "Initializing bitmaps...\n");
	this->bitmaps = new BitmapManager();
	for (int i = 0; i < config.GetImageSetCount(); i++)
	{
		float animation_duration = config.GetAnimationDuration(i);
		this->bitmaps->CreateSet(animation_duration);
		for (int j = 0; j < config.GetImageCount(i); j++)
		{
			this->bitmaps->AddImage(i, config.GetImage(i, j));
		}
	}
	return;
}

void DisplayEngine::InitializeFFT()
{
	fprintf(stderr, "Initializing FFT processor...\n");
		this->fft = new FFT(FFT_LOG, SAMP_RATE);
	this->fft->Create(BIN_COUNT, TOTAL_BIN_DEPTH);
	return;
}

void DisplayEngine::InitializeMatrix(Config& config)
{
	fprintf(stderr, "Initializing LED matrix...\n");
	// get LED matrix parameters
	int width = config.GetPanelWidth();
	int height = config.GetPanelHeight();
	int chain_length = config.GetChainLength();
	int parallel_count = config.GetParallelCount();
	int display_width = config.GetDisplayWidth();
	int display_height = config.GetDisplayHeight();

	// initialize LED matrix
	this->canvas = new RGBMatrix(height, chain_length, parallel_count);
	this->matrix = new GridTransformer(display_width, display_height, width, height, chain_length, config.GetPanels(), canvas);
	this->matrix->SetCutoff(config.GetLEDCutoff());
	this->matrix->SetMaxBrightness(config.GetLEDMaxBrightness());
	this->matrix->Fill(0, 0, 0);
	return;
}

void DisplayEngine::PrintBitmap(Bitmap* bitmap, float red_gain, float green_gain, float blue_gain)
{
	// retrieve data array
	unsigned char* data = bitmap->GetData();

	// iterate through each pixel in the bitmap
	for (int x = 0; x<bitmap->GetWidth(); x++)
	{
		for (int y = 0; y<bitmap->GetHeight(); y++)
		{
			// calculate index into single dimensional array of pixel data
			int index = y * bitmap->GetWidth() * 3 + x * 3;
			// calculate color
			int r = (float)data[index] * red_gain;
			int g = (float)data[index + 1] * green_gain;
			int b = (float)data[index + 2] * blue_gain;
			// draw (mirror y)
			this->matrix->SetPixel(x, y, r, g, b);
		}
	}
	return;
}

void DisplayEngine::PrintBorder(float red_gain, float green_gain, float blue_gain)
{
	// disable minimum brightness cutoff
	this->matrix->EnableCutoff(false);

	// print horizontal border lines
	for (int x = 0; x < this->matrix->width(); x++)
	{
		// print vertical border lines
		for (int y = 0; y < this->matrix->height(); y++)
		{
			int x_dist = x - this->matrix->width() / 2;
			int y_dist = y - this->matrix->height() / 2;
			float color_val = (float)fmax(pow(sqrt(pow(x_dist, 2) + pow(y_dist, 2)), 2) / 32 - 10, 0);
			this->matrix->SetPixel(x, y, (int)(color_val * red_gain), (int)(color_val * green_gain), (int)(color_val * blue_gain));
		}
	}

	// re-enable minimum brightness cutoff
	this->matrix->EnableCutoff(true);
	return;
}

void DisplayEngine::PrintCanvas(int x, int y, const string& text, int r, int g, int b)
{
	// iterate through characters of text
	for (auto c : text)
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
					this->matrix->SetPixel(x, y + j, r, g, b);
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
	// retrieve panel dimensions
	int panel_rows = this->matrix->getRows();
	int panel_columns = this->matrix->getColumns();
	int panel_width = this->matrix->width() / panel_columns;
	int panel_height = this->matrix->height() / panel_rows;

	// iterate through panels
	for (int j = 0; j<panel_rows; ++j)
	{
		for (int i = 0; i<panel_columns; ++i)
		{
			// compute individual panel origin
			int x = i * panel_width;
			int y = j * panel_height;
			// print panel indices to top-left of panel
			stringstream pos;
			pos << i << "," << j;
			this->PrintCanvas(x, y, pos.str());
		}
	}
	return;
}

void DisplayEngine::PrintContractingCircle(float seconds, float red_gain, float green_gain, float blue_gain)
{
	// disable minimum brightness cutoff
	this->matrix->EnableCutoff(false);

	// determine circle time (circle constantly shrinks but resets every second)
	float duration = 1.0;
	if(seconds - this->contractingCircleReset > duration)
	{
		this->contractingCircleReset = seconds;
	}
	float ratio = duration - (seconds - this->contractingCircleReset);
	
	// print horizontal border lines
	int half_width = this->matrix->width()/2;
	int half_height = this->matrix->height()/2;
	for (int x = 0; x < this->matrix->width(); x++)
	{
		// print vertical border lines
		for (int y = 0; y < this->matrix->height(); y++)
		{
			int x_dist = (int)((float)(half_width - abs(x - half_width)) * ratio);
			int y_dist = (int)((float)(half_height - abs(y - half_height)) * ratio);
			float color_val = (float)fmax(pow(sqrt(pow(x_dist, 2) + pow(y_dist, 2)), 2) / 32, 0);
			this->matrix->SetPixel(x, y, (int)(color_val * red_gain), (int)(color_val * green_gain), (int)(color_val * blue_gain));
		}
	}

	// re-enable minimum brightness cutoff
	this->matrix->EnableCutoff(true);
	return;
}

void DisplayEngine::Start()
{
	fprintf(stderr, "Initializing display loop...\n");
	// flag as running
	this->running = true;
	const clock_t start_time = clock();

	// create buffers
	int buffer_size = 1 << FFT_LOG;
	short buf[buffer_size];

	// initialize values
	int bitmap_set_index = 0;
	float last_bitmap_change = 0;
	DisplayModes mode = BitmapDisplayMode;
	float red_gain = 1.0, green_gain = 1.0, blue_gain = 1.0;

	// print identification
	this->PrintIdentification();
	sleep(3);

	// start loop
	while (this->running)
	{
		// get new time
		float seconds = (float)(clock() - start_time) / (float)CLOCKS_PER_SEC;

		// get microphone data
		this->microphone->GetData(buf, buffer_size);

		// process data
		int** bins = this->fft->Cycle(buf, BIN_DEPTH, seconds);
		this->fft->GetColorGains(red_gain, green_gain, blue_gain);

		// respond to events
		FFTEvents fft_event = this->fft->GetEvents();
		switch (fft_event)
		{
			case DecreasedAmplitudeFFTEvent:
				if (seconds - last_bitmap_change > MIN_BITMAP_SET_DURATION)
				{
					bitmap_set_index = (bitmap_set_index + 1) % this->bitmaps->GetSetCount();
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
		int image_index = this->bitmaps->GetIndex(bitmap_set_index, seconds);
		Bitmap* bitmap = this->bitmaps->Get(bitmap_set_index, image_index);
		switch (mode)
		{
		case LowAmplitudeDisplayMode:
			this->PrintBitmap(bitmap, red_gain*0.5, green_gain*0.5, blue_gain*0.5);
			this->PrintContractingCircle(seconds, red_gain, green_gain, blue_gain);
			break;
		case HighAmplitudeDisplayMode:
			this->PrintBitmap(bitmap, red_gain, green_gain, blue_gain);
			this->PrintBorder(red_gain, green_gain, blue_gain);
			break;
		default:
		case BitmapDisplayMode:
			this->PrintBitmap(bitmap, red_gain, green_gain, blue_gain);
			break;
		}

		// wait for next loop
		this->matrix->ResetScreen();
		usleep(1000);
	}

	// clean-up
	this->matrix->Clear();

	return;
}

void DisplayEngine::Stop()
{
	this->running = false;
	return;
}