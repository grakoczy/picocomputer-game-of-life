// ---------------------------------------------------------------------------
// mouse.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "mouse.h"
#include "display.h"
#include "cellmap.h"

#define MOUSE_DIV 2 // Mouse speed divider

static uint16_t mouse_struct = 0xFF50;
static uint16_t mouse_data = 0xFE60; // to 0xFEC3, since 100 bytes are needed for mouse pointer data
static uint16_t mouse_state = 0xFF40;
static uint16_t mouse_X = 0;
static uint16_t mouse_Y = 0;
static bool left_button_pressed = false;

extern uint8_t shape_selected;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void DrawMousePointer(void)
{
    int i;
    static const uint8_t data[100] = {
        16, 16, 16, 16, 16, 16, 16,  0,  0,  0,
        16,70,70,70,70,70, 16,  0,  0,  0,
        16,70,70,70,70, 16,  0,  0,  0,  0,
        16,70,70,70,70, 16,  0,  0,  0,  0,
        16,70,70,70,70,70, 16,  0,  0,  0,
        16,70, 16, 16,70,70,70,  0,  0,  0,
        16, 16,  0,  0, 16,70,70,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    };
    RIA.addr0 = mouse_data;
    RIA.step0 = 1;
    for (i = 0; i < 100; i++) {
        RIA.rw0 = data[i];
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void InitMouse(void)
{
    // initialize bitmap mode structure (to draw mouse pointer)
    xram0_struct_set(mouse_struct, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, y_wrap, false);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, width_px, 10);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, height_px, 10);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, xram_data_ptr, mouse_data);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);
    xregn( 1, 0, 1, 4, 3, 3, mouse_struct, 2); // mode3 (bitmap), 8-bit color, plane2
    xregn( 0, 0, 1, 1, mouse_state);
    DrawMousePointer();
}

// ----------------------------------------------------------------------------
// returns true if handled click, else false if more work to do by App
// ----------------------------------------------------------------------------
static bool LeftBtnPressed(int16_t x, int16_t y)
{
    bool retval = false;
    uint8_t cell_x, cell_y = 0;


    if(x >= x_offset && y >= y_offset ){
        cell_x = (x - x_offset) / 3;
        cell_y = (y - y_offset) / 3;
        if (shape_selected == 0) {
            SetCell(cell_x, cell_y);
        } else if (shape_selected == 1)
        {
            DrawShape(shapes[0], cell_x, cell_y);
        } else if (shape_selected == 2)
        {
            DrawShape(shapes[1], cell_x, cell_y);
        }
        
        
    }

    return retval;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool LeftBtnReleased(int16_t x, int16_t y)
{
    left_button_pressed = false;
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool RightBtnPressed(int16_t x, int16_t y)
{
    uint8_t cell_x, cell_y = 0;

    if(x >= x_offset && y >= y_offset ){
        cell_x = (x - x_offset) / 3;
        cell_y = (y - y_offset) / 3;
        ClearCell(cell_x, cell_y);        
    }
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool RightBtnReleased(int16_t x, int16_t y)
{
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool HandleMouse(void)
{
    static int sx, sy;
    static uint8_t mb, mx, my;
    int16_t x, y;
    uint8_t rw, changed, pressed, released;
    bool xchg = false;
    bool ychg = false;
    static bool first_time = true;

    // read current X
    RIA.addr0 = mouse_state + 1;
    rw = RIA.rw0;
    if (mx != rw)
    {
        xchg = true;
        sx += (int8_t)(rw - mx);
        mx = rw;
        if (sx < 0)
            sx = 0;
        if (sx > (CANVAS_WIDTH - 2) * MOUSE_DIV)
            sx = (CANVAS_WIDTH - 2) * MOUSE_DIV;
    }

    // read current y
    RIA.addr0 = mouse_state + 2;
    rw = RIA.rw0;
    if (my != rw)
    {
        ychg = true;
        sy += (int8_t)(rw - my);
        my = rw;
        if (sy < 0)
            sy = 0;
        if (sy > (CANVAS_HEIGHT - 2) * MOUSE_DIV)
            sy = (CANVAS_HEIGHT - 2) * MOUSE_DIV;
    }

    // update mouse pointer on screen
    x = sx / MOUSE_DIV;
    y = sy / MOUSE_DIV;
    x -= (x % 3); //snap mouse every 3 pixels
    y -= (y % 3);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, x_pos_px, x);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, y_pos_px, y);
    x++, y++;

    mouse_X = x;
    mouse_Y = y;

    // read button states, and detect changes
    RIA.addr0 = mouse_state + 0;
    rw = RIA.rw0;
    changed = mb ^ rw;
    pressed = rw & changed;
    released = mb & changed;
    mb = rw;

    if (!first_time) {
        // handle button changes
        if (pressed & 1) {
            LeftBtnPressed(x, y);
        }
        if (released & 1) {
            LeftBtnReleased(x, y);
        }
        if (pressed & 2) {
            RightBtnPressed(x, y);
        }
        if (released & 2) {
            RightBtnReleased(x, y);
        }
    } else {
        first_time = false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint16_t mouse_x(void)
{
    return mouse_X;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint16_t mouse_y(void)
{
    return mouse_Y;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint8_t mouse_row(void)
{
    return 0;//(uint8_t)mouse_X/font_width();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint8_t mouse_col(void)
{
    return 0;//(uint8_t)mouse_Y/font_height();
}