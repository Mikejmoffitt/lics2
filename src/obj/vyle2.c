// This object covers Vyle 2 - the final boss - but is also responsible for the
// scripting of the boss intro and ending scenes.
//
// Vyle 2 is placed in different maps with a parameter indicating the scene.
//
// Scene 0: Final boss fight intro; transitions to battle.
// Scene 1: Falling downwards in the "end1" room
// Scene 2: Lyle is blasted upwards back into the boss arena; Vyle is asbent
// Scene 3: Lyle boards the rocket and takes off to the credits room

#include "obj/vyle2.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "lyle.h"
#include "sfx.h"
#include "music.h"
#include "projectile.h"
#include "particle.h"
#include "obj/keddums.h"
#include "obj/laser.h"
#include "obj/psychowave.h"

// Vyle 2's sprite is huge (64x64px), so he's animated like Lyle, through DMA
// transfers, instead of loading everything at once.
#define VYLE2_VRAM_SIZE (8 * 8 * 32)

#define VYLE2_ARENA_CX INTTOFIX32(480)
#define VYLE2_ARENA_HALFWIDTH INTTOFIX32(116)
static uint16_t s_vram_pos;
static uint16_t s_vram_rocket_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	s_vram_pos = obj_vram_alloc(VYLE2_VRAM_SIZE) / 32;
	const Gfx *g = gfx_get(GFX_EX_ROCKET);
	s_vram_rocket_pos = gfx_load(g, obj_vram_alloc(g->size));
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
static int16_t kvyle_shaking_anim_speed;
static int16_t kstart_delay;

static int16_t kvyle_general_anim_delay;

static fix16_t kjump_shot_dy_thresh;

static fix16_t kjump_dy_table[8];
static fix16_t kgravity;
static fix16_t kgravity_sjump;

static fix16_t kjump_dx;

static int16_t kvyle_shot_phase_start_delay;
static int16_t kvyle_shot_interval;
static int16_t kvyle_shot_phase_duration;

static int16_t kvyle_precharge_duration;
static fix16_t kvyle_charge_dx;

static int16_t kvyle_superjump_anim_speed;

static fix32_t s_ground_y;

static int16_t kvyle_zap_duration;
static fix16_t kshot_speed;

static fix16_t kvyle_zap_recoil_dx;
static fix16_t kvyle_zap_recoil_dy;

static fix16_t kvyle_after_vuln_jump_dy;

static int16_t kvyle_vulnerable_timeout;

static fix16_t kvyle_superjump_dy;

static int16_t kvyle_hover_time;
static int16_t kvyle_superjump_down_dy;

// ------------------------------

static fix16_t kend1_scroll_speed;
static fix16_t kvyle_end1_fall_dy;
static fix16_t klyle_end1_fall_dy;
static fix16_t kvyle_end1_explode_sound_rate;

#define VYLE2_YSCROLL_BASE 32

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	kend1_scroll_speed = INTTOFIX16(PALSCALE_1ST(5 * 5.0 / 6.0));
	kvyle_end1_fall_dy = INTTOFIX16(PALSCALE_1ST(0.333 * 5.0 / 6.0));
	klyle_end1_fall_dy = INTTOFIX16(PALSCALE_1ST(1.0 * 5.0 / 6.0));
	kvyle_end1_explode_sound_rate = PALSCALE_DURATION(8 * 6.0 / 5.0);

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
	kvyle_shaking_anim_speed = PALSCALE_DURATION(4);
	kstart_delay = PALSCALE_DURATION(90);
	kvyle_general_anim_delay = PALSCALE_DURATION(16.67);
	kvyle_superjump_anim_speed = PALSCALE_DURATION(4);

	kjump_dy_table[0] = INTTOFIX16(PALSCALE_1ST(-5.208));
	kjump_dy_table[1] = INTTOFIX16(PALSCALE_1ST(-5.208 - (1*0.208)));
	kjump_dy_table[2] = INTTOFIX16(PALSCALE_1ST(-5.208 - (2*0.208)));
	kjump_dy_table[3] = INTTOFIX16(PALSCALE_1ST(-5.208 - (3*0.208)));
	kjump_dy_table[4] = INTTOFIX16(PALSCALE_1ST(-5.208 - (4*0.208)));
	kjump_dy_table[5] = INTTOFIX16(PALSCALE_1ST(-5.208 - (5*0.208)));
	kjump_dy_table[6] = INTTOFIX16(PALSCALE_1ST(-5.208 - (6*0.208)));
	kjump_dy_table[7] = INTTOFIX16(PALSCALE_1ST(-5.208 - (7*0.208)));

	kgravity = INTTOFIX16(PALSCALE_2ND(0.208));
	kgravity_sjump = INTTOFIX16(PALSCALE_2ND(0.1667));

	kvyle_shot_phase_start_delay = PALSCALE_DURATION(33.333);
	kvyle_shot_interval = PALSCALE_DURATION(12);
	kvyle_shot_phase_duration = PALSCALE_DURATION(208.333);

	kjump_shot_dy_thresh = INTTOFIX16(PALSCALE_1ST(-2.083));

	kjump_dx = INTTOFIX16(PALSCALE_1ST(0.8333));
	
	kvyle_precharge_duration = PALSCALE_DURATION(41.6667);
	kvyle_charge_dx = INTTOFIX16(PALSCALE_1ST(1.6667));

	kvyle_zap_duration = PALSCALE_DURATION(120);
	kshot_speed = INTTOFIX16(PALSCALE_1ST(3.0));

	kvyle_zap_recoil_dx = INTTOFIX16(PALSCALE_1ST(1.66667));
	kvyle_zap_recoil_dy = INTTOFIX16(PALSCALE_1ST(-4.1667));

	kvyle_after_vuln_jump_dy = INTTOFIX16(PALSCALE_1ST(-7.29167));

	kvyle_vulnerable_timeout = PALSCALE_DURATION(360);
	
	kvyle_superjump_dy = INTTOFIX16(PALSCALE_1ST(-5.8333));

	kvyle_hover_time = PALSCALE_DURATION(48);

	kvyle_superjump_down_dy = INTTOFIX16(PALSCALE_1ST(8.3333));

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
		// 06 - stand forward (med)
		{-12, -32, 54, SPR_SIZE(3, 4)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1},
		// 07 - stand forward (large)
		{-16, -48, 66, SPR_SIZE(4, 3)},
		{-16, -24, 78, SPR_SIZE(4, 3)},
		{-1, -1, -1, -1}, {-1, -1, -1, -1},

		// 08 - stand forward (full)
		FULLFRAME(0, 96),
		// 09 - charge 1
		FULLFRAME(1, 96),
		// 10 - charge 2
		FULLFRAME(2, 96),
		// 11 - charge 3
		FULLFRAME(3, 96),
		// 12 - charge 4
		FULLFRAME(4, 96),
		// 13 - charge 5
		FULLFRAME(5, 96),
		// 14 - charge 6
		FULLFRAME(6, 96),
		// 15 - stand forward (copy of 8?)
		FULLFRAME(7, 96),
		// 16 - mouth open (enemy eject)
		FULLFRAME(8, 96),
		// 17 - walk 1
		FULLFRAME(9, 96),
		// 18 - walk 2
		FULLFRAME(10, 96),
		// 19 - walk 3
		FULLFRAME(11, 96),
		// 20 - stance / prejump
		FULLFRAME(12, 96),
		// 21 - super jump prep 1
		FULLFRAME(13, 96),
		// 22 - super jump prep 2
		FULLFRAME(14, 96),
		// 23 - stand forward alt
		FULLFRAME(15, 96),
		// 24 - jump
		FULLFRAME(16, 96),
		// 25 - hurt 1
		FULLFRAME(17, 96),
		// 26 - hurt 2
		FULLFRAME(18, 96),
		// 27 - falling
		FULLFRAME(19, 96),
		// 28 - dead
		FULLFRAME(20, 96),
	};
