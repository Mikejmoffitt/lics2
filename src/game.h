#ifndef GAME_H
#define GAME_H

#include <stdint.h>

// Constants for the visible screen projection.
#define GAME_SCREEN_W_CELLS 40
#define GAME_SCREEN_H_CELLS 30
#define GAME_SCREEN_W_PIXELS (GAME_SCREEN_W_CELLS * 8)
#define GAME_SCREEN_H_PIXELS (GAME_SCREEN_H_CELLS * 8)

// Constants for the background planes.
#define GAME_PLANE_W_CELLS 64
#define GAME_PLANE_H_CELLS 64
#define GAME_PLANE_W_PIXELS (GAME_PLANE_W_CELLS * 8)
#define GAME_PLANE_H_PIXELS (GAME_PLANE_H_CELLS * 8)

extern uint32_t g_elapsed;

void game_main();

#endif  // GAME_H
