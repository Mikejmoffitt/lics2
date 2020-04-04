#ifndef MAP_H
#define MAP_H

// Clear map structures
void map_init(void);

// Which tilset is loaded?
uint8_t map_get_current_tileset(void);

// DMA a tileset into VRAM; prepare for palette
void map_load_tileset(uint8_t num);

// Return the map by its ID number
const MapFile *map_by_id(uint8_t num);

// Update the screen as needed based on movement differences
void map_draw_diffs(uint16_t moved);
void map_draw_full(uint16_t cam_x, uint16_t cam_y);
void map_draw_vertical(uint16_t cam_x, uint16_t cam_y, uint16_t bottom_side);
void map_draw_horizontal(uint16_t cam_x, uint16_t cam_y, uint16_t right_side);

// Commit DMA queue to VRAM
void map_dma(void);

// Show a map selection screen
void map_debug_chooser(void);

// Is this spot solid? (background)
uint16_t map_collision(uint16_t x, uint16_t y);

// Is this spot going to hurt me? (background)
uint16_t map_hurt(uint16_t x, uint16_t y);

#endif  // MAP_H
