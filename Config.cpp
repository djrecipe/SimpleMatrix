#include <sstream>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <vector>
#include <iostream>

#include <libconfig.h++>

#include "Config.h"

using namespace std;


Config::Config(const string& filename)
{
	try
	{
		libconfig::Config cfg;
		cfg.readFile(filename.c_str());
		libconfig::Setting& root = cfg.getRoot();
		// dimension configuration values
		displayWidth = root["display_width"];
		displayHeight = root["display_height"];
		panelWidth = root["panel_width"];
		panelHeight = root["panel_height"];
		chainLength = root["chain_length"];
		parallelCount = root["parallel_count"];
		// led configuration values
		ledCutoff = root["led_cutoff"];
		ledMaxBrightness = root["led_max_brightness"];
		// Do basic validation of configuration.
		if (displayWidth % panelWidth != 0)
		{
			throw invalid_argument("display_width must be a multiple of panel_width!");
		}
		if (displayHeight % panelHeight != 0)
		{
			throw invalid_argument("display_height must be a multiple of panel_height!");
		}
		if ((parallelCount < 1) || (parallelCount > 3))
		{
			throw invalid_argument("parallel_count must be between 1 and 3!");
		}
		// Parse out the individual panel configurations.
		libconfig::Setting& panels_config = root["panels"];
		for (int i = 0; i < panels_config.getLength(); ++i)
		{
			libconfig::Setting& row = panels_config[i];
			for (int j = 0; j < row.getLength(); ++j)
			{
				GridTransformer::Panel panel = this->GetPanel(row[j]);
				// Add the panel to the list of panel configurations.
				panels.push_back(panel);
			}
		}
		// Check the number of configured panels matches the expected number
		// of panels (# of panel columns * # of panel rows).
		int expected = (displayWidth / panelWidth) * (displayHeight / panelHeight);
		if (panels.size() != (unsigned int)expected)
		{
			stringstream error;
			error << "Expected " << expected << " panels in configuration but found " << panels.size() << "!";
			throw invalid_argument(error.str());
		}
		// Parse image info
		int image_set_duration = root["image_set_duration"];
		imageSetDuration = ((float)image_set_duration) / 1000.0;
		libconfig::Setting& animation_durations_config = root["animation_durations"];
		this->animationDurations.clear();
		for (int i = 0; i<animation_durations_config.getLength(); ++i)
		{
			libconfig::Setting& row = animation_durations_config[i];
			for (int j = 0; j<row.getLength(); ++j)
			{
				int duration = row[j]["value"];
				this->animationDurations.push_back(((float)duration) / 1000.0);
			}

		}
		libconfig::Setting& images_config = root["images"];
		for (int i = 0; i < images_config.getLength(); ++i)
		{
			libconfig::Setting& row = images_config[i];
			std::vector<std::string>* images = new std::vector<std::string>();
			for (int j = 0; j < row.getLength(); ++j)
			{
				const char * c_path = row[j]["value"];
				std::string path(c_path);
				images->push_back(path);
			}
			this->imageSets.push_back(images);
		}
		for (unsigned int i = 0; i<imageSets.size(); i++)
		{
			for (unsigned int j = 0; j<imageSets[i]->size(); j++)
			{
				fprintf(stderr, "Discovered Image Path: %s\n", (*imageSets[i])[j].c_str());
			}
		}
		return;
	}
	catch (const libconfig::FileIOException& fioex)
	{
		throw runtime_error("IO error while reading configuration file.  Does the file exist?");
	}
	catch (const libconfig::ParseException& pex)
	{
		stringstream error;
		error << "Config file error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError();
		throw invalid_argument(error.str());
	}
	catch (const libconfig::SettingNotFoundException& nfex)
	{
		stringstream error;
		error << "Expected to find setting: " << nfex.getPath();
		throw invalid_argument(error.str());
	}
	catch (const libconfig::ConfigException& ex)
	{
		throw runtime_error("Error while loading configuration");
	}
	return;
}

GridTransformer::Panel Config::GetPanel(libconfig::Setting row)
{
	GridTransformer::Panel panel;
	// Read panel order (required setting for each panel).
	panel.order = row[j]["order"];
	// Set default values for rotation and parallel chain, then override
	// them with any panel-specific configuration values.
	panel.rotate = 0;
	panel.parallel = 0;
	row[j].lookupValue("rotate", panel.rotate);
	row[j].lookupValue("parallel", panel.parallel);
	// Perform validation of panel values.
	// If panels are square allow rotations that are a multiple of 90, otherwise
	// only allow a rotation of 180 degrees.
	if ((panelWidth == panelHeight) && (panel.rotate % 90 != 0))
	{
		stringstream error;
		error << "Panel " << i << "," << j << " rotation must be a multiple of 90 degrees!";
		throw invalid_argument(error.str());
	}
	else if ((panelWidth != panelHeight) && (panel.rotate % 180 != 0))
	{
		stringstream error;
		error << "Panel row " << j << ", column " << i << " can only be rotated 180 degrees!";
		throw invalid_argument(error.str());
	}
	// Check that parallel is value between 0 and 2 (up to 3 parallel chains).
	if ((panel.parallel < 0) || (panel.parallel > 2))
	{
		stringstream error;
		error << "Panel row " << j << ", column " << i << " parallel value must be 0, 1, or 2!";
		throw invalid_argument(error.str());
	}
	return panel;
}

Config::~Config()
{
	for (unsigned int i = 0; i<this->imageSets.size(); i++)
	{
		delete this->imageSets[i];
	}
	return;
}
