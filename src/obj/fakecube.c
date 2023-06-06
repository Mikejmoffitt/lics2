#include "obj/fakecube.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "game.h"
#include "obj/cube_manager.h"
#include "lyle.h"

static uint16_t kspawn_seq[2];
static int16_t kspawn_anim_speed;
static fix16_t kceiling_dy;  // Same as lyle.c

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kspawn_seq[0] = PALSCALE_DURATION(180);  // Invisible up to this point.
	kspawn_seq[1] = kspawn_seq[0] + PALSCALE_DURATION(60);  // Flashing.
	kspawn_anim_speed = PALSCALE_DURATION(4);
	kceiling_dy = INTTOFIX16(PALSCALE_1ST(-0.416666667));

	s_constants_set = true;
}

static void draw_to_bg(O_FakeCube *e)
{
	if (e->tile_state)
	{
		const uint16_t attr = VDP_ATTR(g_cube_vram_pos,
		                               0, 0, BG_PAL_LINE, 0);
		md_vdp_poke(e->tile_vram_addr, attr);
		md_vdp_poke(e->tile_vram_addr + 2, attr + 2);
		md_vdp_poke(e->tile_vram_addr + (2 * GAME_PLANE_W_CELLS), attr + 1);
		md_vdp_poke(e->tile_vram_addr + 2 + (2 * GAME_PLANE_W_CELLS), attr + 3);
	}
	else
	{
		md_vdp_poke(e->tile_vram_addr, 0);
		md_vdp_poke(e->tile_vram_addr + 2, 0);
		md_vdp_poke(e->tile_vram_addr + (2 * GAME_PLANE_W_CELLS), 0);
		md_vdp_poke(e->tile_vram_addr + 2 + (2 * GAME_PLANE_W_CELLS), 0);
	}
}

static void main_func(Obj *o)
{
	O_FakeCube *e = (O_FakeCube *)o;

	int16_t new_tile_state = 1;

	if (e->spawn_cnt > 0)
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kspawn_anim_speed);
		e->spawn_cnt++;
		if ((e->spawn_cnt > 2 && e->spawn_cnt < kspawn_seq[0]) ||
		    e->anim_frame == 1)
		{
			new_tile_state = 0;
		}
		if (e->spawn_cnt >= kspawn_seq[1])
		{
			e->spawn_cnt = 0;
		}
	}

	if (new_tile_state != e->tile_state)
	{
		e->tile_state = new_tile_state;
		draw_to_bg(e);
	}

	if (e->early_frame_cnt < 3)
	{
		e->early_frame_cnt++;
		draw_to_bg(e);
	}

	// Don't let Lyle go above the fake cube.
	O_Lyle *l = lyle_get();
	if (l->head.dy < kceiling_dy && l->head.y + l->head.top < o->y)
	{
		l->head.y = o->y - l->head.top;
		l->head.dy = kceiling_dy;
	}
}

void o_load_fakecube(Obj *o, uint16_t data)
{
	O_FakeCube *e = (O_FakeCube *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();

	obj_basic_init(o, "FakeCube", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-16), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	// Determine location on Plane A. Scroll is not taken into account as the
	// boss arena is static.
	const int16_t tile_x = FIX32TOINT(o->x + o->left) / 8;
	const int16_t tile_y = FIX32TOINT(o->y + o->top) / 8;
	e->tile_vram_addr = md_vdp_get_plane_base(VDP_PLANE_A) +
	                    2 * ((tile_y * GAME_PLANE_W_CELLS) +
	                    tile_x);
	e->id = data & 0x7F;
}

int16_t fakecube_drop_cube(O_FakeCube *e, CubeType type)
{
	if (e->spawn_cnt != 0) return 0;
	e->spawn_cnt = 1;
	Cube *c = cube_manager_spawn(e->head.x, e->head.y, type, CUBE_STATUS_AIR, 0, 0);
	if (c)
	{
		// Hack so adjacent cubes don't fizzle each other.
		c->right = INTTOFIX16(7);
	}

	return (c != NULL);
}
