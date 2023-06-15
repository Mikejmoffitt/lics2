#ifndef CUBE_MANAGER_H
#define CUBE_MANAGER_H

#include "cube.h"
#include "obj.h"

#include <stdlib.h>
#include <string.h>

#include "md/megadrive.h"
#include "game.h"

#include "map.h"

#include "progress.h"

#define CUBE_COUNT_MAX 32

#define CUBE_VRAM_TILE (CUBE_VRAM_POSITION / 32)

extern Cube g_cubes[CUBE_COUNT_MAX];
extern uint8_t g_cube_phantom_anim_frame;

extern SprParam g_cube_spr;

void cube_manager_set_hibernate(bool hibernate);

void cube_manager_init(void);
void cube_manager_poll(void);

// Cube drawing is done here, because the cube manager owns the cube VRAM.
static inline void cube_manager_draw_cube(int16_t x, int16_t y, CubeType type)
{
	g_cube_spr.x = x - map_get_x_scroll();
	g_cube_spr.y = y - map_get_y_scroll();
	switch(type)
	{
		case CUBE_TYPE_GREENBLUE:
		case CUBE_TYPE_BLUE:
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE, 0, 0, BG_PAL_LINE, 0);
			break;
		case CUBE_TYPE_RED:
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 8, 0, 0, LYLE_PAL_LINE, 0);
			break;

		case CUBE_TYPE_GREEN:
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 12, 0, 0, LYLE_PAL_LINE, 0);
			break;
		case CUBE_TYPE_PHANTOM:
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 16 +
			                           (4 * g_cube_phantom_anim_frame),
			        0, 0, LYLE_PAL_LINE, 0);
			break;
		default: // Handle all yellow variants with the default label.
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 4, 0, 0, LYLE_PAL_LINE, 0);
			break;
		case CUBE_TYPE_ORANGE:
			g_cube_spr.size = SPR_SIZE(4, 4);
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 52, 0, 0, LYLE_PAL_LINE, 0);
			md_spr_put_st(&g_cube_spr);
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 32, 0, 0, BG_PAL_LINE, 0);
			md_spr_put_st(&g_cube_spr);
			g_cube_spr.size = SPR_SIZE(2, 2);
			return;

		case CUBE_TYPE_SPAWNER:
			g_cube_spr.attr = SPR_ATTR(CUBE_VRAM_TILE + 48, 0, 0, BG_PAL_LINE, 0);
			break;
	}
	md_spr_put_st(&g_cube_spr);
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
		memset(c, 0, sizeof(*c));
		c->x = x;
		c->y = y;
		c->type = type;
		c->status = status;
		c->dx = dx;
		c->dy = dy;
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

		return c;
	}
	return NULL;
}

#endif  // CUBE_MANAGER_H
