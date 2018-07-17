#include "Bitmap.h"

Bitmap::Bitmap(const char * path, float duration, unsigned int width, unsigned int height)
{
	this->duration = duration;
	this->width = width;
	this->height = height;
	this->data = new unsigned char[width * height * 3];
	this->Read(path, this->data);
	return;
}

unsigned char* Bitmap::GetData()
{
	return this->data;
}

unsigned int Bitmap::GetWidth()
{
	return this->width;
}

unsigned int Bitmap::GetHeight()
{
	return this->height;
}

void Bitmap::Read(const char* filename, unsigned char* data)
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

	assert(width == this->width);
	assert(height == this->height);

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

Bitmap::~Bitmap()
{
	delete this->data;
	return;
}