#ifndef OBJ_HUD_H
#define OBJ_HUD_H

#include "obj.h"

typedef struct O_Hud
{
	Obj head;
} O_Hud;

void o_load_hud(Obj *o, uint16_t data);
void o_unload_hud(void);

void hud_set_visible(int16_t visible);

#endif  // OBJ_HUD_H
