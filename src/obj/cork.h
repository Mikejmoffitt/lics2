#ifndef OBJ_CORK_H
#define OBJ_CORK_H

#include "obj.h"

typedef struct O_Cork
{
	Obj head;
	uint16_t storage;
	int16_t anim_cnt;
	int16_t anim_frame;
} O_Cork;

void o_load_cork(Obj *o, uint16_t data);
void o_unload_cork(void);

#endif  // OBJ_CORK_H
