#include "obj/dancyflower.h"

#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "palscale.h"
#include "map.h"
#include "progress.h"

#include "cube.h"

static uint16_t s_vram_pos;

static int16_t kanim_speed;

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(14);

	s_constants_set = 1;
}

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_DANCYFLOWER);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void render(O_Dancyflower *f)
{
	Obj *o = &f->head;
	int16_t sp_x, sp_y;
	uint16_t bottom_tile_offset;
	int16_t head_y_offset;
	switch (f->anim_frame)
	{
		default:
		case 0:
		case 2:
			head_y_offset = 0;
			bottom_tile_offset = 6;
			break;
		case 1:
			head_y_offset = 1;
			bottom_tile_offset = 14;
			break;
		case 3:
			head_y_offset = 1;
			bottom_tile_offset = 22;
			break;
	}
	// Bottom sprite, for the body.
	obj_render_setup(o, &sp_x, &sp_y, -8, -32,
	                 map_get_x_scroll(), map_get_y_scroll());
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + bottom_tile_offset,
	                    0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(2, 4));
	// Top sprite, for the head. It bobs up and down with head_y_offset.
	sp_x -= 4;
	sp_y -= 16;
	sp_y += head_y_offset;
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 2));
}

// Special cube function exists just to mark the "has been destroyed" flag,
// which persists in SRAM.
static void cube_func(Obj *o, Cube *c)
{
	obj_standard_cube_response(o, c);
	if (o->hp <= 0)
	{
		progress_get()->killed_dancyflower = 1;
	}
}

static void main_func(Obj *o)
{
	O_Dancyflower *f = (O_Dancyflower *)o;
	if (o->hurt_stun > 0)
	{
		render(f);
		return;
	}

	// Animate.
	f->anim_cnt++;
	if (f->anim_cnt >= kanim_speed)
	{
		f->anim_cnt = 0;
		f->anim_frame++;
		if (f->anim_frame > 3) f->anim_frame = 0;
	}

	render(f);
}

void o_load_dancyflower(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Dancyflower) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	if (progress_get()->killed_dancyflower)
	{
		obj_erase(o);
		return;
	}
	vram_load();
	set_constants();

	obj_basic_init(o, "DncyFlwr", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE | OBJ_FLAG_BOUNCE_L,
	               INTTOFIX16(-8), INTTOFIX16(8), INTTOFIX16(-47), 2);
	o->main_func = main_func;
	o->cube_func = cube_func;
	
}

void o_unload_dancyflower(void)
{
	s_vram_pos = 0;
}
