#include "obj/elevator.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"

#include "lyle.h"
#include "obj/elevator_stop.h"
#include "game.h"
#include "sfx.h"
#include "input.h"

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_ELEVATOR);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

static fix16_t kmove_dy;
static int16_t kanim_speed;
static int16_t kcollision_delay_frames;

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;

	kmove_dy = INTTOFIX16(PALSCALE_1ST(1.666666667));
	kanim_speed = PALSCALE_DURATION(8);
	kcollision_delay_frames = PALSCALE_DURATION(60);

	s_constants_set = true;
}

static void render(O_Elevator *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -16;
	static const int16_t offset_y = -56;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	const int16_t anim_offset = e->anim_frame ? 32 : 0;
	md_spr_put(sp_x, sp_y, SPR_ATTR(s_vram_pos + anim_offset, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
	md_spr_put(sp_x, sp_y + 32, SPR_ATTR(s_vram_pos + 16 + anim_offset, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(4, 4));
}

static void render_arrows(O_Elevator *e)
{
	Obj *o = &e->head;
	int16_t sp_x, sp_y;
	static const int16_t offset_x = -8;
	static const int16_t offset_y = -56;
	obj_render_setup(o, &sp_x, &sp_y, offset_x, offset_y,
	                 map_get_x_scroll(), map_get_y_scroll());

	const int16_t anim_offset = e->anim_frame ? 68 : 64;
	md_spr_put(sp_x - 24, sp_y + 2, SPR_ATTR(s_vram_pos + anim_offset, 0, 0,
	                         ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	md_spr_put(sp_x + 24, sp_y + 2, SPR_ATTR(s_vram_pos + anim_offset, 0, 0,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));

	md_spr_put(sp_x - 24, sp_y + 27, SPR_ATTR(s_vram_pos + anim_offset, 0, 1,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
	md_spr_put(sp_x + 24, sp_y + 27, SPR_ATTR(s_vram_pos + anim_offset, 0, 1,
	                             ENEMY_PAL_LINE, 0), SPR_SIZE(2, 2));
}

// Returns an elevator stop pointer if currently touching one.
static O_ElevatorStop *find_current_stop(Obj *o)
{
	ObjSlot *s = &g_objects[0];
	while (s < &g_objects[OBJ_COUNT_MAX])
	{
		Obj *other = (Obj *)s;
		s++;
		if (other->status != OBJ_STATUS_ACTIVE) continue;
		if (other->type != OBJ_ELEVATOR_STOP) continue;

		// If in range of a stop, stop upon faulting vertical position.
		if (obj_touching_obj(o, other))
		{
			return (O_ElevatorStop *)other;
		}
	}
	return NULL;
}

static void hide_all_other_elevators(Obj *o, int16_t hidden)
{
	ObjSlot *s = &g_objects[0];
	while (s < &g_objects[OBJ_COUNT_MAX])
	{
		Obj *other = (Obj *)s;
		s++;
		if (other->status != OBJ_STATUS_ACTIVE) continue;
		if (other->type != OBJ_ELEVATOR) continue;
		if (other == o) continue;
		O_Elevator *other_e = (O_Elevator *)other;
		other_e->hidden = hidden;
	}
}

static void main_func(Obj *o)
{
	O_Elevator *e = (O_Elevator *)o;

	if (o->dy != 0)
	{
		obj_standard_physics(o);

		if (e->collision_delay_cnt == 0)
		{
			// Check against elevator stops.
			O_ElevatorStop *stop = find_current_stop(o);
			// If in range of a stop, stop upon faulting vertical position.
			if (stop &&
			    ((o->y <= stop->head.y && o->dy < 0) ||
			     (o->y >= stop->head.y && o->dy > 0)))
			{
				o->dy = 0;

				// The stopped elevator is positioned at the resting place
				// just to do a quick early render, to cover up for the
				// other elevator that may be hidden for one frame.
				o->y = stop->head.y;

				lyle_set_master_en(1);
				sfx_stop(SFX_ELEVATOR);
				sfx_stop(SFX_ELEVATOR_2);
				hide_all_other_elevators(o, 0);
			}
		}
		else
		{
			e->collision_delay_cnt--;
		}

		int16_t py;
		if (map_get_bottom() <= INTTOFIX32(240))
		{
			py = (system_is_ntsc() ? 8 : 0);
		}
		else
		{
			py = FIX32TOINT(o->y);
			const int16_t top_bound = GAME_SCREEN_H_PIXELS / 2;
			py -= top_bound;
		}

		lyle_set_pos(lyle_get_x(), o->y - INTTOFIX32(9));
		map_set_y_scroll(py - 9);

		// A stopped elevator is returned to its original y after
		// Lyle has been positioned, and early returns so that it
		// is not rendered (in violation of the on-screen check).
		if (o->dy == 0)
		{
			render(e);
			o->y = e->original_y;
			return;
		}
	}
	else if (!e->hidden)
	{
		static const fix32_t x_margin = INTTOFIX32(8);
		if (lyle_touching_obj(o) &&
		    lyle_get_x() > o->x - x_margin &&
		    lyle_get_x() < o->x + x_margin )
		{

			const O_Lyle *l = lyle_get();
			const int16_t lyle_is_idle = !(
			    l->hurt_cnt ||
			    l->tele_out_cnt ||
			    l->tele_in_cnt ||
			    l->invuln_cnt ||
			    l->phantom_cnt ||
			    l->throwdown_cnt ||
			    l->throw_cnt ||
			    l->kick_cnt ||
			    l->lift_cnt ||
			    l->action_cnt ||
			    l->holding_cube);

			if (l->grounded && lyle_is_idle)
			{
				render_arrows(e);
				if (l->head.dx == 0)
				{
					O_ElevatorStop *stop = find_current_stop(o);

					const int16_t up_ok = (stop && stop->id != 0x0000);
					const int16_t down_ok = (stop && stop->id != 0x0002);

					const LyleBtn buttons = input_read();
					const int16_t up_req = ((buttons & LYLE_BTN_UP) && up_ok);
					const int16_t down_req = ((buttons & LYLE_BTN_DOWN) && down_ok);
					if (up_req || down_req)
					{
						o->dy = (up_req) ? -kmove_dy : kmove_dy;
						lyle_set_master_en(0);
						e->collision_delay_cnt = kcollision_delay_frames;
						sfx_stop_all();
						sfx_play(SFX_ELEVATOR, 0);
						sfx_play(SFX_ELEVATOR_2, 0);
						hide_all_other_elevators(o, 1);
					}
				}
			}
		}
	}

	OBJ_SIMPLE_ANIM(e->anim_cnt, e->anim_frame, 2, kanim_speed);

	if (!e->hidden) render(e);

	// Upload the alternate Elevator enemy palette to the enemy pal line.
	md_pal_upload(ENEMY_CRAM_POSITION, res_pal_enemy_elevator_bin,
	           sizeof(res_pal_enemy_elevator_bin) / 2);
}

static void cube_func(Obj *o, Cube *c)
{
	if (c->y < o->y - INTTOFIX32(32)) obj_standard_cube_response(o, c);
}

void o_load_elevator(Obj *o, uint16_t data)
{
	O_Elevator *e = (O_Elevator *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");

	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Elevator", 0,
	               INTTOFIX16(-16), INTTOFIX16(16), INTTOFIX16(-56), 127);
	o->main_func = main_func;
	o->cube_func = cube_func;
	e->original_y = o->y;
}

void o_unload_elevator(void)
{
	s_vram_pos = 0;
}
