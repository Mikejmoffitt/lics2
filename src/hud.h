#ifndef HUD_H
#define HUD_H

#include <stdint.h>

void hud_load(void);  // Loads HUD resources into dynamic VRAM.
void hud_render(void);
void hud_set_visible(uint16_t visible);

#endif  // HUD_H
