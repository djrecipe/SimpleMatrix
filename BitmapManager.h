#pragma once

#include "BitmapSet.h"

#include <vector>

class BitmapManager
{
public:
	BitmapManager();
	void AddImage(int index, const char * path);
	void Clear();
	void CreateSet(float duration);
	Bitmap* Get(int set_index, int index);
	int GetIndex(int set_index, float seconds);
	~BitmapManager();
private:
	std::vector<BitmapSet*> sets;
};