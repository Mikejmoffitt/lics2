#ifndef MAP_H
#define MAP_H

// Room management singleton

// The map is responsible for setting up a room with data from a map file hwen
// a new room is entered.

// In addition, during gameplay, the map is the object responsible for
// scrolling the map around and redrawing foreground tiles.

#include "obj.h"
#include "map_file.h"
#include "md/megadrive.h"

#include <stdint.h>

typedef enum MapExitTrigger
{
	MAP_EXIT_NONE,
	MAP_EXIT_TOP,
	MAP_EXIT_BOTTOM,
	MAP_EXIT_LEFT,
	MAP_EXIT_RIGHT,
	MAP_EXIT_TELEPORT,
	AMP_EXIT_OTHER
} MapExitTrigger;

typedef struct O_Map
{
	Obj head;
	const MapFile *current_map;
	uint32_t current_map_size;
	MapExitTrigger exit_trigger;

	int16_t fresh_room;

	// Map limits, in subpixels
	fix32_t right;
	fix32_t bottom;

	// Scroll position, derived from Lyle's position.
	int16_t x_scroll;
	int16_t y_scroll;

	// Scroll from the previous frame. Used to detect changes and direction.
	int16_t x_scroll_prev;
	int16_t y_scroll_prev;

	uint8_t next_room_id;
	uint8_t next_room_entrance;
} O_Map;

extern const uint16_t *g_map_data;
extern uint16_t g_map_row_size;

void o_load_map(Obj *o, uint16_t data);
void o_unload_map(void);


// Public singleton functions

// Load a map by ID number. In particular:
// * Sets the current map pointer
// * Populates the object list with entities from the map file ( --> lyle.c )
// * Sets the BG based on map file (--> bg.c )
// * Queues DMA for the sprite, enemy palettes
// * Queues DMA for the tileset
void map_load(uint8_t id, uint8_t entrance_num);

// Map metadata.
uint8_t map_get_music_track(void);
uint8_t map_get_background(void);

static inline uint8_t map_is_tile_harmful(uint8_t tile)
{
	return tile >= 0xE0;
}

static inline uint8_t map_is_tile_solid(uint8_t tile)
{
	return tile >= 0x80 && tile < 0xE0;
}

static inline uint16_t map_data_at(int16_t x, int16_t y)
{
	const uint16_t check_addr = (y / 8) * (g_map_row_size) + (x / 8);
	return g_map_data[check_addr];
}

static inline uint16_t map_collision(int16_t x, int16_t y)
{
	return map_is_tile_solid(map_data_at(x, y) & 0xFF);
}

fix32_t map_get_right(void);
fix32_t map_get_bottom(void);

int16_t map_set_x_scroll(int16_t x);
int16_t map_set_y_scroll(int16_t y);

int16_t map_get_x_scroll(void);
int16_t map_get_y_scroll(void);

void map_set_next_room(uint8_t id, uint8_t entrance);
uint8_t map_get_next_room_id(void);
uint8_t map_get_next_room_entrance(void);

void map_redraw_room(void);

void map_set_exit_trigger(MapExitTrigger t);
MapExitTrigger map_get_exit_trigger(void);

#endif  // MAP_H
