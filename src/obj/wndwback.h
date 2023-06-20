#ifndef OBJ_WNDWBACK_H
#define OBJ_WNDWBACK_H

#include "obj.h"
#include <stdbool.h>

typedef struct O_Wndwback
{
	Obj head;
	bool visible;
} O_Wndwback;

void o_load_wndwback(Obj *o, uint16_t data);
void o_unload_wndwback(void);
void wndwback_set_visible(bool visible);

#endif  // OBJ_WNDWBACK_H
