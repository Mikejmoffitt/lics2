#ifndef OBJ_PILLA_H
#define OBJ_PILLA_H

#include "obj.h"
#include <stdbool.h>

typedef struct O_Pilla
{
	Obj head;
	int16_t anim_cnt;
	int16_t anim_frame;
	bool is_head;
} O_Pilla;

void o_load_pilla(Obj *o, uint16_t data);
void o_unload_pilla(void);

#endif  // OBJ_PILLA_H
