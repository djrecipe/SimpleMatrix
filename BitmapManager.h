#pragma once

#include "Bitmap.h"

class BitmapManager
{
	public:
		BitmapManager(float set_duration);
		~BitmapManager();
		void Add(Bitmap* bitmap);
	private:
		float imageSetDuration;

		std::vector<Bitmap*> images;
		int GetImageCount();
		unsigned int GetIndex(float seconds);
};