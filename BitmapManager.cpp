#include "BitmapManager.h"

using namespace std;

BitmapManager::BitmapManager(float duration)
{
	this->imageSetDuration = duration;
	this->images.clear();
	return;
}

void BitmapManager::Add(Bitmap* bitmap)
{
	this->images.push_back(bitmap);
	return;
}

int BitmapManager::GetImageCount()
{
	return this->images.size();
}

unsigned int BitmapManager::GetIndex(float seconds)
{
	return 0;
}

BitmapManager::~BitmapManager()
{
	for (int i = 0; i < this->images.size(); i++)
	{
		delete this->images[i];
	}
	return;
}
