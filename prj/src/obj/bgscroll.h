#ifndef OBJ_BGSCROLL_H
#define OBJ_BGSCROLL_H

#include "obj.h"

typedef struct O_BgScroll
{
	Obj head;
	uint16_t scroll_y;
} O_BgScroll;

void o_load_bgscroll(Obj *o, uint16_t data);
void o_unload_bgscroll(void);

#endif  // OBJ_BGSCROLL_H
