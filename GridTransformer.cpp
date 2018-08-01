#include "GridTransformer.h"

using namespace rgb_matrix;
using namespace std;

GridTransformer::GridTransformer(int width, int height, int panel_width, int panel_height, int chain_length, const std::vector<Panel>& panels, Canvas* source):
  _width(width),
  _height(height),
  _panel_width(panel_width),
  _panel_height(panel_height),
  _chain_length(chain_length),
  _source(source),
  _panels(panels)
{
  // Display width must be a multiple of the panel pixel column count.
  assert(_width % _panel_width == 0);
  // Display height must be a multiple of the panel pixel row count.
  assert(_height % _panel_height == 0);
  assert(_source != NULL);
  // Compute number of rows and columns of panels.
  _rows = _height / _panel_height;
  _cols = _width / _panel_width;
  // Check panel definition list has exactly the expected number of panels.
  assert((_rows * _cols) == (int)_panels.size());

  this->pixelStates = new bool*[width];
  for (int x = 0; x < width; x++)
  {
	  this->pixelStates[x] = new bool[height];
  }

  this->cutoff = 0;
  this->maxBrightness = 255;
  return;
}

void GridTransformer::ResetScreen()
{
	for (int x = 0; x < this->_width; x++)
	{
		for (int y = 0; y < this->_height; y++)
		{
			if (!this->pixelStates[x][y])
				this->SetPixel(x, y, 0, 0, 0);
			this->pixelStates[x][y] = false;
		}
	}
	return;
}

void GridTransformer::SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue, bool force)
{
	// check pixel boundaries
	assert(x >= 0 && y >= 0 && x < this->_width && y < this->_height);

	// check if pixel is too dark
	if (red < this->cutoff && green < this->cutoff && blue < this->cutoff)
		return;

	// check if pixel already written to
	if (this->pixelStates[x][y] && !force)
		return;
	this->pixelStates[x][y] = true;

	// Figure out what row and column panel this pixel is within.
	int row = y / this->_panel_height;
	int col = x / this->_panel_width;

  // Get the panel information for this pixel.
  Panel panel = _panels[_cols*row + col];

  // Compute location of the pixel within the panel.
  x = x % _panel_width;
  y = y % _panel_height;

  // Perform any panel rotation to the pixel.
  // NOTE: 90 and 270 degree rotation only possible on 32 row (square) panels.
  if (panel.rotate == 90) {
    assert(_panel_height == _panel_width);
    int old_x = x;
    x = (_panel_height-1)-y;
    y = old_x;
  }
  else if (panel.rotate == 180) {
    x = (_panel_width-1)-x;
    y = (_panel_height-1)-y;
  }
  else if (panel.rotate == 270) {
    assert(_panel_height == _panel_width);
    int old_y = y;
    y = (_panel_width-1)-x;
    x = old_y;
  }

  // Determine x offset into the source panel based on its order along the chain.
  // The order needs to be inverted because the matrix library starts with the
  // origin of an image at the end of the chain and not at the start (where
  // ordering begins for this transformer).
  int x_offset = ((_chain_length-1)-panel.order)*_panel_width;

  // Determine y offset into the source panel based on its parrallel chain value.
  int y_offset = panel.parallel*_panel_height;

  this->_source->SetPixel(x_offset + x, y_offset + y, red, green, blue);
  return;
}

void GridTransformer::SetCutoff(int value)
{
	this->cutoff = value;
	return;
}

void GridTransformer::SetMaxBrightness(int value)
{
	this->maxBrightness = value;
	return;
}

Canvas* GridTransformer::Transform(Canvas* source)
{
  assert(source != NULL);
  int swidth = source->width();
  int sheight = source->height();
  assert((_width * _height) == (swidth * sheight));
  _source = source;
  return this;
}

GridTransformer::~GridTransformer()
{
	for (int x = 0; x < this->_width; x++)
	{
		delete this->pixelStates[x];
	}
	delete this->pixelStates;
	return;
}