#undef FULLFRAME

	if ((uint16_t)e->metaframe >= ARRAYSIZE(frames) || e->metaframe < 0) return;

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

	if (e->shaking)
	{
		sp_x += (system_rand() % 8) - 4;
		sp_y += (system_rand() % 8) - 4;
	}

	const int16_t flip = (o->direction == OBJ_DIRECTION_RIGHT);
	const SprDef *frame = &frames[e->metaframe * 4];

	// Sprite attribute base points to the start of VRAM allocated for Vyle2.
	const uint16_t attr = SPR_ATTR(s_vram_pos, flip, 0, ENEMY_PAL_LINE, 0);
	uint16_t spr_tile = 0;

	SprParam spr;

	for (uint16_t i = 0; i < 4; i++)
	{
		if (frame->size == -1) continue;  // Unused sprite.

		// X offset must be flipped about the Y axis, and then offset by the
		// sprite size as their origin is the top-left.
		const uint8_t spr_w = (((frame->size >> 2) & 0x03) + 1) * 8;
		const int16_t offs_x = (flip) ?
		                       (-frame->x - spr_w) :
		                       frame->x;
		spr.x = sp_x + offs_x;
		if (flip) spr.x -= 1;
		spr.y = sp_y + frame->y;
		spr.attr = attr + spr_tile;
		spr.size = frame->size;

		// Take a sprite slot for this asset.
		md_spr_put_st(&spr);;

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

static inline void do_lyle_angry_walk_anim(O_Vyle2 *e)
{
	static const int16_t lyle_walk_cycle[] = { 25, 26, 27, 26 };
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

static inline void do_lyle_angry_walk_hold_anim(O_Vyle2 *e)
{
	static const int16_t lyle_walk_cycle[] = { 2+8, 3+8, 2+8, 1+8 };
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

static inline void do_lyle_fall_anim(O_Vyle2 *e)
{
	O_Lyle *l = lyle_get();
	if (e->lyle_anim_cnt >= PALSCALE_DURATION(6))
	{
		e->lyle_anim_cnt = 0;
		e->lyle_anim_frame = (e->lyle_anim_frame == 0x0F) ? 0x10 : 0x0F;
		if (e->lyle_anim_frame == 0x0F)
		{
			l->head.direction = (l->head.direction == OBJ_DIRECTION_LEFT) ?
			                    OBJ_DIRECTION_RIGHT : OBJ_DIRECTION_LEFT;
		}
	}
	else
	{
		e->lyle_anim_cnt++;
	}
	if (e->lyle_anim_frame < 0x0F) e->lyle_anim_frame = 0x0F;
	else if (e->lyle_anim_frame > 0x10) e->lyle_anim_frame = 0x10;

	lyle_set_anim_frame(e->lyle_anim_frame);
}

static inline void do_periodic_fullscreen_explosions(O_Vyle2 *e)
{
	e->anim_cnt++;
	if (e->anim_cnt >= kvyle_end1_explode_sound_rate)
	{
		particle_spawn(e->xscroll + INTTOFIX32(system_rand() % GAME_SCREEN_W_PIXELS),
		               e->yscroll + INTTOFIX32(system_rand() % GAME_SCREEN_H_PIXELS),
		               PARTICLE_TYPE_EXPLOSION);
		sfx_stop(SFX_EXPLODE);
		sfx_play(SFX_EXPLODE, 0);
		e->anim_cnt = 0;
	}
}

//
// Fight state helpers
//

// Returns true if jump has occured.
static bool vyle2_prejump(O_Vyle2 *e)
{
	if (e->state_elapsed == 0)
	{
		e->head.dx = 0;
		e->head.dy = 0;
	}
	else if (e->state_elapsed == kvyle_general_anim_delay)
	{
		e->metaframe = 15;
	}
	else if (e->state_elapsed == kvyle_general_anim_delay * 2)
	{
		OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_shaking_anim_speed);
		e->metaframe = 20;
		if (e->anim_cnt == 0)
		{
			e->head.x += e->anim_frame ? INTTOFIX32(1) : INTTOFIX32(-1);
		}
	}
	else if (e->state_elapsed >= kvyle_general_anim_delay * 3)
	{
		return true;
	}
	return false;
}

static void vyle2_do_jump_towards(O_Vyle2 *e, fix32_t tx)
{
	// TODO: jump sfx
	e->metaframe = 24;
	e->jump_tx = tx;
	e->head.dy = kjump_dy_table[system_rand() % ARRAYSIZE(kjump_dy_table)];
	e->head.dx = kjump_dx * ((system_rand() % 3) + 1);
	if (e->head.x > tx) e->head.dx *= -1;
}

static void vyle2_ground_slam(O_Vyle2 *e)
{
	e->ground_slams++;
	const Gfx *g = gfx_get(GFX_EX_VYLE2_GROUND);

	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));

	if (e->ground_slams == 0) return;

	const uint16_t data_offs = (e->ground_slams - 1) * 32 * 4;

	md_dma_transfer_vram(0x1480, g->data + data_offs, 32 * 4 / 2, 2);
	md_dma_transfer_vram(0x1680, g->data + data_offs + 32 * 4 * 4, 32 * 4 / 2, 2);
}

