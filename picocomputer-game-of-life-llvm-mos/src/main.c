// Game Of Life
// based on code from https://github.com/DJayalath/GameOfLife
// by Grzegorz Rakoczy
// for Rumbledethumps' Picocomputer 6502 
// some minor changes and improvements by Wojciech Gwiozdzik

// #define SHOWFACE

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>  // for memset
#include <time.h>
#include "colors.h"
#include "bitmap_graphics.h"
#include "usb_hid_keys.h"
#include "mouse.h"
#include "cellmap.h"

char msg[80] = {0};

// XRAM locations
#define KEYBOARD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data

// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
bool handled_key = false;

int fd = 0; // files descriptor

char * starttext[] = {
	"John Conway's Game of Life",
	"is a cellular automaton that is played on a 2D square grid.",
	"Each square (or 'cell') on the grid can be either alive or dead,",
	"and they evolve according to the following rules:",
	"1. Any live cell with fewer than two live neighbours dies (referred to as underpopulation).",
	"2. Any live cell with more than three live neighbours dies (referred to as overpopulation).",
	"3. Any live cell with two or three live neighbours lives, unchanged, to the next generation.",
	"4. Any dead cell with exactly three live neighbours comes to life.",
	"The initial configuration of cells can be created by a human, but all generations thereafter",
	"are completely determined by the above rules.",
	"The goal of the game is to find patterns that evolve in interesting ways - something",
	"that people have now been doing for over 50 years.",
	"",
	"find out more at https://conwaylife.com/",
	"",
	"<press any key>"
};
uint8_t starttext_lines = (sizeof(starttext) / sizeof(starttext[0]));

uint8_t shapes_num = NUM_SHAPES;
uint8_t shape_selected = 0;
bool shape_changed = true;
bool gamestate_changed = true;
uint8_t gamestate = 2;  //  0          1         2        3
uint8_t gamestate_prev = 2;
char * gamestates[5] =  {"run", "design", "setup", "map clearing", "screenshot"};
#define GAMESTATE_RUNNING 0
#define GAMESTATE_CREATING 1
#define GAMESTATE_SETUP 2
#define GAMESTATE_MAPCLEARING 3
#define GAMESTATE_SCRSAVE 4

#define LASTGENERATION 9999999

uint16_t menu_x = 8;
uint16_t menu_y = 60;

#define NUM_HELPS 7
char * help_texts[NUM_HELPS] = { "[LMB] set cell/select shape",
                                 "[RMB]            clear cell",
																 "[MMB|SPACE]    run | design",
																 "[C]               clear map",
																 "[I]    set initial patterns",
																 "[S] screenshot (lifesc.bmp)",
																 "[ESC]                  exit"};
uint16_t help_x = 8;
uint16_t help_y = 470 - ((NUM_HELPS + 3) * 16);

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

void InitialPatterns(){

	// set initial pattern
	//
	for(int g = -1; g <= 1; g++){
		for(int i = -2; i < 3; i++){ 
			SetCell(x_center + i + g * 40, y_center - 4 + g * 40);
			SetCell(x_center + i + g * 40, y_center + 4 + g * 40);
		}
		for(int i = -4; i < 5; i++){
			SetCell(x_center + i + g * 40, y_center - 2 + g * 40);
			SetCell(x_center + i + g * 40, y_center + 2 + g * 40);
		}
		for(int i = -6; i < 7; i++) SetCell(x_center + i + g * 40, y_center + g * 40);
	}	

	// set random predefined shapes
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center + 40, y_center - 40);
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center - 40, y_center + 40);
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center - 40, y_center     );
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center + 40, y_center     );
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center     , y_center - 40);
	DrawShape(shapes[random(1, NUM_SHAPES - 1)], x_center     , y_center + 40);

}

int WaitForAnykey(){

		while(1)
		{
			xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
			RIA.addr0 = KEYBOARD_INPUT;
			RIA.step0 = 0;

			// fill the keystates bitmask array
			for (int i = 0; i < KEYBOARD_BYTES; i++) {
					uint8_t j, new_keys;
					RIA.addr0 = KEYBOARD_INPUT + i;
					new_keys = RIA.rw0;
					// check for change in any and all keys
					for (j = 0; j < 8; j++) {
							uint8_t new_key = (new_keys & (1<<j));
					}
					keystates[i] = new_keys;
			}

        // check for a key down
        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (key(KEY_ESC)) {
                    break;
                } else {
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }

		}

		return 1;
}

