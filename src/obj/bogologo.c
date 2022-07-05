#include "obj/bogologo.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "sfx.h"
#include "obj/title.h"

static uint16_t s_vram_pos;

static int16_t kanim_speed;
static int16_t kappear_frame;
static int16_t ksolid_frame;
static int16_t kflicker_speed;

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;

	kanim_speed = PALSCALE_DURATION(9);
	kappear_frame = PALSCALE_DURATION(36.0);
	ksolid_frame = PALSCALE_DURATION(108.0);
	kflicker_speed = PALSCALE_DURATION(4);

	s_constants_set = 1;
}

static void render(O_Bogologo *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -64;
	static const int16_t offset_y = -48;

	obj_render_setup_simple(o, &sp_x, &sp_y, offset_x, offset_y,
	                        map_get_x_scroll(), map_get_y_scroll());

	for (int16_t x = 0; x < 4; x++)
	{
		md_spr_put(sp_x + (x * 32), sp_y,
		        SPR_ATTR(s_vram_pos + (x * 12),
		                 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(4, 3));
		md_spr_put(sp_x + (x * 32), sp_y + 24,
		        SPR_ATTR(s_vram_pos + 48 + (x * 12),
		                 0, 0, ENEMY_PAL_LINE, 0),
		        SPR_SIZE(4, 3));
	}
}

static void main_func(Obj *o)
{
	O_Bogologo *e = (O_Bogologo *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}
	if (o->offscreen)
	{
		obj_erase(o);
		return;
	}

	e->appear_cnt++;
	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);
	OBJ_SIMPLE_ANIM(e->flicker_cnt, e->flicker_frame, 2, kflicker_speed);

	if (e->appear_cnt == kappear_frame)
	{
		sfx_play(SFX_TELEPORT, 3);
		sfx_play(SFX_TELEPORT_2, 3);
		const Obj *title = obj_find_by_type(OBJ_TITLE);
		// We need the title object to live.
		if (!title) obj_erase(o);
		const Gfx *g = gfx_get(GFX_BOGOLOGO);
		s_vram_pos = gfx_load(g, title_get_vram_pos());
	}

	if (e->appear_cnt < kappear_frame ||
	    (e->appear_cnt < ksolid_frame && e->flicker_frame == 0))
	{
		return;
	}

	md_pal_upload(ENEMY_CRAM_POSITION,
	           res_pal_bogologo_bin + (e->anim_frame ? 32 : 0),
	           sizeof(res_pal_bogologo_bin) / 4);
	render(e);
}

void o_load_bogologo(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Bogologo) <= sizeof(ObjSlot));
	(void)data;
	set_constants();

	obj_basic_init(o, "BogoLogo", OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-64), INTTOFIX16(64), INTTOFIX16(-48), 1);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_bogologo(void)
{
	s_vram_pos = 0;
}
