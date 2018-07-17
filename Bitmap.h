#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

class Bitmap
{
public:
	Bitmap(const char * path, float duration, unsigned int width, unsigned int height);
	~Bitmap();
	unsigned char* GetData();
	unsigned int GetWidth();
	unsigned int GetHeight();
private:
	float duration;
	unsigned int width;
	unsigned int height;
	unsigned char* data;
	void Read(const char* filename, unsigned char* data);
};