static void setup()
{

	puts("\x0c");
	puts("\033[32mGame Of Life\033[0m\nbased on code from https://github.com/DJayalath/GameOfLife\nfor Rumbledethumps' Picocomputer 6502\nby grakoczy\nsome minor changes & improvements by WojciecGw\nplease wait...\n");
  
	CellMap();

	fd = open("startscreen.bmp", O_RDONLY);
	if(fd >= 0){
			erase_canvas();
			read_xram(0x9600,0x003E,fd); // BMP header to XRAM address 0x9900 (after screen buffer)
			read_xram(0x0000,0x4B00,fd); // 1st half
			read_xram(0x4B00,0x4B00,fd); // 2nd half
			close(fd);
			// dump startscreen data to file
			fd = open("startscreen.bin",O_CREAT | O_WRONLY);
			if(fd >= 0){
				write_xram(0x0000,0x4B00,fd);
				write_xram(0x4B00,0x4B00,fd);
				close(fd);
			} else {
					printf("ERROR: writing startscreen.bin file %i\n\n", fd);
			}
			// dump startscreen BMP header to file
			fd = open("bmphead.bin",O_CREAT | O_WRONLY);
			if(fd >= 0){
				write_xram(0x9600,0x003E,fd);
				close(fd);
			} else {
					printf("ERROR: writing bmphead.bin file %i\n\n", fd);
			}
	} else {
			//erase_canvas();
			printf("INFORMATION: startscreen.bmp file not exists\nusing embeded startscreen\n\n");
	}

	// Your face bottom right corner
	#ifdef SHOWFACE
	fd = open("face.bmp", O_RDONLY);
	if(fd >= 0){
			uint32_t datasize = (lseek(fd, 0x003E, SEEK_END) - 0x3E);
			lseek(fd, 0x003E, SEEK_SET); // omit BMP header
			read_xram(380*80, datasize, fd);
			close(fd);
	} else {
			printf("INFORMATION: face.bmp file not exists\n\n");
	}
	#endif

	init_bitmap_graphics(0xFF00, 0x0000, 0, 3, 640, 480, 1);

	// start screen text
	set_text_color(WHITE);
	for(int t = 0; t < starttext_lines; t++){
		set_cursor(10, 100 + t * 16);
		draw_string(starttext[t]);
	}

	// uint32_t ticks = clock();
	// while(clock() < (ticks + 500)){} // wait 5 seconds

	WaitForAnykey();

	set_text_color(BLACK);
	for(int t = starttext_lines; t > -1; t--){
		set_cursor(10, 100 + t * 16);
		draw_string(starttext[t]);
	}
	
	x_center = cellmap_width / 2 - 1;
	y_center = cellmap_height / 2 - 1;

	// draw playfield
	draw_rect(WHITE, x_offset-3, y_offset - 3, cellmap_width * 3 + 5, cellmap_height * 3 + 5);
	// draw_rect(WHITE, x_offset-2, y_offset - 2, cellmap_width * 3 + 3, cellmap_height * 3 + 3);
	// fill_rect(BLACK, x_offset-1, y_offset - 1, cellmap_width * 3 + 1, cellmap_height * 3 + 1);
	draw_hline(WHITE, 10, 455, 300); // 569

	set_cursor(cellmap_width * 3 + x_offset - 130 , cellmap_height * 3 + y_offset + 2);
	set_text_colors(BLACK, WHITE);
	draw_string(" generation # 0000000 ");
	set_text_colors(WHITE, BLACK);

	// help
	set_cursor(help_x, help_y);
	draw_string("MOUSE & KEYBOARD");
	for(int s = 0; s < NUM_HELPS; s++){
		set_cursor(help_x + 4 , help_y + 20 + s * 16);
		draw_string(help_texts[s]);
	};

	set_cursor(10, 464);
	draw_string("state:");

	InitialPatterns();

	InitMouse();

}

