// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Program to aid in the testing of LED matrix chains.
// Author: Tony DiCola
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <led-matrix.h>
#include <signal.h>
#include <unistd.h>

#include "Config.h"
#include "glcdfont.h"
#include "GridTransformer.h"

using namespace std;
using namespace rgb_matrix;


// Global to keep track of if the program should run.
// Will be set false by a SIGINT handler when ctrl-c is
// pressed, then the main loop will cleanly exit.
volatile bool running = true;

void printCanvas(Canvas* canvas, int x, int y, const string& message,
                 int r = 255, int g = 255, int b = 255) {
  // Loop through all the characters and print them starting at the provided
  // coordinates.
  for (auto c: message) {
    // Loop through each column of the character.
    for (int i=0; i<5; ++i) {
      unsigned char col = glcdfont[c*5+i];
      x += 1;
      // Loop through each row of the column.
      for (int j=0; j<8; ++j) {
        // Put a pixel for each 1 in the column byte.
        if ((col >> j) & 0x01) {
          canvas->SetPixel(x, y+j, r, g, b);
        }
      }
    }
    // Add a column of padding between characters.
    x += 1;
  }
}

void printTest(Canvas* canvas, int width, int height)
{
	int r = 0, g = 0, b = 0;
	int total = width * height;
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			r = (int)(255.0*((float)(i * j))/((float)(total)));
			g = (int)(255.0*((float)(i * j)) / ((float)(total)));
			b = (int)(255.0*((float)(i * j)) / ((float)(total)));
			canvas->SetPixel(i, j, r, g, b);
		}
	}
	return;
}

static void sigintHandler(int s) {
  running = false;
}

int main(int argc, char** argv)
{
  try
  {
    Config config(argc >= 2 ? argv[1] : "/dev/null");

	// Initialize GPIO
	rgb_matrix::GPIO io;

	if (!io.Init())
	{
		throw runtime_error("Error while initializing rpi-led-matrix library");
	}
	int width = config.getPanelWidth();
	int height = config.getPanelHeight();
	int chain_length = config.getChainLength();
	int parallel_count = config.getParallelCount();

    RGBMatrix *canvas = new RGBMatrix(&io, height, chain_length, parallel_count);

    int panel_rows = config.getParallelCount();
    int panel_columns = config.getChainLength();
    GridTransformer* grid = config.getGridTransformer();
    canvas->SetTransformer(grid);
    panel_rows = grid->getRows();
    panel_columns = grid->getColumns();

    cout << " grid rows: " << panel_rows << endl
         << " grid cols: " << panel_columns << endl;

    // Clear the canvas, then draw on each panel.
    canvas->Fill(0, 0, 0);
    for (int j=0; j<panel_rows; ++j) {
      for (int i=0; i<panel_columns; ++i) {
        // Compute panel origin position.
        int x = i*config.getPanelWidth();
        int y = j*config.getPanelHeight();
        // Print the current grid position to the top left (origin) of the panel.
        stringstream pos;
        pos << i << "," << j;
        printCanvas(canvas, x, y, pos.str());
      }
    }
	sleep(5);
	printTest(canvas, config.getDisplayWidth(), config.getDisplayHeight());
    // Loop forever waiting for Ctrl-C signal to quit.
    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;
    while (running) {
      sleep(1);
    }
    canvas->Clear();
    delete canvas;
  }
  catch (const exception& ex) {
    cerr << ex.what() << endl;
    return -1;
  }
  return 0;
}
