
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
extern const uint16_t width;
extern const uint16_t height;
extern const uint16_t length_in_bytes;

// draw mode 2 = two pixel wide, 1 = one pixel
extern uint8_t mode;


// offset for drawing
extern uint16_t x_offset;
extern uint16_t y_offset;

extern uint16_t x_center, y_center;

extern uint8_t shapes[2][16];

void DrawShape(uint8_t shape[16], int startX, int startY);

void DrawMenuShape(uint8_t shape[16], int startX, int startY);

void DrawCell(uint16_t color, uint16_t x, uint16_t y, uint8_t mode, uint16_t x_offset, uint16_t y_offset);

void CellMap();

void ClearMap();

void SetCell(uint16_t x, uint16_t y);


void ClearCell(uint16_t x, uint16_t y);

int16_t CellState(int16_t x, int16_t y);

void NextGen();