int main()
{
  char msg[80] = {0};
	uint8_t i;
	
	// Generation counter
	uint32_t generation = 0;

	gamestate = GAMESTATE_SETUP;
	gamestate_changed = true;
	set_cursor(50, 464);
	draw_string("setting up");
  setup();
	gamestate = GAMESTATE_CREATING;
	gamestate_changed = true;

	while (1) {
		
		HandleMouse();

		xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
		RIA.addr0 = KEYBOARD_INPUT;
		RIA.step0 = 0;

		if(gamestate_changed)
		{
			// fill_rect(BLACK, 50, 464, 300, 20);
			set_text_colors(WHITE, BLACK);
			set_cursor(50,464);
			draw_string("                        ");
			set_cursor(50,464);
			draw_string(gamestates[gamestate]);
		}

		if(gamestate == GAMESTATE_CREATING && gamestate_changed) {
  		// show menu
			set_cursor(menu_x, menu_y);
			draw_string("PREDEFINED SHAPES");
			for(int s = 0; s < shapes_num; s++){
				shapes_coords[s][0] = menu_x + 2;
				shapes_coords[s][1] = menu_y + 20 + s * 17;
				DrawMenuShape2(shapes[s], shapes_coords[s][0], shapes_coords[s][1]);
				set_cursor(shapes_coords[s][0] + 26, shapes_coords[s][1] + 1);
				draw_string(shapes_names[s]);
				// draw assigned keypress
				// set_cursor(shapes_coords[s][0] + 110, shapes_coords[s][1] + 1);
				// sprintf(msg, "[key %i]", shapes_keypress[s]);
				// draw_string(msg);
			};
			gamestate_changed = false;
		}

		if(gamestate == GAMESTATE_CREATING && shape_changed){
			set_cursor(menu_x, menu_y + 35 + shapes_num * 17);
			draw_string("SHAPE SELECTED");
			fill_rect(BLACK, menu_x + 2, menu_y + 55 + shapes_num * 17, 18, 18);
			DrawMenuShape2(shapes[shape_selected], menu_x + 2, menu_y + 55 + shapes_num * 17);
			set_cursor(menu_x + 26, menu_y + 55 + shapes_num * 17 + 2);
			draw_string("                     ");
			set_cursor(menu_x + 26, menu_y + 55 + shapes_num * 17 + 2);
			draw_string(shapes_names[shape_selected]);
			shape_changed = false;
		}

		if(gamestate == GAMESTATE_MAPCLEARING){
			ClearMap();
			generation = 0;
			set_cursor(cellmap_width * 3 + x_offset - 130 , cellmap_height * 3 + y_offset + 2);
			set_text_colors(BLACK, WHITE);
			draw_string(" generation # 0000000 ");
			set_text_colors(WHITE, BLACK);
			gamestate = GAMESTATE_CREATING;
			gamestate_changed = true;
		}

		if (gamestate == GAMESTATE_RUNNING) {
			if(generation < LASTGENERATION){
				generation++;
				sprintf(msg, " generation # %07lu ", generation);
				set_cursor(cellmap_width * 3 + x_offset - 130 , cellmap_height * 3 + y_offset + 2);
				set_text_colors(BLACK, WHITE);
				draw_string(msg);
				set_text_colors(WHITE, BLACK);
				NextGen();
				gamestate_changed = false;
			} else {
				gamestate = GAMESTATE_MAPCLEARING;
				gamestate_changed = true;
			}
		}

		if (gamestate == GAMESTATE_SCRSAVE) {
			int fd = open("lifesc.bmp", O_CREAT | O_WRONLY);
			if(fd >= 0){
					write_xram(0x9600,0x003E,fd); // BMP header
					write_xram(0x0000,0x4B00,fd); // 1st half of screen buffer
					write_xram(0x4B00,0x4B00,fd); // 2nd half of screen buffer
					close(fd);
			} else {
					puts("\x0c");
					printf("ERROR: lifesc.bmp %i\n\n", fd);
					break;
			}
			gamestate = gamestate_prev;
			gamestate_changed = true;
		}

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
				if (key(KEY_C)){
					if(gamestate == GAMESTATE_CREATING){ 
						gamestate = GAMESTATE_MAPCLEARING;
						gamestate_changed = true;
					}
				}
				if (key(KEY_I)){
					InitialPatterns();
					gamestate = GAMESTATE_CREATING; 
					gamestate_changed = true;
				}
				if (key(KEY_S)){
					gamestate_prev = gamestate;
					gamestate = GAMESTATE_SCRSAVE;
					gamestate_changed = true;
				}
				if (key(KEY_SPACE)){
					gamestate = ((gamestate == GAMESTATE_CREATING) ? GAMESTATE_RUNNING : GAMESTATE_CREATING);
					gamestate_changed = true;
					shape_changed = true;
				}
				if (key(KEY_ESC)) {
						erase_canvas();
						puts("\x0c");
						puts("\033[32mGame Of Life\033[0m\nthank You for playing :)\n\n");
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
