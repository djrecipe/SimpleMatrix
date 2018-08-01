#include "BitmapManager.h"

BitmapManager::BitmapManager()
{
}

void BitmapManager::AddImage(int index, const char * path)
{
	assert(index < this->sets.size());
	this->sets[index]->Add(path);
	return;
}

void BitmapManager::Clear()
{
	this->sets.clear();
	return;
}

void BitmapManager::CreateSet(float duration)
{
	BitmapSet* set = new BitmapSet(duration);
	this->sets.push_back(set);
	return;
}

Bitmap* BitmapManager::Get(int set_index, int index)
{
	assert(set_index < this->sets.size());
	BitmapSet* set = this->sets[set_index];
	return set->Get(index);
}

int BitmapManager::GetIndex(int set_index, float seconds)
{
	assert(set_index < this->sets.size());
	return this->sets[set_index]->GetIndex(seconds);
}

BitmapManager::~BitmapManager()
{
	for(int i=0; i<this->sets.size(); i++)
	{
		delete this->sets[i];
	}
	return;
}