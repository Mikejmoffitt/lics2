#ifndef OBJ_HUD_H
#define OBJ_HUD_H

#include "obj.h"

typedef struct O_Hud
{
	Obj head;
} O_Hud;

void o_load_hud(Obj *o, uint16_t data);
void o_unload_hud(void);

#endif  // OBJ_HUD_H
