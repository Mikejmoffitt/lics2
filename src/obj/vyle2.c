#include "obj/vyle2.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"
#include "common.h"

// Vyle 2's sprite is huge (64x64px), so he's animated like Lyle, through DMA
// transfers, instead of loading everything at once.
#define VYLE2_VRAM_SIZE (8 * 8 * 32)

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	s_vram_pos = obj_vram_alloc(VYLE2_VRAM_SIZE) / 32;
}

// Store static constants here.

static inline void set_constants(void)
{
	static int16_t s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	s_constants_set = 1;
}

static void render(O_Vyle2 *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;

	typedef struct SprDef
	{
		int16_t x;
		int16_t y;
		uint16_t tile;
		int16_t size;
	} SprDef;

	// Vyle 2 uses between 1 and 4 sprites to draw an animation frame.
	// As mentioned above, a full 64 x 64 bitmap is large, and fitting all of
	// his frame data into VRAM at once is challenging.

// Macro to define a simple 64 x 64 metasprite out of four 32 x 32 sprites..
#define FULLFRAME(n, base) \
		{-32, -64, (n * 64) + base, SPR_SIZE(4, 4)}, \
		{0,   -64, (n * 64) + base + 16, SPR_SIZE(4, 4)}, \
		{-32, -32, (n * 64) + base + 32, SPR_SIZE(4, 4)}, \
		{0,   -32, (n * 64) + base + 48, SPR_SIZE(4, 4)}

	// Four sprites per frame.
	static const SprDef frames[] =
	{
		// 00 - stand w/ gun
		{-12, -23, 0, SPR_SIZE(3, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 01 - walk w/ gun 1
		{-12, -24, 9, SPR_SIZE(3, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 02 - walk w/ gun 2
		{-12, -24, 18, SPR_SIZE(3, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 03 - walk w/ gun 3
		{-12, -24, 27, SPR_SIZE(3, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 04 - push
		{-23, -24, 36, SPR_SIZE(4, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 05 - stand forward (small)
		{-8, -24, 48, SPR_SIZE(2, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 05 - stand forward (med)
		{-12, -32, 54, SPR_SIZE(3, 4)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 06 - stand forward (large)
		{-16, -48, 66, SPR_SIZE(4, 3)},
		{-16, -24, 78, SPR_SIZE(4, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1},

		// 07 - stand forward (full)
		FULLFRAME(0, 96),
		// 08 - charge 1
		FULLFRAME(1, 96),
		// 09 - charge 2
		FULLFRAME(2, 96),
		// 10 - charge 3
		FULLFRAME(3, 96),
		// 11 - charge 4
		FULLFRAME(4, 96),
		// 12 - charge 5
		FULLFRAME(5, 96),
		// 13 - charge 6
		FULLFRAME(6, 96),
		// 14 - stand
		FULLFRAME(7, 96),
		// 15 - mouth open
		FULLFRAME(8, 96),
		// 16 - walk 1
		FULLFRAME(9, 96),
		// 17 - walk 2
		FULLFRAME(10, 96),
		// 18 - walk 3
		FULLFRAME(11, 96),
		// 19 - stance
		FULLFRAME(12, 96),
		// 20 - jump prep 1
		FULLFRAME(13, 96),
		// 21 - jump prep 2
		FULLFRAME(14, 96),
		// 22 - stand forward alt
		FULLFRAME(15, 96),
		// 23 - jump
		FULLFRAME(16, 96),
		// 24 - hurt 1
		FULLFRAME(17, 96),
		// 25 - hurt 2
		FULLFRAME(18, 96),
		// 26 - falling
		FULLFRAME(19, 96),
		// 27 - dead
		FULLFRAME(20, 96),
	};
#undef FULLFRAME

	static const int16_t bytes_for_sprite_size[] =
	{
		[SPR_SIZE(1, 1)] = 32,
		[SPR_SIZE(1, 2)] = 64,
		[SPR_SIZE(1, 3)] = 96,
		[SPR_SIZE(1, 4)] = 128,
		[SPR_SIZE(2, 1)] = 64,
		[SPR_SIZE(2, 2)] = 128,
		[SPR_SIZE(2, 3)] = 192,
		[SPR_SIZE(2, 4)] = 256,
		[SPR_SIZE(3, 1)] = 96,
		[SPR_SIZE(3, 2)] = 192,
		[SPR_SIZE(3, 3)] = 288,
		[SPR_SIZE(3, 4)] = 384,
		[SPR_SIZE(4, 1)] = 128,
		[SPR_SIZE(4, 2)] = 256,
		[SPR_SIZE(4, 3)] = 384,
		[SPR_SIZE(4, 4)] = 512,
	};

	obj_render_setup(o, &sp_x, &sp_y, 0, 0,
	                 map_get_x_scroll(), map_get_y_scroll());

	const int16_t flip = (o->direction == OBJ_DIRECTION_RIGHT);
	const SprDef *frame = &frames[e->metaframe * 4];

	// Sprite attribute base points to the start of VRAM allocated for Vyle2.
	const uint16_t attr = SPR_ATTR(s_vram_pos, flip, 0, ENEMY_PAL_LINE, 0);
	uint16_t spr_tile = 0;

	for (uint16_t i = 0; i < 4; i++)
	{
		if (frame->size == -1) continue;  // Unused sprite.

		// X offset must be flipped about the Y axis, and then offset by the
		// sprite size as their origin is the top-left.
		const uint8_t spr_w = (((frame->size >> 2) & 0x03) + 1) * 8;
		const int16_t offs_x = (flip) ?
		                       (-frame->x - spr_w) :
		                       frame->x;

		// Take a sprite slot for this asset.
		spr_put(sp_x + offs_x, sp_y + frame->y,
		        attr + spr_tile, frame->size);

		// Slot the graphics assets into VRAM.
		const uint16_t transfer_bytes = bytes_for_sprite_size[frame->size];
		gfx_load_ex(gfx_get(GFX_VYLE2), frame->tile * 32,
		            transfer_bytes, (s_vram_pos + spr_tile) * 32);

		spr_tile += transfer_bytes / 32;
		frame++;
	}
}

static void main_func(Obj *o)
{
	O_Vyle2 *e = (O_Vyle2 *)o;
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	e->anim_cnt++;
	if (e->anim_cnt == 30)
	{
		o->direction = (o->direction == OBJ_DIRECTION_RIGHT) ? OBJ_DIRECTION_LEFT : OBJ_DIRECTION_RIGHT;
	}
	if (e->anim_cnt > 60)
	{
		e->anim_cnt = 0;
		e->metaframe++;
	}
	render(e);
}

void o_load_vyle2(Obj *o, uint16_t data)
{
	SYSTEM_ASSERT(sizeof(O_Vyle2) <= sizeof(ObjSlot));
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE,
	               INTTOFIX16(-32), INTTOFIX16(32), INTTOFIX16(-64), 5);
	o->left = INTTOFIX16(-18);
	o->right = INTTOFIX16(18);
	o->top = INTTOFIX16(-48);
	o->main_func = main_func;
	o->cube_func = NULL;
}

void o_unload_vyle2(void)
{
	s_vram_pos = 0;
}
