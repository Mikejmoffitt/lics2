#include "obj/secrettv.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "palscale.h"
#include "map.h"
#include "progress.h"


static const SecretTvFrame kanim_athf[] =
{
	{0, 0}, {1, 0}
};

static const SecretTvFrame kanim_bla[] =
{
	{2, 0}, {3, 0}
};

static const SecretTvFrame kanim_bogo[] =
{
	{4, 0}, {5, 0},  {6, 0}, {7, 0},
	{8, 0}, {9, 0}, {10, 0}, {11, 0},
	{12, 0}
};

static const SecretTvFrame kanim_boom[] =
{
	{13, 0}
};

static const SecretTvFrame kanim_chickegg[] =
{
	{14, 0}, {15, 0}
};

static const SecretTvFrame kanim_freecow[] =
{
	{16, 0}, {17, 0}
};

static const SecretTvFrame kanim_deathlife[] =
{
	{18, 0}, {19, 0}
};

static const SecretTvFrame kanim_devil[] =
{
	{20, 0}, {21, 0}
};

static const SecretTvFrame kanim_duck[] =
{
	{22, 0}, {23, 0}
};

static const SecretTvFrame kanim_fiftytwo[] =
{
	{24, 0}, {25, 0}
};

static const SecretTvFrame kanim_girl[] =
{
	{26, 0}, {27, 0}, {28, 0}
};

static const SecretTvFrame kanim_hyakugojyuuichi[] =
{
	{29, 0}, {30, 0}, {31, 0},
	{32, 0}, {33, 0}, {34, 0}, {35, 0}
};

static const SecretTvFrame kanim_fox[] =
{
	{36, 0}, {37, 0}
};

static const SecretTvFrame kanim_face[] =
{
	{38, 0}, {39, 0}
};

static const SecretTvFrame kanim_school[] =
{
	{40, 0}, {41, 0}, {42, 0}, {43, 0}
};

static const SecretTvFrame kanim_dog1[] =
{
	{44, 1}, {44, 2}
};

static const SecretTvFrame kanim_dog2[] =
{
	{45, 1}, {45, 2}
};

static const SecretTvFrame kanim_ring[] =
{
	{46, 1}, {47, 1},
	{48, 1}, {49, 1}, {50, 1}, {51, 1},
	{52, 1}, {53, 1}
};

static const SecretTvFrame kanim_munk[] =
{
	{54, 3}, {55, 3},
};

static const SecretTvFrame kanim_petoo1[] =
{
	{56, 3}, {57, 3}, {58, 3}, {59, 3},
	{60, 3}, {61, 3}, {62, 3}
};

static const SecretTvFrame kanim_petoo2[] =
{
	{63, 3}, {64, 3}, {65, 3}, {66, 3},
	{67, 3}
};

static const SecretTvFrame kanim_lyle[] =
{
	{68, 4}, {69, 4},
};

static const SecretTvFrame kanim_news[] =
{
	{70, 5}, {71, 5},
};

static const SecretTvFrame kanim_mario[] =
{
	{72, 6}, {73, 6}, {74, 6}, {75, 6},
	{76, 6}
};

static const SecretTvFrame kanim_warp1[] =
{
	{77, 7}, {77, 8}
};

static const SecretTvFrame kanim_warp2[] =
{
	{78, 7}, {78, 8}
};

static const SecretTvFrame kanim_warp3[] =
{
	{79, 7}, {79, 8}
};

static const SecretTvFrame kanim_static[] =
{
	{80, 10}, {81, 10}, {82, 10},
};

#define STVANDEF(anim, delay, loop) \
{anim, ARRAYSIZE(anim), loop, delay}

