#include "BitmapManager.h"

using namespace std;

BitmapManager::BitmapManager(float set_duration, float animation_duration)
{
	this->imageSetDuration = set_duration;
	this->imageAnimationDuration = animation_duration;
	this->Initialize();
	return;
}

int BitmapManager::GetImageCount(int set_index)
{
	return 1;
}

unsigned int BitmapManager::GetSetIndex(float seconds)
{
	float divisor = this->logos.size();
	float interval = this->imageSetDuration / divisor;
	int value = (int)seconds % (int)this->imageSetDuration;
	unsigned int index = fmin((float)value / interval, (float)(this->logos.size() - 1));
	return index;
}

unsigned int BitmapManager::GetIndex(float seconds, int set_index)
{
	int weight = (int)(100.0*this->imageAnimationDuration);
	int value = (int)(seconds*100.0) % weight;
	int image_count = this->GetImageCount(set_index);
	int loop_image_count = 1;
	if (image_count > 1)
		loop_image_count = ((image_count - 2) * 2) + 2;
	int divisor = weight / loop_image_count;
	int index = value / divisor;
	if (index >= image_count)
	{
		index = image_count - 2 - (index - image_count);
	}
	index = (int)fmax(fmin(index, image_count - 1), 0);
	return index;
}

void BitmapManager::Initialize()
{
	this->logos.clear();
	for (int i = 0; i<this->imageSetCount; i++)
	{
		std::vector<unsigned char*> *new_logos = new std::vector<unsigned char*>();
		for (int j = 0; j<this->GetImageCount(i); j++)
		{
			new_logos->push_back(new unsigned char[this->panelWidth * this->panelHeight * 3]);
			const char * path = this->GetImage(i, j);
			this->ReadBitmap(path, (*new_logos)[j]);
		}
		this->logos.push_back(new_logos);
	}
	return;
}

void BitmapManager::Read(const char* filename, unsigned char* data)
{
	if (filename == NULL or sizeof(filename) < 1)
		throw "Invalid bitmap filename";

	fprintf(stderr, "Reading bitmap (%s)\n", filename);

	// open file
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		throw "Argument Exception";

	// read header
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f);
	int width = *(int*)&info[18];
	int height = *(int*)&info[22];
	// assert correct dimensions
	assert(width == this->config->getPanelWidth() * this->config->getChainLength());
	assert(height == this->config->getPanelHeight());

	//
	int row_padded = (width * 3 + 3) & (~3);
	unsigned char tmp;

	int x = 0, y = 0, index = 0;
	for (y = 0; y < height; y++)
	{
		fread(&data[y*row_padded], sizeof(unsigned char), row_padded, f);
		for (x = 0; x < width; x++)
		{
			// Convert (B, G, R) to (R, G, B)
			index = y * row_padded + (x * 3);
			tmp = data[index];
			data[index] = data[index + 2];
			data[index + 2] = tmp;
		}
	}
	fclose(f);
	fprintf(stderr, "Successfully read bitmap\n");
	return;
}