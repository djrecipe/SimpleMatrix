#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <vector>

class Bitmap
{
public:
	Bitmap(const char * path);
	~Bitmap();
	unsigned char* GetData();
	unsigned int GetWidth();
	unsigned int GetHeight();
private:
	unsigned int width;
	unsigned int height;
	unsigned char* data;
	void Read(const char* filename);
};