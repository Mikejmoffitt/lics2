#ifndef OBJ_LAVA_H
#define OBJ_LAVA_H

#include "obj.h"

typedef struct O_Lava O_Lava;
struct O_Lava
{
	Obj head;

	Obj *cow;

	int16_t px;
	int16_t splat_py;

	int16_t anim_cnt;
	int16_t anim_frame;
	
	int16_t size;

	fix32_t max_y;
	int16_t generator_cnt;

	int16_t splat_cnt;
	int16_t splat_anim_cnt;
	int16_t splat_anim_frame;

	int16_t is_generator;
};

void o_load_lava(Obj *o, uint16_t data);
void o_unload_lava(void);

#endif  // OBJ_LAVA_H
