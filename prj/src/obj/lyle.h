#ifndef OBJ_LYLE_H
#define OBJ_LYLE_H

#include "obj.h"
#include "md/megadrive.h"
#include "cube.h"
#include <stdint.h>

#define LYLE_START_HP 5
#define LYLE_MAX_HP 15

#define LYLE_START_CP 5
#define LYLE_MAX_CP 30

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

	int8_t action_cnt;
	int8_t grounded;

	int8_t ext_disable;
	int8_t cube_jump_disable_cnt;

	int8_t cp;
	int8_t scroll_disable;
} O_Lyle;

void o_load_lyle(Obj *o, uint16_t data);
void o_unload_lyle(void);

// Public functions

O_Lyle *lyle_get(void);

void lyle_get_bounced(void);
void lyle_get_hurt(void);
void lyle_kill(void);

fix32_t lyle_get_x(void);
fix32_t lyle_get_y(void);
void lyle_set_pos(fix32_t x, fix32_t y);

int16_t lyle_get_hp(void);
int16_t lyle_get_cp(void);

void lyle_set_scroll_en(int16_t en);
void lyle_set_control_en(int16_t en);

#endif  // OBJ_LYLE_H
