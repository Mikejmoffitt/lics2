#ifndef OBJ_LYLE_H
#define OBJ_LYLE_H

#include "obj.h"
#include "md/megadrive.h"
#include "cube.h"
#include <stdint.h>
#include <stdbool.h>

#define LYLE_START_HP 5
#define LYLE_MAX_HP 15

#define LYLE_START_CP 15
#define LYLE_MAX_CP 30

// Hitboxes
#define LYLE_LEFT INTTOFIX16(-4)
#define LYLE_RIGHT INTTOFIX16(4)
#define LYLE_TOP INTTOFIX16(-19)

// Lyle is not in the object list, but is an object for easier interaction.
typedef struct O_Lyle
{
	Obj head;

	Cube *on_cube;
	CubeType holding_cube;

	int16_t hurt_cnt;
	int16_t tele_out_cnt;
	int16_t tele_in_cnt;
	int16_t invuln_cnt;
	int16_t cp_restore_cnt;
	int16_t phantom_cnt;
	int16_t throwdown_cnt;
	int16_t throw_cnt;
	int16_t kick_cnt;
	int16_t lift_cnt;

	int8_t anim_cnt;
	int8_t anim_frame;
	int8_t metaframe;

	int8_t action_cnt;
	int8_t grounded;

	int8_t ext_disable;
	int8_t cube_jump_disable_cnt;

	int8_t cp;
	int8_t scroll_disable_h;
	int8_t scroll_disable_v;
	int8_t priority;

	int8_t full_disable;
	int8_t dead;
} O_Lyle;

// TODO: Params for persistent state
void lyle_init(void);
void lyle_poll(void);

O_Lyle *lyle_get(void);

bool lyle_touching_obj(Obj *o);

void lyle_get_bounced(void);
void lyle_get_hurt(bool bypass_invuln);

fix32_t lyle_get_x(void);
fix32_t lyle_get_y(void);
void lyle_set_pos(fix32_t x, fix32_t y);
void lyle_set_direction(ObjDirection d);

int16_t lyle_get_hp(void);
void lyle_set_hp(int16_t hp);
int16_t lyle_get_cp(void);
static inline void lyle_kill(void) { lyle_set_hp(0); }

void lyle_set_scroll_h_en(int16_t en);
void lyle_set_scroll_v_en(int16_t en);
void lyle_set_control_en(int16_t en);
void lyle_set_master_en(int16_t en);
void lyle_set_anim_frame(int8_t frame);

void lyle_upload_palette(void);

void lyle_set_hibernate(bool en);

#endif  // OBJ_LYLE_H
