#ifndef OBJ_GAMEOVER_H
#define OBJ_GAMEOVER_H

#include "obj.h"
#include "input.h"

typedef enum __attribute__((packed)) GameOverState
{
	GAMEOVER_LYLE_FALL,
	GAMEOVER_LYLE_ANIM,
	GAMEOVER_CUBE_FALL,
	GAMEOVER_CUBE_BOUNCE,
	GAMEOVER_CUBE_MENU,
} GameOverState;

typedef struct O_GameOver
{
	Obj head;

	fix32_t max_y;

	int16_t state_elapsed;

	int16_t lyle_anim_cnt;
	uint8_t lyle_anim_frame;
	int8_t lyle_metaframe;

	uint8_t flash_cnt;
	uint8_t flash_frame;

	int8_t menu_choice;
	int8_t choose_cnt;

	LyleBtn buttons_prev;
	GameOverState state;
} O_GameOver;

void o_load_gameover(Obj *o, uint16_t data);
void o_unload_gameover(void);

#endif  // OBJ_GAMEOVER_H
