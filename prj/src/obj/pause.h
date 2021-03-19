#ifndef OBJ_PAUSE_H
#define OBJ_PAUSE_H

#include "obj.h"

typedef struct O_Pause
{
	Obj head;
	MdButton buttons_prev;

	int16_t cursor_flash_frame;
	int16_t cursor_flash_cnt;

	int16_t paused;
} O_Pause;

void o_load_pause(Obj *o, uint16_t data);
void o_unload_pause(void);

#endif  // OBJ_PAUSE_H
