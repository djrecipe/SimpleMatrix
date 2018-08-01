#include "Bitmap.h"

using namespace std;

Bitmap::Bitmap(const char * path)
{
	this->Read(path);
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

void Bitmap::Read(const char* path)
{
	if (path == NULL or strlen(path) < 1)
		throw invalid_argument("Invalid bitmap path");

	// open file
	fprintf(stderr, "Reading bitmap '%s'\n", path);
	FILE* f = fopen(path, "rb");
	if (f == NULL)
		throw invalid_argument("Failed to open bitmap file");

	// read header
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f);
	this->width = *(int*)&info[18];
	this->height = *(int*)&info[22];
	
	// read data
	int row_padded = (width * 3 + 3) & (~3);
	this->data = new unsigned char[width * height * 3];
	for (int y = 0; y < height; y++)
	{
		fread(&data[y*row_padded], sizeof(unsigned char), row_padded, f);
		for (int x = 0; x < width; x++)
		{
			// Convert (B, G, R) to (R, G, B)
			int index = y * row_padded + (x * 3);
			unsigned char tmp = data[index];
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