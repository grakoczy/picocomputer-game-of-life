#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>  // for memset
#include "colors.h"
#include "display.h"
#include "cellmap.h"
#include "bitmap_graphics.h"
#include "main.h"

const uint16_t width = cellmap_width;
const uint16_t height = cellmap_height;
const uint16_t length_in_bytes = cellmap_width * cellmap_height;

// draw mode 2 = two pixel wide, 1 = one pixel
uint8_t mode = 2;

// offset for drawing
uint16_t x_offset = 128 + 64 - 10;
uint16_t y_offset = 48 + 2;

uint16_t x_center, y_center;

uint8_t cells[cellmap_width * cellmap_height];
uint8_t temp_cells[cellmap_width * cellmap_height];

uint8_t shapes[NUM_SHAPES][16] = 
    {  
				{1, 0, 0, 0, // Freehand
         0, 0, 0, 0,
         0, 0, 0, 0,
         0, 0, 0, 0},
			
				{0, 1, 0, 0, // Glider
         0, 0, 1, 0,
         1, 1, 1, 0,
         0, 0, 0, 0},
    
        {1, 0, 1, 1, // B-heptomino
         1, 1, 1, 0,
         0, 1, 0, 0,
         0, 0, 0, 0},

        {1, 0, 0, 1, // X-wing
         0, 1, 1, 0,
         0, 1, 1, 0,
         1, 0, 0, 1},

        {0, 1, 1, 0, // Wheel
         1, 1, 1, 1,
         1, 1, 1, 1,
         0, 1, 1, 0},

        {0, 1, 1, 0, // Hi-Hat
         0, 1, 1, 0,
         0, 1, 1, 0,
         1, 1, 1, 1},

        {1, 0, 1, 0, // Shoe
         1, 0, 1, 0,
         1, 0, 0, 1,
         1, 1, 1, 0},

    };

char * shapes_names[NUM_SHAPES] = {"Freehand Solo","Glider","B-heptomino","X-Wing","Wheel","Hi-Hat","Shoe"};
uint16_t shapes_coords[NUM_SHAPES][2] = {{5, 20}, {5, 30}, {5, 40}, {5, 50}, {5, 60}, {5, 70},{5, 80}};

void DrawShape(uint8_t shape[16], int startX, int startY)
{
    // Iterate through the 16 elements (4x4 grid)
    for (int i = 0; i < 4; ++i) // Loop over rows
    {
        for (int j = 0; j < 4; ++j) // Loop over columns
        {
            // Check if the cell should be drawn (1 means filled)
            if (shape[i * 4 + j] == 1)
            {
                // Draw at position (startX + j, startY + i)
									SetCell(startX + j, startY + i);
            }
        }
    }
}

void DrawMenuShape(uint8_t shape[16], int startX, int startY)
{
    // Iterate through the 16 elements (4x4 grid)
    for (int i = 0; i < 4; ++i) // Loop over rows
    {
        for (int j = 0; j < 4; ++j) // Loop over columns
        {
            // Check if the cell should be drawn (1 means filled)
            if (shape[i * 4 + j] == 1)
            {
                // Draw at position (startX + j, startY + i)
                DrawCell(WHITE, startX + j, startY + i, 2, 0, 0);
						}
        }
    }
}

void DrawMenuShape2(uint8_t shape[16], int startX, int startY)
{
    // Iterate through the 16 elements (4x4 grid)
    for (int i = 0; i < 4; ++i) // Loop over rows
    {
        for (int j = 0; j < 4; ++j) // Loop over columns
        {
            // Check if the cell should be drawn (1 means filled)
            if (shape[i * 4 + j] == 1)
            {
                // Draw at position (startX + j, startY + i)
                draw_pixel(WHITE, startX + j * 3 + 0, startY + i * 3 + 0);
                draw_pixel(WHITE, startX + j * 3 + 1, startY + i * 3 + 0);
                draw_pixel(WHITE, startX + j * 3 + 0, startY + i * 3 + 1);
                draw_pixel(WHITE, startX + j * 3 + 1, startY + i * 3 + 1);
						}
        }
    }
}

// CELL STRUCTURE
/* 
Cells are stored in 8-bit chars where the 0th bit represents
the cell state and the 1st to 4th bit represent the number
of neighbours (up to 8). The 5th to 7th bits are unused.
Refer to this diagram: http://www.jagregory.com/abrash-black-book/images/17-03.jpg
*/

