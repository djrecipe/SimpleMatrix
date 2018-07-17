#pragma once

#include <string>
#include <vector>

#include "GridTransformer.h"

class Config
{
public:
	Config(const std::string& filename);
	~Config();

	float getAnimationDuration(int set_index) const
	{
		return animationDurations[set_index];
	}

	int GetDisplayWidth() const
	{
		return this->displayWidth;
	}
	int GetDisplayHeight() const
	{
		return this->displayHeight;
	}
	int getLEDCutoff() const
	{
		return this->ledCutoff;
	}
	int getLEDMaxBrightness() const
	{
		return this->ledMaxBrightness;
	}
	int GetPanelWidth() const
	{
		return this->panelWidth;
	}
	int GetPanelHeight() const
	{
		return this->panelHeight;
	}
	int GetChainLength() const
	{
		return this->chainLength;
	}
	const char* getImage(int set_index, int image_index)
	{
		return (*imageSets[set_index])[image_index].c_str();
	}
	int getImageCount(int index) const
	{
		return this->imageSets[index]->size();
	}
	int getImageSetCount() const
	{
		return this->imageSets.size();
	}
	int getImageSetDuration() const
	{
		return this->imageSetDuration;
	}
	int getParallelCount() const
	{
		return parallelCount;
	}
	std::vector<GridTransformer::Panel> GetPanels() const
	{
		return this->panels;
	}

	GridTransformer::Panel GetPanel(libconfig::Setting row);

private:
	int displayWidth,
		displayHeight,
		panelWidth,
		panelHeight,
		chainLength,
		parallelCount,
	int ledCutoff,
		ledMaxBrightness;
	int imageSetDuration;
	std::vector<float> animationDurations;
	std::vector<GridTransformer::Panel> panels;
	std::vector<std::vector<std::string>*> imageSets;
};

