#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include <stdint.h>

void hud_init(void);
void hud_render(void);
void hud_set_visible(bool visible);

#endif  // HUD_H
