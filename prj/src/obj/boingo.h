#ifndef OBJ_BOINGO_H
#define OBJ_BOINGO_H

#include "obj.h"
#include <md/megadrive.h>

typedef enum BoingoType
{
	BOINGO_TYPE_NORMAL = 0,
	BOINGO_TYPE_ANGRY = 1,
	BOINGO_TYPE_CUBE = 2,
	BOINGO_TYPE_CUBE_ACTIVE = 3,
} BoingoType;

typedef struct O_Boingo
{
	Obj head;
	BoingoType boingo_type;
	int8_t jump_cnt;
	int8_t anim_cnt;
	int8_t anim_frame;
	int8_t jumping;
} O_Boingo;

void o_load_boingo(Obj *o, uint16_t data);
void o_unload_boingo(void);

#endif  // OBJ_BOINGO_H
