#pragma once

#include <cassert>
#include <math.h>

#include "Bitmap.h"

class BitmapSet
{
	public:
		BitmapSet(float set_duration);
		~BitmapSet();
		void Add(const char* path);
		Bitmap* Get(int index);
		unsigned int GetIndex(float seconds);
	private:
		float duration;

		std::vector<Bitmap*> images;
		int GetImageCount();
};