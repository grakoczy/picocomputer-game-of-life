#include <stdint.h>
#include <inttypes.h>
extern uint8_t gamestate;
extern bool gamestate_changed;
extern bool shape_changed;

#define GAMESTATE_RUNNING 0
#define GAMESTATE_CREATING 1
#define GAMESTATE_SETUP 2
#define GAMESTATE_MAPCLEARING 3
#define GAMESTATE_SCRSAVE 4
