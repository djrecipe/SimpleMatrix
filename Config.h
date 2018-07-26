#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdio>
#include <string>
#include <vector>

#include <libconfig.h++>

#include "GridTransformer.h"

class Config
{
public:
	Config(const std::string& filename);
	~Config();

	float GetAnimationDuration(int set_index) const
	{
		return this->animationDurations[set_index];
	}

	std::string GetAudioDevice() const
	{
		return this->audioDevice;
	}

	int GetDisplayWidth() const
	{
		return this->displayWidth;
	}
	int GetDisplayHeight() const
	{
		return this->displayHeight;
	}
	int GetLEDCutoff() const
	{
		return this->ledCutoff;
	}
	int GetLEDMaxBrightness() const
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
	const char* GetImage(int set_index, int image_index)
	{
		return (*imageSets[set_index])[image_index].c_str();
	}
	int GetImageCount(int index) const
	{
		return this->imageSets[index]->size();
	}
	int GetImageSetCount() const
	{
		return this->imageSets.size();
	}
	int GetImageSetDuration() const
	{
		return this->imageSetDuration;
	}
	int GetParallelCount() const
	{
		return this->parallelCount;
	}
	std::vector<GridTransformer::Panel> GetPanels() const
	{
		return this->panels;
	}

	GridTransformer::Panel GetPanel(libconfig::Setting* row);

private:
	int displayWidth,
		displayHeight,
		panelWidth,
		panelHeight,
		chainLength,
		parallelCount,
		ledCutoff,
		ledMaxBrightness,
		imageSetDuration;
	std::string audioDevice;
	std::vector<float> animationDurations;
	std::vector<GridTransformer::Panel> panels;
	std::vector<std::vector<std::string>*> imageSets;
};

