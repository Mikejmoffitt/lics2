#ifndef OBJ_BG_H
#define OBJ_BG_H

#include "obj.h"

typedef enum BgScroll
{
	BG_SCROLL_NONE,
	BG_SCROLL_PLANE,
	BG_SCROLL_H_CELL,
	BG_SCROLL_V_CELL,
} BgScroll;

typedef struct O_Bg
{
	Obj head;
	BgScroll scroll_mode;
	uint8_t bg_id;
} O_Bg;

void o_load_bg(Obj *o, uint16_t data);
void o_unload_bg(void);

#endif  // OBJ_BG_H
