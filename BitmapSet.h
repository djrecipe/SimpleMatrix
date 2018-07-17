#pragma once

#include <math.h>
#include "Bitmap.h"

class BitmapSet
{
	public:
		BitmapSet(float set_duration);
		~BitmapSet();
		void Add(Bitmap* bitmap);
		Bitmap* Get(int index);
		unsigned int GetIndex(float seconds);
	private:
		float duration;

		std::vector<Bitmap*> images;
		int GetImageCount();
};