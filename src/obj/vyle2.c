#include "obj/vyle2.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "obj/map.h"

#include "lyle.h"
#include "sfx.h"
#include "music.h"
#include "obj/laser.h"

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
static fix16_t klyle_walk_dx;
static fix16_t klyle_run_dx;
static fix16_t kscroll_pan_dx;
static int16_t kkeddums_meow_delay_frames;
static int16_t kpan_start_delay_frames;

static int16_t kvyle_activation_delay_frames;
static int16_t kvyle_activation_duration_frames;

static int16_t kvyle_grow_frames;
static int16_t kvyle_grow_post_frames;
static int16_t kvyle_grow_anim_speed;

static int16_t kvyle_big_walk_anim_speed;
static int16_t kstart_delay;

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	klyle_walk_dx = INTTOFIX16(PALSCALE_1ST(0.9));  // TODO: This is made up
	klyle_run_dx = INTTOFIX16(PALSCALE_1ST(1.41666666667));
	kscroll_pan_dx = INTTOFIX16(PALSCALE_1ST(4));  // "
	kkeddums_meow_delay_frames = PALSCALE_DURATION(120);
	kpan_start_delay_frames = PALSCALE_DURATION(50);
	kvyle_activation_delay_frames = PALSCALE_DURATION(50);
	kvyle_activation_duration_frames = PALSCALE_DURATION(30);
	kvyle_grow_frames = PALSCALE_DURATION(60);
	kvyle_grow_post_frames = PALSCALE_DURATION(120);
	kvyle_grow_anim_speed = PALSCALE_DURATION(2);
	kvyle_big_walk_anim_speed = PALSCALE_DURATION(7);
	kstart_delay = PALSCALE_DURATION(90);

	s_constants_set = true;
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
	// So, we need to transfer what's needed for the current frame into VRAM.

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
		{-12, -24, 0, SPR_SIZE(3, 3)},
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
		md_spr_put(sp_x + offs_x, sp_y + frame->y,
		        attr + spr_tile, frame->size);

		// Slot the graphics assets into VRAM.
		const uint16_t transfer_bytes = bytes_for_sprite_size[frame->size];
		gfx_load_ex(gfx_get(GFX_VYLE2), frame->tile * 32,
		            transfer_bytes, (s_vram_pos + spr_tile) * 32);

		spr_tile += transfer_bytes / 32;
		frame++;
	}
}

static inline void do_lyle_shake_anim(O_Vyle2 *e, O_Lyle *l)
{
	const int8_t last_frame = e->lyle_anim_frame;
	OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 2, PALSCALE_DURATION(2));
	lyle_set_anim_frame(24);
	// The shaking
	if (e->lyle_anim_frame != last_frame)
	{
		l->head.x += (e->lyle_anim_frame == 0) ? INTTOFIX16(1) : INTTOFIX16(-1);
	}
}

static inline void do_lyle_walk_anim(O_Vyle2 *e)
{
	const int16_t lyle_walk_cycle[] = { 25, 26, 27, 26 };
	const int8_t last_frame = e->lyle_anim_frame;
	OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame,
	                4, PALSCALE_DURATION(6.8));
	lyle_set_anim_frame(lyle_walk_cycle[e->lyle_anim_frame]);
	// walk sounds.
	if (e->lyle_anim_frame != last_frame)
	{
		if (e->lyle_anim_frame == 0) sfx_play(SFX_WALK1, 8);
		else if (e->lyle_anim_frame == 2) sfx_play(SFX_WALK2, 8);
	}
}

