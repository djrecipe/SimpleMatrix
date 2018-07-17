#include "BitmapSet.h"

using namespace std;

BitmapSet::BitmapSet(float duration)
{
	this->duration = duration;
	this->images.clear();
	return;
}

void BitmapSet::Add(Bitmap* bitmap)
{
	this->images.push_back(bitmap);
	return;
}

Bitmap* BitmapSet::Get(int index)
{
	return this->images[index];
}

int BitmapSet::GetImageCount()
{
	return this->images.size();
}

unsigned int BitmapSet::GetIndex(float seconds)
{
	int weight = (int)(100.0*this->duration);
	int value = (int)(seconds*100.0) % weight;
	int image_count = this->GetImageCount();
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

BitmapSet::~BitmapSet()
{
	for (int i = 0; i < this->images.size(); i++)
	{
		delete this->images[i];
	}
	return;
}
