#ifndef OBJ_CUBE_MANAGER_H
#define OBJ_CUBE_MANAGER_H

#include "cube.h"
#include "obj.h"

#include <stdlib.h>
#include "md/megadrive.h"
#include "game.h"

#include "map.h"

#include "progress.h"

#define CUBE_COUNT_MAX 32

typedef struct O_CubeManager
{
	Obj head;
} O_CubeManager;

extern Cube g_cubes[CUBE_COUNT_MAX];
extern uint16_t g_cube_vram_pos;
extern uint8_t g_cube_phantom_anim_frame;

void o_load_cube_manager(Obj *o, uint16_t data);
void o_unload_cube_manager(void);

// Cube drawing is done here, because the cube manager owns the cube VRAM.
static inline void cube_manager_draw_cube(int16_t x, int16_t y, CubeType type)
{
	x -= map_get_x_scroll();
	y -= map_get_y_scroll();
	switch(type)
	{
		case CUBE_TYPE_GREENBLUE:
		case CUBE_TYPE_BLUE:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos, 0, 0, BG_PAL_LINE, 0),
			        SPR_SIZE(2, 2));
			break;
		case CUBE_TYPE_RED:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 8, 0, 0, LYLE_PAL_LINE, 0),
			        SPR_SIZE(2, 2));
			break;

		case CUBE_TYPE_GREEN:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 12, 0, 0, LYLE_PAL_LINE, 0),
			        SPR_SIZE(2, 2));
			break;
		case CUBE_TYPE_PHANTOM:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 16 +
			                       (4 * g_cube_phantom_anim_frame),
			        0, 0, LYLE_PAL_LINE, 0), SPR_SIZE(2, 2));
			break;
		default: // Handle all yellow variants with the default label.
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 4, 0, 0, LYLE_PAL_LINE, 0),
			        SPR_SIZE(2, 2));
			break;
		case CUBE_TYPE_ORANGE:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 52, 0, 0, LYLE_PAL_LINE, 1),
			        SPR_SIZE(4, 4));
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 32, 0, 0, BG_PAL_LINE, 1),
			        SPR_SIZE(4, 4));
			break;
		case CUBE_TYPE_SPAWNER:
			md_spr_put(x, y, SPR_ATTR(g_cube_vram_pos + 48, 0, 0, BG_PAL_LINE, 0),
			        SPR_SIZE(2, 2));
			break;
	}
}

static inline Cube *cube_manager_spawn(fix32_t x, fix32_t y, CubeType type,
                                       CubeStatus status, fix16_t dx, fix16_t dy)
{
	const ProgressSlot *prog = progress_get();
	if ((type & 0xFFF0) == 0x0840)  // CP orb
	{
		const int16_t orb_id = type & 0x000F;
		if (prog->cp_orbs & (1 << orb_id))
		{
			return NULL;
		}
	}
	else if ((type & 0xFFF0) == 0x0880)  // HP orb
	{
		const int16_t orb_id = type & 0x000F;
		if (prog->hp_orbs & (1 << orb_id))
		{
			return NULL;
		}
	}

	uint16_t i = ARRAYSIZE(g_cubes);
	while (i--)
	{
		Cube *c = &g_cubes[i];
		if (c->status != CUBE_STATUS_NULL) continue;
		c->x = x;
		c->y = y;
		c->type = type;
		c->status = status;
		c->dx = dx;
		c->dy = dy;
		c->spawned_cube = NULL;
		c->lyle_spawn_check = 0;
		if (c->type == CUBE_TYPE_ORANGE)
		{
			c->left = INTTOFIX16(-15);
			c->right = INTTOFIX16(15);
			c->top = INTTOFIX16(-32);
		}
		else
		{
			c->left = INTTOFIX16(-8);
			c->right = INTTOFIX16(8);
			c->top = INTTOFIX16(-15);
		}

		c->bounce_count = 0;
		c->collision_timeout = 0;
		c->spawn_count = 0;
		c->fizzle_count = 0;
		return c;
	}
	return NULL;
}

#endif  // OBJ_CUBE_MANAGER_H
