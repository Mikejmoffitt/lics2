#include "obj/metagrub.h"
#include "system.h"
#include "obj/map.h"
#include "lyle.h"
#include "obj/title.h"

#include "gfx.h"
#include "md/megadrive.h"

#include "palscale.h"

static fix16_t klunge_strength[4];
static fix16_t kdecel;
static int16_t klunge_time;

static bool s_constants_set;

static int16_t s_disable;

static inline void set_constants(void)
{
	if (s_constants_set) return;

	klunge_time = PALSCALE_DURATION(24);
	klunge_strength[0] = INTTOFIX16(PALSCALE_1ST(1.417));
	klunge_strength[1] = INTTOFIX16(PALSCALE_1ST(1.937));
	klunge_strength[2] = INTTOFIX16(PALSCALE_1ST(2.117));
	klunge_strength[3] = INTTOFIX16(PALSCALE_1ST(2.3));
	kdecel = INTTOFIX16(PALSCALE_2ND(0.096));

	s_constants_set = true;
}

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_METAGRUB);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static inline void metagrub_draw(Obj *o)
{
	int16_t x_offset, y_offset;
	uint16_t tile = s_vram_pos;
	const uint8_t dir = o->direction == OBJ_DIRECTION_LEFT;
	const uint8_t pal = LYLE_PAL_LINE;
	uint8_t size;

	if (o->dx != 0)
	{
		x_offset = -13;
		y_offset = -5;
		tile += 4;
		size = SPR_SIZE(3, 1);
	}
	else
	{
		x_offset = -9;
		y_offset = -8;
		size = SPR_SIZE(2, 2);
	}

	int16_t sp_x, sp_y;
	obj_render_setup(o, &sp_x, &sp_y, x_offset, y_offset,
	                 map_get_x_scroll(), map_get_y_scroll());

	md_spr_put(sp_x, sp_y, SPR_ATTR(tile, dir, 0, pal, 0), size);
}

static void metagrub_main(Obj *o)
{
	O_Metagrub *e = (O_Metagrub *)o;

	if (s_disable) return;
	const fix32_t lyle_x = lyle_get_x();

	if (o->hurt_stun > 0)
	{
		metagrub_draw(o);
		return;
	}

	if (o->dx == 0 && e->move_cnt > 0)
	{
		o->direction = (e->head.x < lyle_x) ?
		               OBJ_DIRECTION_RIGHT :
		               OBJ_DIRECTION_LEFT;
		e->move_cnt--;
	}
	else if (e->move_cnt == 0)
	{
		o->dx = klunge_strength[system_rand() % ARRAYSIZE(klunge_strength)];
		if (o->direction == OBJ_DIRECTION_LEFT) o->dx = -o->dx;

		e->move_cnt = klunge_time;
	}

	obj_standard_physics(o);
	if (o->dx >= kdecel) o->dx -= kdecel;
	else if (o->dx <= -kdecel) o->dx += kdecel;
	else o->dx = 0;

	if (o->dx > 0 &&
	    map_collision(FIX32TOINT(o->x) + 11, FIX32TOINT(o->y) - 5))
	{
		o->dx = -o->dx;
		o->direction = OBJ_DIRECTION_LEFT;
	}
	else if (o->dx < 0 &&
	         map_collision(FIX32TOINT(o->x) - 11, FIX32TOINT(o->y) - 5))
	{
		o->dx = -o->dx;
		o->direction = OBJ_DIRECTION_RIGHT;
	}

	metagrub_draw(o);
}

void o_load_metagrub(Obj *o, uint16_t data)
{
	(void)data;
	SYSTEM_ASSERT(sizeof(O_Metagrub) <= sizeof(ObjSlot));
	vram_load();

	obj_basic_init(o, "MetaGrub", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-7), INTTOFIX16(7), INTTOFIX16(-7), 1);

	set_constants();

	o->main_func = metagrub_main;
}

void o_unload_metagrub(void)
{
	s_vram_pos = 0;
}

void metagrub_set_enable(int16_t enable)
{
	s_disable = !enable;
}