// Returns true if landed.
static bool vyle2_jump_midair_logic(O_Vyle2 *e, bool hone_to_tx)
{
	const O_Lyle *l = lyle_get();
	e->head.dy += kgravity;

	// Bouncing off the walls during a jump.
	if ((e->head.x > VYLE2_ARENA_CX + VYLE2_ARENA_HALFWIDTH && e->head.dx > 0) ||
	    (e->head.x < VYLE2_ARENA_CX - VYLE2_ARENA_HALFWIDTH && e->head.dx < 0))
	{
		e->head.dx *= -1;
	}

	// Fire a shot at Lyle during ascent.
	if (e->head.dy < 0 && !e->shot_at_lyle && e->head.dy >= kjump_shot_dy_thresh)
	{
		e->shot_at_lyle = true;
		// TODO: Shot speed
		projectile_shoot_at(e->head.x, e->head.y - INTTOFIX32(24), PROJECTILE_TYPE_BALL2,
		                    l->head.x, l->head.y - INTTOFIX32(10), kshot_speed);
		sfx_play(SFX_GAXTER_SHOT, 1);
	}

	// The last jump hones in to the center.
	if (hone_to_tx)
	{
		// Snap to center once it's been reached.
		if ((e->head.x >= e->jump_tx && e->head.dx > 0) ||
		    (e->head.x <= e->jump_tx && e->head.dx < 0))
		{
			e->head.x = e->jump_tx;
			e->head.dx = 0;
		}
		if (e->head.x < e->jump_tx && e->head.dx < 0) e->head.dx *= -1;
		else if (e->head.x > e->jump_tx && e->head.dx > 0) e->head.dx *= -1;
	}

	// Landing
	if (e->head.dy > 0 && e->head.y >= s_ground_y)
	{
		e->shot_at_lyle = 0;
		e->head.y = s_ground_y;
		e->head.dy = 0;
		e->head.dx = 0;
		return true;
	}
	return false;
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

	static const uint16_t vyle_walk_cycle[] = { 2, 1, 2, 3 };
	static const uint16_t vyle_big_walk_cycle[] = { 9, 10, 11, 12, 13, 14};
	static const uint16_t vyle_precharge_cycle[] = { 10, 11, 13, 14 };

	if (e->state == VYLE2_STATE_INIT)
	{
		lyle_set_control_en(false);
		lyle_set_master_en(false);
		lyle_set_scroll_h_en(false);
		lyle_set_scroll_v_en(false);
		e->xscroll = 0;
		e->yscroll = INTTOFIX32(VYLE2_YSCROLL_BASE);

		e->state = e->first_state;
	}

	switch (e->state)
	{
		default:
		case VYLE2_STATE_INIT:
			break;

//
// Final boss fight intro.
//

		case VYLE2_STATE_LYLE_WALK_ANGRY:
			if (e->state_elapsed == 0)
			{
				o->direction = OBJ_DIRECTION_LEFT;
				e->metaframe = 0;
				l->head.dx = klyle_walk_dx;
				l->head.dy = 0;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				keddums_set_state(KEDDUMS_FLOAT);
				psychowave_set_state(PWAVE_STATE_OFF);
			}
			// Lyle begins by approaching until he reaches a certain point.
			do_lyle_angry_walk_anim(e);

			// Once lyle has walked far enough, proceed.
			if (l->head.x >= INTTOFIX32(92)) e->state = VYLE2_STATE_CAMERA_PAN_TO_MACHINE;
			obj_accurate_physics(&l->head);
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

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
				sfx_play(SFX_MEOW, 1);
			}

			break;

		case VYLE2_STATE_KEDDUMS_MEOW:
			// TODO: Is this sequence quite right?
			keddums_set_state(KEDDUMS_SHAKE);
			// After a short time, keddums meows, and the screen moves back.
			if (e->state_elapsed > kkeddums_meow_delay_frames)
			{
				e->state = VYLE2_STATE_CAMERA_PAN_TO_LYLE;
			}
			break;

		case VYLE2_STATE_CAMERA_PAN_TO_LYLE:
			// Camera pans back to Lyle.
			e->xscroll -= kscroll_pan_dx;
			keddums_set_state(KEDDUMS_FLOAT);
			if (e->xscroll < 0)
			{
				e->xscroll = 0;
				e->state = VYLE2_STATE_LYLE_APPROACH_MACHINE;
			}
			break;

		case VYLE2_STATE_LYLE_APPROACH_MACHINE:
			// Lyle now walks forwards, bringing the camera with him.
			lyle_set_scroll_h_en(1);
			do_lyle_angry_walk_anim(e);
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
				e->xscroll = INTTOFIX32(px);
			}

			// Once lyle has gone far enough (and Vyle has reached the machine) proceed.
			if (l->head.x >= map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS + 51))
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
				sfx_play(SFX_BEEP, 0);
				keddums_set_state(KEDDUMS_SHAKE);
				psychowave_set_state(PWAVE_STATE_ON);
				// TODO: activate big orb thing
				e->metaframe = 4;
			}
			else if (e->state_elapsed == (kvyle_activation_delay_frames +
			                              kvyle_activation_duration_frames))
			{
				// Done pushing the button.
				sfx_play(SFX_MEOW, 1);
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

				if (e->head.x < map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS) + INTTOFIX32(43))
				{
					e->state = VYLE2_STATE_VYLE_GROW_1;
					e->head.x = map_get_right() - INTTOFIX32(GAME_SCREEN_W_PIXELS) + INTTOFIX32(43);
				}
			}
			break;

		case VYLE2_STATE_VYLE_GROW_1:
			if (e->state_elapsed == 0)
			{
				o->dx = 0;
				e->metaframe = 5;
			}

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_VYLE_GROW_2;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

			do_lyle_shake_anim(e, l);
			break;

		case VYLE2_STATE_VYLE_GROW_2:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_TELEPORT, 3);
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
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

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
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

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
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

			if (e->state_elapsed == kvyle_grow_post_frames)
			{
				e->state = VYLE2_STATE_ENTER_ARENA;
				psychowave_set_state(PWAVE_STATE_OFF);
				keddums_set_state(KEDDUMS_FLOAT);
			}

			do_lyle_shake_anim(e, l);
			break;

		case VYLE2_STATE_ENTER_ARENA:
			// The camera pans back until it reaches the arena.
			if (e->xscroll > INTTOFIX32(320))
			{
				laser_set_mode(LASER_MODE_OFF);
				e->xscroll -= kscroll_pan_dx;
				if (e->xscroll < INTTOFIX32(320))
				{
					e->xscroll = INTTOFIX32(320);
				}
			}
			else
			{
				laser_set_mode(LASER_MODE_ON);
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

			// Lyle runs back to the left until he reaches a certain point.
			if (l->head.x > INTTOFIX32(392))
			{
				l->head.dx = -klyle_run_dx;
				l->head.direction = OBJ_DIRECTION_LEFT;
				do_lyle_angry_walk_anim(e);
			}
			else
			{
				e->state = VYLE2_STATE_START_DELAY;
			}

			// Vyle runs to the left, until he reaches the center of the stage.
			if (e->head.x > VYLE2_ARENA_CX)
			{
				e->head.dx = -klyle_run_dx;
				e->head.direction = OBJ_DIRECTION_LEFT;
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 6, kvyle_big_walk_anim_speed);
				if (e->anim_cnt == 0)
				{
					if (e->anim_frame == 2 || e->anim_frame == 5)
					{
						sfx_play(SFX_KNOCK, 1);
					}
				}
				e->metaframe = vyle_big_walk_cycle[e->anim_frame];
			}
			else
			{
				e->state = VYLE2_STATE_START_DELAY;
			}

			obj_accurate_physics(&l->head);
			break;

		case VYLE2_STATE_START_DELAY:
			if (e->state_elapsed == 0)
			{
				l->head.dx = 0;
				l->head.direction = OBJ_DIRECTION_RIGHT;
				e->head.dx = 0;
				e->metaframe = 18;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			do_lyle_shake_anim(e, l);

			if (e->state_elapsed == kstart_delay)
			{
				// TODO: Is his start state random?
				e->state = VYLE2_STATE_PRE_JUMP;
				e->jump_count = 0;
				lyle_set_control_en(1);
				lyle_set_master_en(1);
				lyle_set_scroll_h_en(0);
				music_play(11);
				s_ground_y = e->head.y;
			}
			break;

//
// Fight states.
//
		case VYLE2_STATE_PRE_JUMP:
			if (vyle2_prejump(e))
			{
				e->state = VYLE2_STATE_JUMP;
				e->jump_count++;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_JUMP:
			if (e->state_elapsed == 0)
			{
				vyle2_do_jump_towards(e, (e->jump_count < 6) ? l->head.x : VYLE2_ARENA_CX);
			}
			if (vyle2_jump_midair_logic(e, e->jump_count >= 6))
			{
				e->state = VYLE2_STATE_LAND;
				sfx_play(SFX_SLAM, 5);
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_LAND:
			e->metaframe = 22;
			if (e->jump_count < 6 || e->head.x != VYLE2_ARENA_CX)
			{
				e->state = VYLE2_STATE_PRE_JUMP;
			}
			else if (e->state_elapsed == kvyle_general_anim_delay)
			{
				e->jump_count = 0;
				e->state = VYLE2_STATE_SHOOTING;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;  // --> PRE_JUMP or SHOOTING

		case VYLE2_STATE_SHOOTING:
			if (e->state_elapsed == 0)
			{
				e->shots_remaining = 18;
				e->shot_cnt = 0;
				e->metaframe = 15;
				e->head.x = VYLE2_ARENA_CX;
			}
			else if (e->state_elapsed > kvyle_shot_phase_start_delay)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_superjump_anim_speed);
				e->metaframe = 21 + e->anim_frame;

				if (e->shots_remaining > 0)
				{
					e->shot_cnt++;
					if (e->shot_cnt >= kvyle_shot_interval)
					{
						e->shot_cnt = 0;
						e->shots_remaining--;
						const int16_t shot_angle = 64 - 8 * (1 + ((system_rand() % 9)));
						const int16_t shot_angle_2 = 64 + 8 * (1 + ((system_rand() % 9)));
						projectile_shoot_angle(e->head.x, e->head.y - INTTOFIX32(18), PROJECTILE_TYPE_BALL2,
						                       shot_angle, kshot_speed);
						projectile_shoot_angle(e->head.x, e->head.y - INTTOFIX32(18), PROJECTILE_TYPE_BALL2,
						                       shot_angle_2, kshot_speed);
						sfx_play(SFX_GAXTER_SHOT, 1);
					}
				}

				if (e->state_elapsed >= kvyle_shot_phase_duration)
				{
					e->state = VYLE2_STATE_EDGE_PRE_JUMP;
				}
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_EDGE_PRE_JUMP:
			if (vyle2_prejump(e))
			{
				e->state = VYLE2_STATE_EDGE_JUMP;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_EDGE_JUMP:
			if (e->state_elapsed == 0)
			{
				vyle2_do_jump_towards(e, VYLE2_ARENA_CX + VYLE2_ARENA_HALFWIDTH - INTTOFIX32(4));
			}
			if (vyle2_jump_midair_logic(e, true))
			{
				sfx_play(SFX_SLAM, 5);
				e->state = VYLE2_STATE_EDGE_LAND;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_EDGE_LAND:
			e->metaframe = 22;
			if (e->head.x < VYLE2_ARENA_CX + VYLE2_ARENA_HALFWIDTH - INTTOFIX32(4))
			{
				e->state = VYLE2_STATE_EDGE_PRE_JUMP;
			}
			else if (e->state_elapsed == kvyle_general_anim_delay)
			{
				e->metaframe = 15;
				e->state = VYLE2_STATE_PRE_BELCH;
				e->shot_cnt = 0;
				e->shots_remaining = 1 + (system_rand() % 4);
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;  // --> EDGE_PRE_JUMP or PRE_BELCH

		case VYLE2_STATE_PRE_BELCH:
			if (e->state_elapsed == kvyle_general_anim_delay)
			{
				e->metaframe = 18;
			}
			// Exit condition
			if (e->shots_remaining == 0)
			{
				if (e->state_elapsed >= kvyle_general_anim_delay * 2 &&
				    (obj_find_by_type(OBJ_GAXTER1) == NULL))
				{
					e->state = VYLE2_STATE_PRE_CHARGE;
				}
				else if (e->state_elapsed >= kvyle_general_anim_delay * 4)
				{
					e->state = VYLE2_STATE_PRE_CHARGE;
				}
			}
			else if (e->shots_remaining > 0 && e->state_elapsed == kvyle_general_anim_delay*3)
			{
				e->state = VYLE2_STATE_BELCH;
				e->shots_remaining--;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;  // --> BELCH or PRE_CHARGE

		case VYLE2_STATE_BELCH:
			if (e->state_elapsed == 0)
			{
				e->metaframe = 16;
				sfx_play(SFX_ROAR, 2);
				obj_spawn(FIX32TOINT(e->head.x) - 8, FIX32TOINT(e->head.y) - 48, OBJ_GAXTER1, 0);
			}
			else if (e->state_elapsed < kvyle_general_anim_delay*2)
			{
				OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_shaking_anim_speed);
				if (e->anim_cnt == 0)
				{
					e->head.x += e->anim_frame ? INTTOFIX32(1) : INTTOFIX32(-1);
				}
			}
			else
			{
				e->state = VYLE2_STATE_PRE_BELCH;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_PRE_CHARGE:
			if (e->state_elapsed >= kvyle_precharge_duration)
			{
				e->state = VYLE2_STATE_CHARGE;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, ARRAYSIZE(vyle_precharge_cycle), kvyle_big_walk_anim_speed);
			e->metaframe = vyle_precharge_cycle[e->anim_frame];
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_CHARGE:
			if (e->state_elapsed == 0)
			{
				e->head.dx = -kvyle_charge_dx;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, ARRAYSIZE(vyle_big_walk_cycle), kvyle_big_walk_anim_speed);
			e->metaframe = vyle_big_walk_cycle[e->anim_frame];
			if (e->head.x <= VYLE2_ARENA_CX - VYLE2_ARENA_HALFWIDTH - INTTOFIX32(28))
			{
				e->state = VYLE2_STATE_ZAP;
				e->head.dx = 0;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_ZAP:
			if (e->state_elapsed == 0)
			{
				sfx_play(SFX_LASER_CONSTANT, 0);
				e->shaking = true;
				e->metaframe = 16;
			}
			else if (e->state_elapsed >= kvyle_zap_duration)
			{
				sfx_stop(SFX_LASER_CONSTANT);
				sfx_play(SFX_NOISE_SILENCE, 0);
				e->state = VYLE2_STATE_ZAP_RECOIL;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_ZAP_RECOIL:
			if (e->state_elapsed == 0)
			{
				e->shaking = false;
				e->head.dx = kvyle_zap_recoil_dx;
				e->head.dy = kvyle_zap_recoil_dy;
			}
			e->head.dy += kgravity;
			e->metaframe = 25;
			if (e->head.dy >= 0 && e->head.y >= s_ground_y)
			{
				sfx_play(SFX_SLAM, 5);
				e->state = VYLE2_STATE_ZAP_RECOIL_BOUNCED;
				e->head.y = s_ground_y;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_ZAP_RECOIL_BOUNCED:
			if (e->state_elapsed == 0)
			{
				e->head.dx = kvyle_zap_recoil_dx / 2;
				e->head.dy = (kvyle_zap_recoil_dy * 3) / 4;
			}
			e->head.dy += kgravity;
			if (e->head.dy >= 0 && e->head.y >= s_ground_y)
			{
				sfx_play(SFX_SLAM, 5);
				e->state = VYLE2_STATE_VULNERABLE;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_VULNERABLE:
			if (e->state_elapsed == 0)
			{
				e->anim_frame = 0;
				e->anim_cnt = 0;
				e->head.y = s_ground_y;
				e->head.dy = 0;
				e->head.dx = 0;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_shaking_anim_speed);
			e->metaframe = 25 + e->anim_frame;

			// Cube function marks a hit with an HP change.

			if (e->state_elapsed == kvyle_vulnerable_timeout)
			{
				e->state = VYLE2_STATE_PRE_JUMP;
			}
			else if (e->head.hp != 127)
			{
				e->head.hp = 127;  // Don't actually let him die
				e->state = VYLE2_STATE_CENTER_PRE_JUMP;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_CENTER_PRE_JUMP:
			if (vyle2_prejump(e))
			{
				e->state = VYLE2_STATE_CENTER_JUMP;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_CENTER_JUMP:
			if (e->state_elapsed == 0)
			{
				vyle2_do_jump_towards(e, VYLE2_ARENA_CX);
				e->head.dy = kvyle_after_vuln_jump_dy;
			}
			if (vyle2_jump_midair_logic(e, true))
			{
				sfx_play(SFX_SLAM, 5);
				e->state = VYLE2_STATE_CENTER_LAND;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_CENTER_LAND:
			e->metaframe = 22;
			if (e->head.x != VYLE2_ARENA_CX)
			{
				e->state = VYLE2_STATE_CENTER_PRE_JUMP;
			}
			else if (e->state_elapsed == kvyle_general_anim_delay)
			{
				e->metaframe = 15;
				e->state = VYLE2_STATE_PRE_SUPERJUMP;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;  // CENTER_PRE_JUMP OR PRE_SUPERJUMP

		case VYLE2_STATE_PRE_SUPERJUMP:
			if (e->state_elapsed == 0)
			{
			}
			else if (e->state_elapsed >= kvyle_general_anim_delay * 7.5)
			{
				e->state = VYLE2_STATE_SUPERJUMP_UP;
			}
			else if (e->state_elapsed >= kvyle_general_anim_delay * 4)
			{
				e->metaframe = 21 + e->anim_frame;
			}
			else if (e->state_elapsed >= kvyle_general_anim_delay)
			{
				e->metaframe = 20;
				if (e->anim_cnt == 0)
				{
					e->head.x += e->anim_frame ? INTTOFIX32(1) : INTTOFIX32(-1);
				}
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_shaking_anim_speed);

			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_SUPERJUMP_UP:
			if (e->state_elapsed == 0)
			{
				e->metaframe = 24;
				e->head.dy = kvyle_superjump_dy;
			}
			e->head.dy += kgravity_sjump;
			if (e->head.dy >= 0 || e->head.y <= INTTOFIX32(48))
			{
				e->state = VYLE2_STATE_SUPERJUMP_HOVER;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_SUPERJUMP_HOVER:
			if (e->state_elapsed == 0)
			{
				e->shaking = true;
				e->head.dy = 0;
				e->metaframe = 20;
			}
			else if (e->state_elapsed >= kvyle_hover_time)
			{
				e->state = VYLE2_STATE_SUPERJUMP_DOWN;
			}
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kvyle_shaking_anim_speed);
			if (e->anim_cnt == 0)
			{
				e->head.x += e->anim_frame ? INTTOFIX32(1) : INTTOFIX32(-1);
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_SUPERJUMP_DOWN:
			if (e->state_elapsed == 0)
			{
				e->shaking = false;
				e->metaframe = 24;
				e->head.dy = kvyle_superjump_down_dy;
			}
			if (e->head.dy > 0 && e->head.y >= s_ground_y)
			{
				vyle2_ground_slam(e);
				e->crumble_cnt = 40;
				if (e->ground_slams < 4)
				{
					e->head.y = s_ground_y;
					e->head.dy = 0;
					e->jump_count = 0;
					e->state = VYLE2_STATE_LAND;
					sfx_play(SFX_SLAM, 5);

					if (l->grounded)
					{
						lyle_get_hurt(false);
					}
				}
				else
				{
					e->state = VYLE2_STATE_SUPERJUMP_EXIT;
				}
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;  // --> VYLE2_STATE_LAND or VYLE2_STATE_SUPERJUMP_EXIT

		case VYLE2_STATE_SUPERJUMP_EXIT:
			if (e->state_elapsed == 0)
			{
				lyle_set_control_en(false);
				lyle_set_master_en(false);
				l->head.dx = 0;
				l->head.dy = 0;
				lyle_set_anim_frame(4);
			}
			l->head.dy += kgravity / 2;
			obj_accurate_physics(&l->head);

			if (e->head.y >= INTTOFIX32(240+96))
			{
				e->head.dy = 0;
			}

			if (l->head.y >= INTTOFIX32(240+96))
			{
				// To ending (part 1)
				map_set_next_room(59, 0);
				map_set_exit_trigger(MAP_EXIT_BOTTOM);
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

//
// Ending sequence (part 1)
//

		case VYLE2_STATE_END1_FALL_REPEAT:
			if (e->state_elapsed == 0)
			{
				e->head.direction = OBJ_DIRECTION_LEFT;
				e->head.dy = 0;  // handling this one manually;
				e->metaframe = 27;
				l->head.dx = 0;
				l->head.y -= INTTOFIX32(24);
				l->head.dy = klyle_end1_fall_dy;
				md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));

				e->fall_cycles = 0;
			}

			do_lyle_fall_anim(e);

			// Vyle's shaking
			OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, PALSCALE_DURATION(2));
			if (e->anim_cnt == 0)
			{
				e->head.x += e->anim_frame ? INTTOFIX32(1) : INTTOFIX32(-1);
			}

			e->yscroll += kend1_scroll_speed;
			e->head.y += kend1_scroll_speed + kvyle_end1_fall_dy;
			l->head.y += kend1_scroll_speed;
			if (e->yscroll > INTTOFIX32(240+VYLE2_YSCROLL_BASE))
			{
				e->yscroll -= INTTOFIX32(240);
				e->head.y -= INTTOFIX32(240);
				l->head.y -= INTTOFIX32(240);
				e->fall_cycles++;
			}

			if (e->fall_cycles >= 6)
			{
				e->state = VYLE2_STATE_END1_FALL_DOWN;
			}

			obj_accurate_physics(&l->head);
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_END1_FALL_DOWN:
			if (e->state_elapsed == 0)
			{
				e->yscroll = INTTOFIX32(480 + VYLE2_YSCROLL_BASE);
				map_set_y_scroll(FIX32TOINT(e->yscroll));
				map_redraw_room();

				l->head.y = e->yscroll - INTTOFIX32(64);
				l->head.dy = INTTOFIX16(PALSCALE_1ST(5 * 5.0 / 6.0));

				e->head.y = e->yscroll - INTTOFIX32(64);
				e->head.dy = 0;
			}
			else
			{
				do_lyle_fall_anim(e);
				obj_accurate_physics(&l->head);
				// Lyle tumbles as he falls, until he hits the platform or ground.
				const int16_t px_x = FIX32TOINT(l->head.x + (l->head.right + l->head.left) / 2);
				const int16_t py_bottom = FIX32TOINT(l->head.y);
				if (map_collision(px_x, py_bottom + 1))
				{
					const int16_t touching_tile_y = ((py_bottom + 1) / 8) * 8;
					l->head.y = INTTOFIX32(touching_tile_y) - 1;
					l->head.dy = 0;
					e->state = VYLE2_STATE_END1_LYLE_LANDED;
				}
			}
			break;

		case VYLE2_STATE_END1_LYLE_LANDED:
			if (e->state_elapsed == 0)
			{
				lyle_set_anim_frame(0x11);
				e->lyle_anim_cnt = 0;
			}
			else if (e->state_elapsed < PALSCALE_DURATION(40 * 6.0 / 5.0))
			{
				e->lyle_anim_cnt++;
				if (e->lyle_anim_cnt >= PALSCALE_DURATION(11 * 6.0 / 5.0))
				{
					lyle_set_anim_frame(0x12);
				}
			}
			else if (e->state_elapsed == PALSCALE_DURATION(40 * 6.0/5.0))
			{
				lyle_set_anim_frame(0x13);
			}
			else if (e->state_elapsed >= PALSCALE_DURATION(40 * 6.0/5.0) &&
			         e->state_elapsed < PALSCALE_DURATION(60 * 6.0/5.0))
			{
				// Shaking as he gets up
				OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 2, kvyle_shaking_anim_speed);
				if (e->lyle_anim_cnt == 0)
				{
					l->head.x += e->lyle_anim_frame ? INTTOFIX32(-1) : INTTOFIX32(1);
				}
			}
			else if (e->state_elapsed == PALSCALE_DURATION(60 * 6.0/5.0))
			{
				lyle_set_anim_frame(0x0D);
				l->head.direction = OBJ_DIRECTION_LEFT;
				e->head.dy = INTTOFIX16(PALSCALE_1ST(8 * 5.0/6.0));
			}
			else if (e->state_elapsed == PALSCALE_DURATION(150 * 6.0/5.0))
			{
				e->state = VYLE2_STATE_END1_EXPLODING;
			}

			if (e->head.dy > 0 && e->head.y >= INTTOFIX32(584+48))
			{
				e->head.dy = 0;
				e->head.y = INTTOFIX32(584+48);
				e->metaframe = 28;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_END1_EXPLODING:
			if (e->state_elapsed == 0)
			{
				e->shaking = true;
			}

			// Explosion sounds
			if (e->state_elapsed < PALSCALE_DURATION(250 * 6.0 / 5.0))
			{
				e->anim_cnt++;
				if (e->anim_cnt >= kvyle_end1_explode_sound_rate)
				{
					if (e->state_elapsed < PALSCALE_DURATION(100 * 6.0 / 5.0))
					{
						sfx_stop(SFX_OBJ_BURST);
						sfx_play(SFX_OBJ_BURST, 5);
						sfx_stop(SFX_OBJ_BURST_HI);
						sfx_play(SFX_OBJ_BURST_HI, 6);
						particle_spawn(e->head.x, e->head.y + (e->head.top / 2), PARTICLE_TYPE_FIZZLERED);
					}
					else
					{
						sfx_stop(SFX_EXPLODE);
						sfx_play(SFX_EXPLODE, 0);
						const fix32_t ex = e->head.x + INTTOFIX32((system_rand() % 54) - (54 / 2));
						const fix32_t ey = e->head.y - INTTOFIX32((system_rand() % 47) + (47 / 2));
						particle_spawn(ex, ey, PARTICLE_TYPE_EXPLOSION);
					}
					e->anim_cnt = 0;
				}
			}
			else if (e->state_elapsed == PALSCALE_DURATION(250 * 6.0 / 5.0))
			{
				sfx_play(SFX_EXPLODE, 0);
			}
			else if (e->state_elapsed < PALSCALE_DURATION(260 * 6.0 / 5.0))
			{
				sfx_play(SFX_EXPLODE, 0);
				for (int16_t i = 0; i < 3; i++)
				{
					const fix32_t ex = e->head.x + INTTOFIX32((system_rand() % 54) - (54 / 2));
					const fix32_t ey = e->head.y + INTTOFIX32((e->head.top / 2) - (system_rand() % 47) + (47 / 2));
					Obj *explosion = obj_spawn(FIX32TOINT(ex), FIX32TOINT(ey), OBJ_BIGEXPLOSION, 0);
					if (!explosion) break;
					explosion->dx = INTTOFIX16(((system_rand() % 32) - 16) / 4);
					explosion->dy = INTTOFIX16(((system_rand() % 32) - 16) / 4);
					particle_spawn(ex, ey, PARTICLE_TYPE_EXPLOSION);
				}
			}
			else
			{
				e->state = VYLE2_STATE_END1_EXPLODED;
			}
			md_pal_set(ENEMY_CRAM_POSITION + 0xC, PALRGB(0x7, 0x3, 0x3));
			break;

		case VYLE2_STATE_END1_EXPLODED:
			if (e->state_elapsed == 0)
			{
				e->metaframe = -1;  // Disappear
				l->head.dy = INTTOFIX16(PALSCALE_1ST(-5 * 5.0 / 6.0));
			}
			else if (e->state_elapsed == PALSCALE_DURATION(20 * 6.0 / 5.0))
			{
				map_redraw_room();
				e->yscroll = INTTOFIX32(VYLE2_YSCROLL_BASE);
				l->head.dy = 0;
			}
			else if (e->state_elapsed == PALSCALE_DURATION(50 * 6.0 / 5.0))
			{
				l->head.y = INTTOFIX32(282);
				l->head.dy = INTTOFIX16(PALSCALE_1ST(-5 * 5.0 / 6.0));
			}
			else if (l->head.y < INTTOFIX32(-32))
			{
				// To part 2
				map_set_next_room(60, 0);
				map_set_exit_trigger(MAP_EXIT_TOP);
			}
			do_lyle_fall_anim(e);
			obj_accurate_physics(&l->head);
			break;

//
// Ending sequence (part 2)
//
		case VYLE2_STATE_END2_LYLE_RISE_UP:
			if (e->state_elapsed == 0)
			{
				e->metaframe = -1;  // Hide vyle
				l->head.direction = OBJ_DIRECTION_RIGHT;
				l->head.dy = INTTOFIX16(PALSCALE_1ST((-35 / 5.0) * (5.0 / 6.0))); // TODO: Real number
				l->head.dx = INTTOFIX16(PALSCALE_1ST(2.0 * 5.0 / 6.0));
				e->xscroll = l->head.x - INTTOFIX32(GAME_SCREEN_W_PIXELS / 2);
				map_set_x_scroll(FIX32TOINT(e->xscroll));
				map_redraw_room();
				psychowave_set_state(PWAVE_STATE_OFF);
			}
			
			do_periodic_fullscreen_explosions(e);

			l->head.dy += INTTOFIX16(PALSCALE_2ND((1.0 / 5.0) * 5.0 / 6.0));
			e->xscroll = l->head.x - INTTOFIX32(GAME_SCREEN_W_PIXELS / 2);
			do_lyle_fall_anim(e);
			if (l->head.dy > 0)
			{
				const int16_t px_x = FIX32TOINT(l->head.x + (l->head.right + l->head.left) / 2);
				const int16_t py_bottom = FIX32TOINT(l->head.y);
				if (map_collision(px_x, py_bottom + 1))
				{
					const int16_t touching_tile_y = ((py_bottom + 1) / 8) * 8;
					l->head.y = INTTOFIX32(touching_tile_y) - 1;
					l->head.dx = 0;
					l->head.dy = 0;
					e->state = VYLE2_STATE_END2_LYLE_LANDED;
				}
			}
			obj_accurate_physics(&l->head);
			break;

		case VYLE2_STATE_END2_LYLE_LANDED:
			if (e->state_elapsed == 0)
			{
				l->head.direction = OBJ_DIRECTION_RIGHT;
				lyle_set_anim_frame(0x11);
				e->lyle_anim_cnt = 0;
			}
			else if (e->state_elapsed < PALSCALE_DURATION(40 * 6.0 / 5.0))
			{
				e->lyle_anim_cnt++;
				if (e->lyle_anim_cnt >= PALSCALE_DURATION(11 * 6.0 / 5.0))
				{
					lyle_set_anim_frame(0x12);
				}
			}
			else if (e->state_elapsed == PALSCALE_DURATION(40 * 6.0/5.0))
			{
				lyle_set_anim_frame(0x13);
			}
			else if (e->state_elapsed >= PALSCALE_DURATION(40 * 6.0/5.0) &&
			         e->state_elapsed < PALSCALE_DURATION(60 * 6.0/5.0))
			{
				// Shaking as he gets up
				OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 2, kvyle_shaking_anim_speed);
				if (e->lyle_anim_cnt == 0)
				{
					l->head.x += e->lyle_anim_frame ? INTTOFIX32(-1) : INTTOFIX32(1);
				}
			}
			else if (e->state_elapsed >= PALSCALE_DURATION(60 * 6.0/5.0) &&
			         e->state_elapsed < PALSCALE_DURATION(90 * 6.0/5.0))
			{
				do_lyle_shake_anim(e, l);
			}
			else if (e->state_elapsed == PALSCALE_DURATION(90 * 6.0/5.0))
			{
				e->state = VYLE2_STATE_END2_LYLE_CAT_LAUNCHED;
			}
			do_periodic_fullscreen_explosions(e);
			break;

		case VYLE2_STATE_END2_LYLE_CAT_LAUNCHED:
			if (e->state_elapsed == 0)
			{
				l->head.dx = 0;
				l->head.dy = 0;
				keddums_set_state(KEDDUMS_FLY);
				psychowave_set_state(PWAVE_STATE_BROKEN);
				// TODO: Spawn glass shard objects
				sfx_play(SFX_OBJ_BURST, 3);
				sfx_play(SFX_OBJ_BURST_HI, 3);
			}

			if (e->state_elapsed < PALSCALE_DURATION(20 * 6.0 / 5.0))
			{
				do_lyle_shake_anim(e, l);
			}
			else if (e->state_elapsed == PALSCALE_DURATION(20 * 6.0 / 5.0))
			{
				l->head.dx = INTTOFIX16(PALSCALE_1ST(2.0 * 5.0 / 6.0));
				l->head.dy = INTTOFIX16(PALSCALE_1ST((-20 / 5.0) * 5.0 / 6.0));
				lyle_set_anim_frame(25);
				sfx_play(SFX_JUMP, 0);
			}
			else
			{
				l->head.dy += INTTOFIX16(PALSCALE_2ND((1.0 / 5.0) * (5.0 / 6.0) * (5.0 / 6.0)));
				if (l->head.dy > 0 && l->head.y >= INTTOFIX32(224))
				{
					l->head.y = INTTOFIX32(224);
					l->head.dy = 0;
					e->state = VYLE2_STATE_END2_LYLE_ESCAPE;
				}

				O_Keddums *k = keddums_get();
				if (l->head.x >= k->head.x)
				{
					keddums_set_state(KEDDUMS_FOLLOW_LYLE);
					lyle_set_anim_frame(9);
				}
				e->xscroll = l->head.x - INTTOFIX32(GAME_SCREEN_W_PIXELS / 2);
			}

			obj_accurate_physics(&l->head);
			break;

		case VYLE2_STATE_END2_LYLE_ESCAPE:
			do_lyle_angry_walk_hold_anim(e);

			obj_accurate_physics(&l->head);
			if (l->head.x >= INTTOFIX32(960))
			{
				// To part 3
				map_set_next_room(61, 0);
				map_set_exit_trigger(MAP_EXIT_RIGHT);
			}
			e->xscroll = l->head.x - INTTOFIX32(GAME_SCREEN_W_PIXELS / 2);
			break;

//
// Ending Sequence (part 3)
//

		case VYLE2_STATE_END3_LYLE_ENTER:
			if (e->state_elapsed == 0)
			{
				l->head.x = INTTOFIX32(-16);
				e->metaframe = -1;  // Hide vyle
				l->head.direction = OBJ_DIRECTION_RIGHT;
				keddums_set_state(KEDDUMS_FOLLOW_LYLE);
				l->head.dx = INTTOFIX16(PALSCALE_1ST(1.0 * 5.0 / 6.0));
				l->head.dy = 0;
			}

			do_lyle_angry_walk_hold_anim(e);
			obj_accurate_physics(&l->head);

			if (e->state_elapsed == PALSCALE_DURATION(50 * 6.0 / 5.0))
			{
				e->state = VYLE2_STATE_END3_LYLE_HOP_DOWN;
			}

			md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
			           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
			           SPR_SIZE(2, 3));
			break;

		case VYLE2_STATE_END3_LYLE_HOP_DOWN:
			if (e->state_elapsed == 0)
			{
				lyle_set_anim_frame(4+8);
				l->head.dy = INTTOFIX16(PALSCALE_1ST((-10 / 5.0) * 5.0 / 6.0));
				sfx_play(SFX_JUMP, 0);
			}
			l->head.dy += INTTOFIX16(PALSCALE_2ND((1.0 / 5.0) * 5.0 / 6.0));
			if (l->head.dy > 0 && l->head.y > INTTOFIX32(128))
			{
				const int16_t px_x = FIX32TOINT(l->head.x + (l->head.right + l->head.left) / 2);
				const int16_t py_bottom = FIX32TOINT(l->head.y);
				if (map_collision(px_x, py_bottom + 1))
				{
					const int16_t touching_tile_y = ((py_bottom + 1) / 8) * 8;
					l->head.y = INTTOFIX32(touching_tile_y) - 1;
					l->head.dy = 0;
					e->state = VYLE2_STATE_END3_LYLE_APPROACH;
				}
			}
			md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
			           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
			           SPR_SIZE(2, 3));
			obj_accurate_physics(&l->head);
			break;

		case VYLE2_STATE_END3_LYLE_APPROACH:
			do_lyle_angry_walk_hold_anim(e);
			obj_accurate_physics(&l->head);
			if (l->head.x >= INTTOFIX32(160 - 20))
			{
				e->state = VYLE2_STATE_END3_LYLE_HOP_UP;
			}
			md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
			           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
			           SPR_SIZE(2, 3));
			break;

		case VYLE2_STATE_END3_LYLE_HOP_UP:
			if (e->state_elapsed == 0)
			{
				lyle_set_anim_frame(4+8);
				l->head.dy = INTTOFIX16(PALSCALE_1ST((-20 / 5.0) * 5.0 / 6.0));
				sfx_play(SFX_JUMP, 0);
			}
			l->head.dy += INTTOFIX16(PALSCALE_2ND((1.0 / 5.0) * 5.0 / 6.0));
			if (l->head.x > INTTOFIX32(160))
			{
				l->head.x = INTTOFIX32(160);
			}
			if (l->head.dy > 0)
			{
				// TODO: Colliison with rocket, X distance check
				const int16_t px_x = FIX32TOINT(l->head.x + (l->head.right + l->head.left) / 2);
				const int16_t py_bottom = FIX32TOINT(l->head.y);
				if (map_collision(px_x, py_bottom + 1 + 24))
				{
					const int16_t touching_tile_y = ((py_bottom + 1) / 8) * 8;
					l->head.y = INTTOFIX32(touching_tile_y) - 1;
					l->head.dy = 0;
					l->head.dx = 0;
					e->state = VYLE2_STATE_END3_LYLE_TAKEOFF;
				}
			}
			md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
			           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
			           SPR_SIZE(2, 3));
			obj_accurate_physics(&l->head);
			break;

		case VYLE2_STATE_END3_LYLE_TAKEOFF:
			if (e->state_elapsed == 0)
			{
				lyle_set_anim_frame(8);
				l->head.dy = 0;
				l->head.dx = 0;
				md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
				           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
				           SPR_SIZE(2, 3));
			}
			else if (e->state_elapsed == 20)
			{
				// TODO: Play buzzing sound (same as second boss)
				md_spr_put(160-8, 168 - (system_is_ntsc() ? 16 : 0),
				           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
				           SPR_SIZE(2, 3));
			}
			else if (e->state_elapsed > PALSCALE_DURATION(20 * 6.0 / 5.0) &&
			         e->state_elapsed < PALSCALE_DURATION(60 * 6.0 / 5.0))
			{
				const int16_t x = 160 - 8 + (system_rand() % 5) - 2;
				const int16_t y = 168 + (system_rand() % 5) - 2 - (system_is_ntsc() ? 16 : 0);
				md_spr_put(x, y,
				           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
				           SPR_SIZE(2, 3));
			}
			else
			{
				md_spr_put(160-8, FIX32TOINT(l->head.y) + 1 - (system_is_ntsc() ? 16 : 0),
				           SPR_ATTR(s_vram_rocket_pos, 0, 0, ENEMY_PAL_LINE, 0),
				           SPR_SIZE(2, 3));
			}

			if (e->state_elapsed == PALSCALE_DURATION(55 * 6.0 / 5.0))
			{
				sfx_play(SFX_GIVER, 0);
			}
			else if (e->state_elapsed > PALSCALE_DURATION(55 * 6.0 / 5.0))
			{
				l->head.dy -= INTTOFIX16(PALSCALE_2ND((1 / 5.0) * (5.0 / 6.0) * (5.0 / 6.0)));
				// TODO: Launch blue fizzle particles downwards with randomization
				obj_accurate_physics(&l->head);
				if (l->head.y < INTTOFIX32(-64))
				{
					l->head.y = INTTOFIX32(-64);
					l->head.dy = 0;
				}
			}
			if (e->state_elapsed == PALSCALE_DURATION(120 * 6.0 / 5.0))
			{
				// To staff roll
				map_set_next_room(62, 0);
				map_set_exit_trigger(MAP_EXIT_RIGHT);
			}
			OBJ_SIMPLE_ANIM(e->lyle_anim_cnt, e->lyle_anim_frame, 2, PALSCALE_DURATION(2));
			break;

	}

	if (e->crumble_cnt > 0)
	{
		e->crumble_cnt--;
		particle_spawn(VYLE2_ARENA_CX - INTTOFIX32(144) + INTTOFIX32(system_rand() % (320 - 32)),
		               s_ground_y + INTTOFIX32(8), PARTICLE_TYPE_CRUMBLY);
	}

	if (state_prev != e->state) e->state_elapsed = 0;
	else e->state_elapsed++;
	obj_mixed_physics_h(o);

	map_set_x_scroll(FIX32TOINT(e->xscroll));
	map_set_y_scroll(FIX32TOINT(e->yscroll));

	render(e);
}

static void cube_func(Obj *o, Cube *c)
{
	O_Vyle2 *e = (O_Vyle2 *)o;

	if (e->state == VYLE2_STATE_VULNERABLE)
	{
		obj_standard_cube_response(o, c);
	}
	else
	{
		cube_destroy(c);
	}
}

void o_load_vyle2(Obj *o, uint16_t data)
{
	_Static_assert(sizeof(O_Vyle2) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	set_constants();
	vram_load();

	obj_basic_init(o, "Vyle 2", OBJ_FLAG_HARMFUL | OBJ_FLAG_TANGIBLE | OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-32), INTTOFIX16(32), INTTOFIX16(-64), 127);
	o->left = INTTOFIX16(-18);
	o->right = INTTOFIX16(18);
	o->top = INTTOFIX16(-48);
	o->main_func = main_func;
	o->cube_func = cube_func;

	lyle_set_scroll_h_en(0);
	lyle_set_scroll_v_en(0);
	map_set_y_scroll(0x28);

	O_Vyle2 *e = (O_Vyle2 *)o;

	if (data == 0)
	{
		e->first_state = VYLE2_STATE_LYLE_WALK_ANGRY;
	}
	else if (data == 1)
	{
		e->first_state = VYLE2_STATE_END1_FALL_REPEAT;
	}
	else if (data == 2)
	{
		e->first_state = VYLE2_STATE_END2_LYLE_RISE_UP;
	}
	else if (data == 3)
	{
		e->first_state = VYLE2_STATE_END3_LYLE_ENTER;
	}
	else
	{
		e->first_state = VYLE2_STATE_PRE_JUMP;
	}
}

void o_unload_vyle2(void)
{
	s_vram_pos = 0;
	s_vram_rocket_pos = 0;
}
