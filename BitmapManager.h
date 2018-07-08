#pragma once

class BitmapManager
{
	public:
		BitmapManager(float set_duration, float animation_duration);
	private:
		int imageSetCount;
		float imageSetDuration;
		float imageAnimationDuration;

		std::vector<std::vector<unsigned char*>*> logos;
		void Initialize();
		int GetImageCount(int set_index)
		unsigned int GetIndex(float seconds, int set_index);
		unsigned int GetSetIndex(float seconds);
};