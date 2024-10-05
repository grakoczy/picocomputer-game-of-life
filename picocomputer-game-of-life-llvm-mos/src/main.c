// based on code from https://github.com/DJayalath/GameOfLife


#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>  // for memset
#include "colors.h"
#include "bitmap_graphics.h"

#define WIDTH 320
#define HEIGHT 200


char msg[80] = {0};



// CELL STRUCTURE
/* 
Cells are stored in 8-bit chars where the 0th bit represents
the cell state and the 1st to 4th bit represent the number
of neighbours (up to 8). The 5th to 7th bits are unused.
Refer to this diagram: http://www.jagregory.com/abrash-black-book/images/17-03.jpg
*/



// Cell map dimensions
#define cellmap_width  128
#define cellmap_height  128
const uint16_t width = cellmap_width;
const uint16_t height = cellmap_height;
const uint16_t length_in_bytes = cellmap_width * cellmap_height;

uint8_t cells[length_in_bytes];
uint8_t temp_cells[length_in_bytes];



// offset for drawing
uint16_t x_offset = 90;
uint16_t y_offset = 30;

uint16_t x_center, y_center;




// XRAM locations
#define KEYBOARD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data

// HID keycodes that we care about for this demo
#define KEY_ESC 41 // Keyboard ESCAPE
#define KEY_SPC 44 // Keyboard SPACE

// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
bool handled_key = false;

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))


static void CellMap()
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


static void SetCell(uint16_t x, uint16_t y)
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
    draw_pixel(WHITE, x+x_offset, y+y_offset);
}

static void ClearCell(uint16_t x, uint16_t y)
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
    draw_pixel(BLACK, x+x_offset, y+y_offset);
}

static int16_t CellState(int16_t x, int16_t y)
{
	uint8_t *cell_ptr =
		cells + (y * width) + x;

	// Return first bit (LSB: cell state stored here)
	return *cell_ptr & 0x01;
}

static void NextGen()
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


static void setup()
{
    init_bitmap_graphics(0xFF00, 0x0000, 0, 1, 320, 240, 1);//, 0, HEIGHT);
    erase_canvas();

	x_center = cellmap_width / 2 - 1;
	y_center = cellmap_height /2 - 1;

	printf("xc: %i, yc: %i", x_center, y_center);
    xregn(1, 0, 1, 0, 1, HEIGHT, 240);

    CellMap();


    draw_hline(WHITE, x_offset-1, y_offset-1, cellmap_width+2);
    draw_hline(WHITE, x_offset, y_offset+cellmap_height, cellmap_width+1);
    draw_vline(WHITE, x_offset-1, y_offset, cellmap_height+1);
    draw_vline(WHITE, x_offset+cellmap_width, y_offset, cellmap_height+1);

    // glider
	// SetCell(11, 20);
	// SetCell(12, 21);
	// SetCell(10, 22);
	// SetCell(11, 22);
	// SetCell(12, 22);

	// glider generator
	// 4-8-12 diamond
	SetCell(x_center-1, y_center - 4);
	SetCell(x_center, y_center - 4);
	SetCell(x_center+1, y_center - 4);
	SetCell(x_center+2, y_center - 4);

	SetCell(x_center-3, y_center - 2);
	SetCell(x_center-2, y_center - 2);
	SetCell(x_center-1, y_center - 2);
	SetCell(x_center, y_center - 2);
	SetCell(x_center+1, y_center - 2);
	SetCell(x_center+2, y_center - 2);
	SetCell(x_center+3, y_center - 2);
	SetCell(x_center+4, y_center - 2);


	SetCell(x_center-5, y_center);
	SetCell(x_center-4, y_center);
	SetCell(x_center-3, y_center);
	SetCell(x_center-2, y_center);
	SetCell(x_center-1, y_center);
	SetCell(x_center, y_center);
	SetCell(x_center+1, y_center);
	SetCell(x_center+2, y_center);
	SetCell(x_center+3, y_center);
	SetCell(x_center+4, y_center);
	SetCell(x_center+5, y_center);
	SetCell(x_center+6, y_center);


	SetCell(x_center-3, y_center + 2);
	SetCell(x_center-2, y_center + 2);
	SetCell(x_center-1, y_center + 2);
	SetCell(x_center, y_center + 2);
	SetCell(x_center+1, y_center + 2);
	SetCell(x_center+2, y_center + 2);
	SetCell(x_center+3, y_center + 2);
	SetCell(x_center+4, y_center + 2);

	SetCell(x_center-1, y_center + 4);
	SetCell(x_center, y_center + 4);
	SetCell(x_center+1, y_center + 4);
	SetCell(x_center+2, y_center + 4);
	
}

int16_t main()
{
    char msg[80] = {0};
	uint8_t i;
	// Generation counter
	uint16_t generation = 0;


    puts("\n\n\n");
	

    setup();

	puts("starting...");

	set_cursor(10, 220);
    draw_string("Press SPACE to start, ESC to exit");

    // wait for a keypress
    xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
    RIA.addr0 = KEYBOARD_INPUT;
    RIA.step0 = 0;
    while (1) {

        // fill the keystates bitmask array
        for (i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t j, new_keys;
            RIA.addr0 = KEYBOARD_INPUT + i;
            new_keys = RIA.rw0;
///*
            // check for change in any and all keys
            for (j = 0; j < 8; j++) {
                uint8_t new_key = (new_keys & (1<<j));
                if ((((i<<3)+j)>3) && (new_key != (keystates[i] & (1<<j)))) {
                    printf( "key %d %s\n", ((i<<3)+j), (new_key ? "pressed" : "released"));
                }
            }
//*/
            keystates[i] = new_keys;
        }

        // check for a key down
        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (key(KEY_SPC)) {
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }
    }


    while (1) {

        generation++;
        printf("\ngeneration %i", generation);

		sprintf(msg, "gen: %i", generation);
		fill_rect(BLACK, 5, 20, 10*6, 8);
		set_text_color(WHITE);
		set_cursor(5, 20);
		draw_string(msg);

        NextGen();

		

        xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
        RIA.addr0 = KEYBOARD_INPUT;
        RIA.step0 = 0;
        

        // fill the keystates bitmask array
        for (i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t j, new_keys;
            RIA.addr0 = KEYBOARD_INPUT + i;
            new_keys = RIA.rw0;

            // check for change in any and all keys
            for (j = 0; j < 8; j++) {
                uint8_t new_key = (new_keys & (1<<j));
                // if ((((i<<3)+j)>3) && (new_key != (keystates[i] & (1<<j)))) {
                //     printf( "key %d %s\n", ((i<<3)+j), (new_key ? "pressed" : "released"));
                // }
            }

            keystates[i] = new_keys;
        }

        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (key(KEY_ESC)) {
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }
    };

	return 0;
    
}
