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
#include "usb_hid_keys.h"
#include "mouse.h"
#include "cellmap.h"

char msg[80] = {0};


bool paused = true;


// XRAM locations
#define KEYBOARD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data


// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
bool handled_key = false;

uint16_t shapes_coords[2][2] = {{5, 20}, {5, 30}};
uint8_t shape_selected = 0;

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))




// void DrawShape(uint16_t color, uint16_t x, uint16_t y, uint8_t shape_number) {


// 	printf("%i, %i, %i, %i \n", shapes[shape_number][4], shapes[shape_number][5], shapes[shape_number][6], shapes[shape_number][7]);
// 	for (uint8_t i = 0; i < 4; i++) {
// 		for (uint8_t j = 0; j < 4; j++) {
// 			uint8_t on = shapes[shape_number][i+j];
// 			// printf("i: %i, j: %i, on: %i", i, j, on);
// 			printf(" %i ", on);
// 			if (on == 1) {
// 				SetCell(x+j, y+x);
// 			}
			
// 		}
// 		printf("\n");
// 	}
// }



static void setup()
{
    // init_bitmap_graphics(0xFF00, 0x0000, 0, 1, 320, 240, 1);//, 0, CANVAS_HEIGHT);
	init_bitmap_graphics(0xFF00, 0x0000, 0, 3, 640, 480, 1);

    erase_canvas();

	x_center = cellmap_width / 2 - 1;
	y_center = cellmap_height /2 - 1;

	// printf("xc: %i, yc: %i", x_center, y_center);
    // xregn(1, 0, 1, 0, 1, CANVAS_HEIGHT, 240);

    CellMap();


    draw_hline(WHITE, x_offset-1, y_offset-1, cellmap_width*3+2);
    draw_hline(WHITE, x_offset, y_offset+cellmap_height*3, cellmap_width*3+1);
    draw_vline(WHITE, x_offset-1, y_offset, cellmap_height*3+1);
    draw_vline(WHITE, x_offset+cellmap_width*3, y_offset, cellmap_height*3+1);

    set_cursor(16, 20);
	draw_string("gen: 0");

	DrawMenuShape(shapes[0], shapes_coords[0][0], shapes_coords[0][1]);
	set_cursor(30, 60);
	draw_string(" glider (press 1)");
	DrawMenuShape(shapes[1], shapes_coords[1][0], shapes_coords[1][1]);
	set_cursor(30, 90);
	draw_string(" B-heptomino (press 2)");

	set_cursor(16, 120);
	draw_string("free drawing press 0");

	set_cursor(16, 140);
	sprintf(msg, "shape selected: %i", shape_selected);
	draw_string(msg);


	set_cursor(10, 400);
    draw_string("LMB set cell");

	set_cursor(10, 410);
    draw_string("RMB clear cell");

	set_cursor(10, 420);
    draw_string("SPACE to start/pause");

	set_cursor(10, 430);
    draw_string("C to clear map");

	set_cursor(10, 440);
    draw_string("ESC to exit");

	// DrawShape(shapes[0], 5, 10);
	// DrawShape(shapes[1], 25, 80);

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

	InitMouse();
	
}

int16_t main()
{
    char msg[80] = {0};
	uint8_t i;
	// Generation counter
	uint16_t generation = 0;


    puts("\n\n\n");
	

    setup();

	puts("\nstarting...");



    while (1) {
		HandleMouse();

		if (!paused) {
			generation++;
			// printf("\ngeneration %i", generation);

			sprintf(msg, "%i", generation);
			fill_rect(BLACK, 46, 20, 5*6, 8);
			set_text_color(WHITE);
			set_cursor(46, 20);
			draw_string(msg);

			NextGen();
		} 		

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
				if (key(KEY_0)){
					shape_selected = 0;
					set_cursor(16, 140);
					fill_rect(BLACK, 16+16*6, 140, 10, 8);
					sprintf(msg, "shape selected: %i", shape_selected);
					draw_string(msg);
				}
				if (key(KEY_1)) {
                    shape_selected = 1;
					set_cursor(16, 140);
					fill_rect(BLACK, 16+16*6, 140, 10, 8);
					sprintf(msg, "shape selected: %i", shape_selected);
					draw_string(msg);
                }
				if (key(KEY_2)){
					shape_selected = 2;
					set_cursor(16, 140);
					fill_rect(BLACK, 16+16*6, 140, 10, 8);
					sprintf(msg, "shape selected: %i", shape_selected);
					draw_string(msg);
				}
				if (key(KEY_C)){
					ClearMap();
					generation = 0;
				}
				if (key(KEY_SPACE)){
					paused = !paused;
				}
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
