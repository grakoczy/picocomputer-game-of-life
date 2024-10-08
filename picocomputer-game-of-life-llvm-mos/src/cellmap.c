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



const uint16_t width = cellmap_width;
const uint16_t height = cellmap_height;
const uint16_t length_in_bytes = cellmap_width * cellmap_height;

// draw mode 2 = two pixel wide, 1 = one pixel
uint8_t mode = 2;


// offset for drawing
uint16_t x_offset = 128+64;
uint16_t y_offset = 48;

uint16_t x_center, y_center;

uint8_t cells[length_in_bytes];
uint8_t temp_cells[length_in_bytes];

// CELL STRUCTURE
/* 
Cells are stored in 8-bit chars where the 0th bit represents
the cell state and the 1st to 4th bit represent the number
of neighbours (up to 8). The 5th to 7th bits are unused.
Refer to this diagram: http://www.jagregory.com/abrash-black-book/images/17-03.jpg
*/

void DrawCell(uint16_t color, uint16_t x, uint16_t y, uint8_t mode) {
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


void CellMap()
{
    // width = w;
    // height = h;
    // length_in_bytes = w * h;

    // Use malloc for dynamic memory allocation
    // cells = (uint8_t*)malloc(length_in_bytes)
	// cells = (uint8_t*)malloc(length_in_bytes * sizeof(uint8_t));   // cell storage
	// temp_cells = (uint8_t*)malloc(length_in_bytes * sizeof(uint8_t)); // temp cell storage


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

    // setPixel(x+x_offset, y+y_offset, true);
    // draw_pixel(WHITE, x+x_offset, y+y_offset);
	DrawCell(WHITE, x, y, mode);
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

    // setPixel(x+x_offset, y+y_offset, false);
    // draw_pixel(BLACK, x+x_offset, y+y_offset);
	DrawCell(BLACK, x, y, mode);
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