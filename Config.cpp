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
		// Load config file with libconfig.
		libconfig::Config cfg;
		cfg.readFile(filename.c_str());
		libconfig::Setting& root = cfg.getRoot();
		// Parse out the matrix configuration values.
		_display_width = root["display_width"];
		_display_height = root["display_height"];
		_panel_width = root["panel_width"];
		_panel_height = root["panel_height"];
		_chain_length = root["chain_length"];
		_parallel_count = root["parallel_count"];
		// led config
		_led_cutoff = root["led_cutoff"];
		_led_max_brightness = root["led_max_brightness"];
		// Set default value for optional config values.
		_crop_x = -1;
		_crop_y = -1;
		// Load optional crop_origin value.
		if (root.exists("crop_origin")) {
			libconfig::Setting& crop_origin = root["crop_origin"];
			if (crop_origin.getLength() != 2) {
				throw invalid_argument("crop_origin must be a list with two values, the X and Y coordinates of the crop box origin!");
			}
			_crop_x = crop_origin[0];
			_crop_y = crop_origin[1];
		}
		// Do basic validation of configuration.
		if (_display_width % _panel_width != 0) {
			throw invalid_argument("display_width must be a multiple of panel_width!");
		}
		if (_display_height % _panel_height != 0) {
			throw invalid_argument("display_height must be a multiple of panel_height!");
		}
		if ((_parallel_count < 1) || (_parallel_count > 3)) {
			throw invalid_argument("parallel_count must be between 1 and 3!");
		}
		// Parse out the individual panel configurations.
		libconfig::Setting& panels_config = root["panels"];
		for (int i = 0; i < panels_config.getLength(); ++i)
		{
			libconfig::Setting& row = panels_config[i];
			for (int j = 0; j < row.getLength(); ++j)
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
				if ((_panel_width == _panel_height) && (panel.rotate % 90 != 0)) {
					stringstream error;
					error << "Panel " << i << "," << j << " rotation must be a multiple of 90 degrees!";
					throw invalid_argument(error.str());
				}
				else if ((_panel_width != _panel_height) && (panel.rotate % 180 != 0)) {
					stringstream error;
					error << "Panel row " << j << ", column " << i << " can only be rotated 180 degrees!";
					throw invalid_argument(error.str());
				}
				// Check that parallel is value between 0 and 2 (up to 3 parallel chains).
				if ((panel.parallel < 0) || (panel.parallel > 2)) {
					stringstream error;
					error << "Panel row " << j << ", column " << i << " parallel value must be 0, 1, or 2!";
					throw invalid_argument(error.str());
				}
				// Add the panel to the list of panel configurations.
				_panels.push_back(panel);
			}
		}
		// Check the number of configured panels matches the expected number
		// of panels (# of panel columns * # of panel rows).
		int expected = (_display_width / _panel_width) * (_display_height / _panel_height);
		if (_panels.size() != (unsigned int)expected) {
			stringstream error;
			error << "Expected " << expected << " panels in configuration but found " << _panels.size() << "!";
			throw invalid_argument(error.str());
		}
		// Parse image info
		int image_set_duration = root["image_set_duration"];
		_image_set_duration = ((float)image_set_duration) / 1000.0;
		libconfig::Setting& animation_durations_config = root["animation_durations"];
		this->_animation_durations.clear();
		for (int i = 0; i<animation_durations_config.getLength(); ++i)
		{
			libconfig::Setting& row = animation_durations_config[i];
			for (int j = 0; j<row.getLength(); ++j)
			{
				int duration = row[j]["value"];
				this->_animation_durations.push_back(((float)duration) / 1000.0);
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
			this->_image_sets.push_back(images);
		}
		for (unsigned int i = 0; i<_image_sets.size(); i++)
		{
			for (unsigned int j = 0; j<_image_sets[i]->size(); j++)
			{
				fprintf(stderr, "Discovered Image Path: %s\n", (*_image_sets[i])[j].c_str());
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
		error << "Config file error at " << pex.getFile() << ":" << pex.getLine()
			<< " - " << pex.getError();
		throw invalid_argument(error.str());
	}
	catch (const libconfig::SettingNotFoundException& nfex)
	{
		stringstream error;
		error << "Expected to find setting: " << nfex.getPath();
		throw invalid_argument(error.str());
	}
	catch (const libconfig::ConfigException& ex) {
		throw runtime_error("Error while loading configuration");
	}
}

Config::~Config()
{
	for (unsigned int i = 0; i<this->_image_sets.size(); i++)
	{
		delete this->_image_sets[i];
	}
	// TODO: delete vector pointers
}
