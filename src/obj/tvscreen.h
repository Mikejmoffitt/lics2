#ifndef OBJ_TVSCREEN_H
#define OBJ_TVSCREEN_H

#include "obj.h"
#include <stdbool.h>

typedef struct O_TvScreen
{
	Obj head;
	uint16_t id;
	bool active;
	uint16_t anim_frame;
	uint16_t anim_cnt;
	uint16_t attr;
} O_TvScreen;

void o_load_tvscreen(Obj *o, uint16_t data);
void o_unload_tvscreen(void);

#endif  // OBJ_TVSCREEN_H
