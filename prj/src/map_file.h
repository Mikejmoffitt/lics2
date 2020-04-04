#ifndef MAP_FILE_H
#define MAP_FILE_H

#include <stdint.h>

typedef struct MapObj
{
	uint16_t type; // Object ID
	uint16_t data; // Depends on object type. Best example is entrance IDs.
	uint16_t x; // X in real coordinates (pixel-precise)
	uint16_t y; // Y in real coordinates (pixel-precise)
} MapObj;

typedef struct MapFile
{
	// Room name (mostly for the editor)
	char name[32];

	uint8_t music;

	// Unique room identifier
	uint8_t id;

	// Dimensions for the room in screens
	uint8_t ex1;  // Padding
	uint8_t w;
	uint8_t ex2;  // Padding
	uint8_t h;

	// Position in top-left of game-wide map
	uint8_t map_x;
	uint8_t map_y;

	// Graphics tileset and palette to display with (enum)
	uint8_t tileset;

	// Palette for enemies, objects
	uint8_t sprite_palette;

	// Which background to choose from (enum); palette implied
	uint8_t background;

	// Large array of map objects for the object list (null terminated)
	MapObj objects[128];

	// Starting point of map data. Overlaid over real binary data.
	uint16_t map_data[0];
} MapFile;

#endif