static const SecretTvAnim kanims[] =
{
	STVANDEF(kanim_static, 2, 0),
	STVANDEF(kanim_athf, 8, 0),
	STVANDEF(kanim_bla, 8, 0),
	STVANDEF(kanim_bogo, 8, 0),
	STVANDEF(kanim_boom, 8, 0),
	STVANDEF(kanim_chickegg, 12, 0),
	STVANDEF(kanim_freecow, 12, 0),
	STVANDEF(kanim_deathlife, 12, 0),
	STVANDEF(kanim_devil, 13, 0),
	STVANDEF(kanim_duck, 13, 0),
	STVANDEF(kanim_fiftytwo, 9, 0),
	STVANDEF(kanim_girl, 18, 0),
	STVANDEF(kanim_hyakugojyuuichi, 10, 0),
	STVANDEF(kanim_fox, 6, 0),
	STVANDEF(kanim_face, 12, 0),
	STVANDEF(kanim_school, 13, 0),
	STVANDEF(kanim_dog1, 2, 0),
	STVANDEF(kanim_dog2, 2, 0),
	STVANDEF(kanim_ring, 20, 6),
	STVANDEF(kanim_munk, 6, 0),
	STVANDEF(kanim_petoo1, 15, 0),
	STVANDEF(kanim_petoo2, 15, 0),
	STVANDEF(kanim_lyle, 15, 0),
	STVANDEF(kanim_news, 16, 0),
	STVANDEF(kanim_mario, 3, 0),
	STVANDEF(kanim_warp1, 2, 0),
	STVANDEF(kanim_warp2, 2, 0),
	STVANDEF(kanim_warp3, 2, 0),
};

#undef STVANDEF

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	s_vram_pos = obj_vram_alloc(32 * 3 * 2);
}

static void render(O_SecretTv *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -12;
	static const int16_t offset_y = -17;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	// The screen
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos/32, 0, 0, ENEMY_PAL_LINE, 0), SPR_SIZE(3, 2));

	// The TV frame itself
	sp_x -= 8;
	sp_y -= 7;
	for (uint16_t i = 0; i < 4; i++)
	{
		md_spr_put(sp_x, sp_y, SPR_ATTR(0x28 + (0x10 * i), 0, 0, MAP_PAL_LINE, 0), SPR_SIZE(4, 1));
		sp_y += 8;
	}
}

static void set_anim(O_SecretTv *e, uint16_t id)
{
	if (id >= ARRAYSIZE(kanims)) id = 0;
	e->anim = &kanims[id];
	e->anim_frame = 0;
	e->anim_cnt = 0;
	e->anim_delay = PALSCALE_DURATION((e->anim->delay));
}

static void animate(O_SecretTv *e)
{
	if (!e->anim) return;

	e->anim_cnt++;
	if (e->anim_cnt >= e->anim_delay)
	{
		e->anim_cnt = 0;
		e->anim_frame++;
		if (e->anim_frame >= e->anim->len)
		{
			e->anim_frame = e->anim->loop;
		}
	}

	const SecretTvFrame *frame = &e->anim->frames[e->anim_frame];

	// Set the palette
	const uint16_t *kpal = (const uint16_t *)res_pal_secrettv_bin;
	md_pal_upload(ENEMY_CRAM_POSITION, &kpal[frame->pal * 0x10], 0x10);

	// DMA in the tile data
	const Gfx *g = gfx_get(GFX_EX_SECRETTV);
	md_dma_transfer_vram(s_vram_pos, &g->data[frame->code * 3 * 2 * 32], (32 * 3 * 2) / 2, 2);
}

static void main_func(Obj *o)
{
	O_SecretTv *e = (O_SecretTv *)o;

	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	animate(e);
	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	O_SecretTv *e = (O_SecretTv *)o;
	obj_standard_cube_response(o, c);
	set_anim(e, system_rand() % ARRAYSIZE(kanims));
	if (o->hp <= 0)
	{
		o->hp = 127;
	}

	const bool explode = (system_rand() % 64) == 0;
	o->hp = explode ? 0 : 127;
}

void o_load_secrettv(Obj *o, uint16_t data)
{
	O_SecretTv *e = (O_SecretTv *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	vram_load();

	obj_basic_init(o, "SecretTv", OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-12), INTTOFIX16(12), INTTOFIX16(-16), 127);
	o->y -= INTTOFIX32(4);
	o->x += INTTOFIX32(4);
	o->main_func = main_func;
	o->cube_func = cube_func;

	set_anim(e, 0);
}

void o_unload_secrettv(void)
{
	s_vram_pos = 0;
}