static void main_func(Obj *o)
{
	O_Vyle2 *e = (O_Vyle2 *)o;
	O_Lyle *l = lyle_get();
	if (o->hurt_stun > 0)
	{
		render(e);
		return;
	}

	const Vyle2State state_prev = e->state;

	const int16_t vyle_walk_cycle[] = { 2, 1, 2, 3 };
	const int16_t vyle_big_walk_cycle[] = { 9, 10, 11, 12, 13, 14};

	switch (e->state)
	{
		default:
			break;
		case VYLE2_STATE_LYLE_WALK_ANGRY:
			if (e->state_elapsed == 0)
			{
				o->direction = OBJ_DIRECTION_LEFT;
				e->metaframe = 0;
				lyle_set_control_en(0);
				lyle_set_master_en(0);
				lyle_set_scroll_h_en(0);
				lyle_set_scroll_v_en(0);
				l->head.dx = klyle_walk_dx;
				l->head.dy = 0;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				map_set_x_scroll(0);
			}

			// Lyle begins by approaching until he reaches a certain point.
			do_lyle_walk_anim(e);

			// Once lyle has walked far enough, proceed.
			if (l->head.x >= INTTOFIX32(92)) e->state = VYLE2_STATE_CAMERA_PAN_TO_MACHINE;
			obj_accurate_physics(&l->head);

			break;
		case VYLE2_STATE_CAMERA_PAN_TO_MACHINE:
			// Lyle is halted, and the screen pans to show Vyle and his machine.
			if (e->state_elapsed == 0)
			{
				l->head.dx = 0;
				sfx_play(SFX_BOINGO_JUMP, 8);
			}

			do_lyle_shake_anim(e, l);

			// Begin panning after a short pause.
			if (e->state_elapsed > kpan_start_delay_frames) e->xscroll += kscroll_pan_dx;

			// Once we hit the right edge, proceed.
			if (e->xscroll >= map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS))
			{
				e->xscroll = map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS);
				e->state = VYLE2_STATE_KEDDUMS_MEOW;
			}

			map_set_x_scroll(FIX32TOINT(e->xscroll));
			break;

		case VYLE2_STATE_KEDDUMS_MEOW:
			// After a short time, keddums meows, and the screen moves back.
			if (e->state_elapsed > kkeddums_meow_delay_frames)
			{
				e->state = VYLE2_STATE_CAMERA_PAN_TO_LYLE;
			}
			// TODO: actual meow
			break;

		case VYLE2_STATE_CAMERA_PAN_TO_LYLE:
			// Camera pans back to Lyle.
			e->xscroll -= kscroll_pan_dx;
			if (e->xscroll < 0)
			{
				e->xscroll = 0;
				e->state = VYLE2_STATE_LYLE_APPROACH_MACHINE;
			}
			map_set_x_scroll(FIX32TOINT(e->xscroll));
			break;

		case VYLE2_STATE_LYLE_APPROACH_MACHINE:
			// Lyle now walks forwards, bringing the camera with him.
			lyle_set_scroll_h_en(1);
			do_lyle_walk_anim(e);
			l->head.dx = klyle_run_dx;

			// once lyle gets far enough, Vyle should start to go towards the machine
			if (l->head.x >= INTTOFIX32(320 + 56))
			{
				e->head.direction = OBJ_DIRECTION_RIGHT;
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, PALSCALE_DURATION(6.8));
				e->head.dx = klyle_run_dx;
				e->metaframe = vyle_walk_cycle[e->anim_frame];
			}

			obj_accurate_physics(&l->head);

			{
				int16_t px = FIX32TOINT(l->head.x);
				const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
				px -= left_bound;
				map_set_x_scroll(px);
			}

			// Once lyle has gone far enough (and Vyle has reached the machine) proceed.
			if (l->head.x >= map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS + 52))
			{
				e->state = VYLE2_STATE_VYLE_ACTIVATE_MACHINE;
			}
			break;

		case VYLE2_STATE_VYLE_ACTIVATE_MACHINE:
			if (e->state_elapsed == 0)
			{
				// Halt both vyle and lyle.
				e->head.dx = 0;
				e->metaframe = 0;
				l->head.dx = 0;
				lyle_set_anim_frame(24);
				// Turn on the lasers.
				// TODO: laser sfx
				laser_set_mode(LASER_MODE_ON);

				int16_t px = FIX32TOINT(l->head.x);
				const int16_t left_bound = GAME_SCREEN_W_PIXELS / 2;
				px -= left_bound;
				e->xscroll = INTTOFIX32(px);
			}

			if (e->xscroll < map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS + 128))
			{
				e->xscroll += kscroll_pan_dx;
			}

			do_lyle_shake_anim(e, l);

			// Vyle pushes the button.
			if (e->state_elapsed == kvyle_activation_delay_frames)
			{
				// TODO: keddums meow
				// TODO: activate big orb thing
				// TODO: make keddums begin shaking
				// TODO: Illuminate kitty powered psychowave
				e->metaframe = 4;
			}
			else if (e->state_elapsed == (kvyle_activation_delay_frames +
			                              kvyle_activation_duration_frames))
			{
				// Done pushing the button.
				e->metaframe = 0;
			}
			else if (e->state_elapsed >= (kvyle_activation_delay_frames +
			                              (kvyle_activation_duration_frames * 2)))
			{
				// Vyle begins to walk left.
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame,
				                4, PALSCALE_DURATION(6.8));
				e->head.direction = OBJ_DIRECTION_LEFT;
				e->head.dx = -klyle_run_dx;
				e->metaframe = vyle_walk_cycle[e->anim_frame];

				if (e->head.x < map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS) + INTTOFIX32(40))
				{
					e->state = VYLE2_STATE_VYLE_GROW_1;
					e->head.x = map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS) + INTTOFIX32(40);
				}
			}
			map_set_x_scroll(FIX32TOINT(e->xscroll));
			break;

		case VYLE2_STATE_VYLE_GROW_1:
			o->dx = 0;
			if (e->state_elapsed == 0)
			{
				e->metaframe = 5;
			}

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_VYLE_GROW_2;
			}

			do_lyle_shake_anim(e, l);
			break;

		case VYLE2_STATE_VYLE_GROW_2:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_TELEPORT, 3);
				sfx_play(SFX_TELEPORT_2, 3);
			}
			if (e->state_elapsed < kvyle_grow_frames)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_grow_anim_speed);
				e->metaframe = e->anim_frame ? 5 : 6;
			}
			else
			{
				e->metaframe = 6;
			}

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_VYLE_GROW_3;
			}

			do_lyle_shake_anim(e, l);
			break;

		case VYLE2_STATE_VYLE_GROW_3:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_TELEPORT, 3);
				sfx_play(SFX_TELEPORT_2, 3);
			}
			if (e->state_elapsed < kvyle_grow_frames)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_grow_anim_speed);
				e->metaframe = e->anim_frame ? 6 : 7;
			}
			else
			{
				e->metaframe = 7;
			}

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_VYLE_GROW_4;
			}

			do_lyle_shake_anim(e, l);
			break;

		case VYLE2_STATE_VYLE_GROW_4:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_TELEPORT, 3);
				sfx_play(SFX_TELEPORT_2, 3);
			}
			if (e->state_elapsed < kvyle_grow_frames)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_grow_anim_speed);
				e->metaframe = e->anim_frame ? 7 : 8;
			}
			else
			{
				e->metaframe = 8;
			}

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_ENTER_ARENA;
			}

			do_lyle_shake_anim(e, l);
			break;
		case VYLE2_STATE_ENTER_ARENA:
			// The camera pans back until it reaches the arena.
			if (e->xscroll > INTTOFIX32(320))
			{
				// TODO: laser sfx
				laser_set_mode(LASER_MODE_OFF);
				e->xscroll -= kscroll_pan_dx;
				if (e->xscroll < INTTOFIX32(320))
				{
					e->xscroll = INTTOFIX32(320);
				}
			}
			else
			{
				// TODO: laser sfx (limit once...)
				laser_set_mode(LASER_MODE_ON);
			}

			// Lyle runs back to the left until he reaches a certain point.
			if (l->head.x > INTTOFIX32(392))
			{
				l->head.dx = -klyle_run_dx;
				l->head.direction = OBJ_DIRECTION_LEFT;
				do_lyle_walk_anim(e);
			}
			else
			{
				e->state = VYLE2_STATE_START_DELAY;
			}

			// Vyle runs to the left, until he reaches the center of the stage.
			if (e->head.x > INTTOFIX32(480))
			{
				e->head.dx = -klyle_run_dx;
				e->head.direction = OBJ_DIRECTION_LEFT;
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 6, kvyle_big_walk_anim_speed);
				e->metaframe = vyle_big_walk_cycle[e->anim_frame];
			}
			else
			{
				e->state = VYLE2_STATE_START_DELAY;
			}

			obj_accurate_physics(&l->head);
			map_set_x_scroll(FIX32TOINT(e->xscroll));
			break;
		case VYLE2_STATE_START_DELAY:
			if (e->state_elapsed == 0)
			{
				l->head.dx = 0;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				e->head.dx = 0;
				e->metaframe = 18;
			}
			do_lyle_shake_anim(e, l);

			if (e->state_elapsed == kstart_delay)
			{
				// TODO: Is his start state random?
				e->state = VYLE2_STATE_PRE_JUMP;
				lyle_set_control_en(1);
				lyle_set_master_en(1);
				lyle_set_scroll_h_en(0);
				music_play(11);
			}
			break;
	}

	if (state_prev != e->state) e->state_elapsed = 0;
	else e->state_elapsed++;
	obj_mixed_physics_h(o);

	render(e);
}

void o_load_vyle2(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Vyle2) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Vyle 2", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-32), INTTOFIX16(32), INTTOFIX16(-64), 5);
	o->left = INTTOFIX16(-18);
	o->right = INTTOFIX16(18);
	o->top = INTTOFIX16(-48);
	o->main_func = main_func;
	o->cube_func = NULL;

	lyle_set_scroll_h_en(0);
	lyle_set_scroll_v_en(0);
	map_set_y_scroll(0x28);

}

void o_unload_vyle2(void)
{
	s_vram_pos = 0;
}
