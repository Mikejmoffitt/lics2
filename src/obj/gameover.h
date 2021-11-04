#ifndef OBJ_GAMEOVER_H
#define OBJ_GAMEOVER_H

#include "obj.h"
#include "md/megadrive.h"

typedef enum GameOverState
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

	GameOverState state;
	int16_t state_elapsed;

	fix32_t lyle_y;
	fix16_t lyle_dy;

	int16_t lyle_anim_cnt;
	uint16_t lyle_anim_frame;
	int16_t lyle_metaframe;

	int16_t flash_cnt;
	int16_t flash_frame;

	int16_t menu_choice;
	int16_t choose_cnt;

	MdButton buttons_prev;
} O_GameOver;

void o_load_gameover(Obj *o, uint16_t data);
void o_unload_gameover(void);

#endif  // OBJ_GAMEOVER_H