void DrawCell(uint16_t color, uint16_t x, uint16_t y, uint8_t mode, uint16_t x_offset, uint16_t y_offset) {

	// WojciechGw
	if( (x >= 0 && y >= 0 && x < 128 && y < 128)){

		int16_t startX = x * 3 + x_offset;
		int16_t startY = y * 3 + y_offset;

		uint8_t shift = 1 * (7 - (startX & 7));
		RIA.step0 = 0;
		color = (color != 0) ? 1 : 0;
		RIA.addr0 = CANVAS_WIDTH/8 * startY + startX/8;
		RIA.rw0 = (RIA.rw0 & ~(1 << shift)) | ((color & 1) << shift);
		if (mode == 2) {
			RIA.addr0 = CANVAS_WIDTH/8 * startY + (startX+1)/8;
			shift = 1 * (7 - ((startX+1) & 7));
			RIA.rw0 = (RIA.rw0 & ~(1 << shift)) | ((color & 1) << shift);

			RIA.addr0 = CANVAS_WIDTH/8 * (startY+1) + (startX)/8;
			shift = 1 * (7 - ((startX) & 7));
			RIA.rw0 = (RIA.rw0 & ~(1 << shift)) | ((color & 1) << shift);

			RIA.addr0 = CANVAS_WIDTH/8 * (startY+1) + (startX+1)/8;
			shift = 1 * (7 - ((startX+1) & 7));
			RIA.rw0 = (RIA.rw0 & ~(1 << shift)) | ((color & 1) << shift);
		}
	}

}


void CellMap()
{
    // Clear all cells to start
    memset(cells, 0, length_in_bytes);

}

void ClearMap() {

    for (uint16_t x = 0; x < width; x++) {
        for (uint16_t y = 0; y < height; y++) {
            // printf("x: %i, y: %i\n", x, y);
            ClearCell(x, y);
        }
    }
    memset(cells, 0, length_in_bytes);

}


void SetCell(uint16_t x, uint16_t y)
{
	int16_t w = width, h = height;
	int16_t xoleft, xoright, yoabove, yobelow;
	uint8_t *cell_ptr = cells + (y * w) + x;

	// Calculate the offsets to the eight neighboring cells,
	// accounting for wrapping around at the edges of the cell map
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;

	*(cell_ptr) |= 0x01; // Set first bit to 1

	// Change successive bits for neighbour counts
	*(cell_ptr + yoabove + xoleft) += 0x02;
	*(cell_ptr + yoabove) += 0x02;
	*(cell_ptr + yoabove + xoright) += 0x02;
	*(cell_ptr + xoleft) += 0x02;
	*(cell_ptr + xoright) += 0x02;
	*(cell_ptr + yobelow + xoleft) += 0x02;
	*(cell_ptr + yobelow) += 0x02;
	*(cell_ptr + yobelow + xoright) += 0x02;

	DrawCell(WHITE, x, y, mode, x_offset, y_offset);
}

void ClearCell(uint16_t x, uint16_t y)
{
	int16_t w = width, h = height;
	int16_t xoleft, xoright, yoabove, yobelow;
	uint8_t *cell_ptr = cells + (y * w) + x;

	// Calculate the offsets to the eight neighboring cells,
	// accounting for wrapping around at the edges of the cell map
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;


	*(cell_ptr) &= ~0x01; // Set first bit to 0

	// Change successive bits for neighbour counts
	*(cell_ptr + yoabove + xoleft) -= 0x02;
	*(cell_ptr + yoabove) -= 0x02;
	*(cell_ptr + yoabove + xoright) -= 0x02;
	*(cell_ptr + xoleft) -= 0x02;
	*(cell_ptr + xoright) -= 0x02;
	*(cell_ptr + yobelow + xoleft) -= 0x02;
	*(cell_ptr + yobelow) -= 0x02;
	*(cell_ptr + yobelow + xoright) -= 0x02;

	DrawCell(BLACK, x, y, mode, x_offset, y_offset);
}

int16_t CellState(int16_t x, int16_t y)
{
	uint8_t *cell_ptr =
		cells + (y * width) + x;

	// Return first bit (LSB: cell state stored here)
	return *cell_ptr & 0x01;
}

void NextGen()
{
	uint16_t x, y, count;
	uint16_t h, w;
	uint8_t *cell_ptr;

    h = height;
    w = width;

	// Copy to temp map to keep an unaltered version
	memcpy(temp_cells, cells, length_in_bytes);

	// Process all cells in the current cell map
	cell_ptr = temp_cells;

	for (y = 0; y < h; y++) {

		x = 0;
		do {

			// Zero bytes are off and have no neighbours so skip them...
			while (*cell_ptr == 0) {
				cell_ptr++; // Advance to the next cell
				// If all cells in row are off with no neighbours go to next row
				if (++x >= w) goto RowDone;
			}

			// Remaining cells are either on or have neighbours
			count = *cell_ptr >> 1; // # of neighboring on-cells
			if (*cell_ptr & 0x01) {

				// On cell must turn off if not 2 or 3 neighbours
				if ((count != 2) && (count != 3)) {
					ClearCell(x, y);
					// setPixel(x+x_offset, y+y_offset, false);
				}
			}
			else {

				// Off cell must turn on if 3 neighbours
				if (count == 3) {
					SetCell(x, y);
					// setPixel(x+x_offset, y+y_offset, true);
				}
			}

			// Advance to the next cell byte
			cell_ptr++;

		} while (++x < w);
	RowDone:;
	}
}