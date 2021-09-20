#ifndef OBJ_FAKECUBE_H
#define OBJ_FAKECUBE_H

#include "obj.h"
#include "cube.h"

typedef struct O_FakeCube
{
	Obj head;
	int16_t spawn_cnt;
	int16_t anim_cnt;
	int16_t anim_frame;

	int16_t tile_state;
	int16_t early_frame_cnt;

	uint16_t tile_vram_addr;  // Position of top-left tile on Plane A.

	int8_t id;
} O_FakeCube;

void o_load_fakecube(Obj *o, uint16_t data);

// Returns 1 if a cube was dropped.
int16_t fakecube_drop_cube(O_FakeCube *e, CubeType type);

#endif  // OBJ_FAKECUBE_H
