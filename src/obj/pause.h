#ifndef OBJ_PAUSE_H
#define OBJ_PAUSE_H

#include "obj.h"

typedef enum PauseScreen
{
	PAUSE_SCREEN_NONE = 0,
	PAUSE_SCREEN_LYLE_WEAK,
	PAUSE_SCREEN_MAP,
	PAUSE_SCREEN_GET_MAP,
	PAUSE_SCREEN_GET_CUBE_LIFT,
	PAUSE_SCREEN_GET_CUBE_JUMP,
	PAUSE_SCREEN_GET_CUBE_KICK,
	PAUSE_SCREEN_GET_ORANGE_CUBE,
	PAUSE_SCREEN_GET_PHANTOM,
	PAUSE_SCREEN_GET_PHANTOM_DOUBLE_DAMAGE,
	PAUSE_SCREEN_GET_PHANTOM_HALF_TIME,
	PAUSE_SCREEN_GET_PHANTOM_CHEAP,
	PAUSE_SCREEN_HP_ORB_0,
	PAUSE_SCREEN_HP_ORB_1,
	PAUSE_SCREEN_HP_ORB_2,
	PAUSE_SCREEN_HP_ORB_3,
	PAUSE_SCREEN_HP_ORB_4,
	PAUSE_SCREEN_HP_ORB_5,
	PAUSE_SCREEN_HP_ORB_6,
	PAUSE_SCREEN_HP_ORB_7,
	PAUSE_SCREEN_HP_ORB_8,
	PAUSE_SCREEN_HP_ORB_9,
	PAUSE_SCREEN_HP_ORB_10,
	PAUSE_SCREEN_HP_ORB_11,
	PAUSE_SCREEN_HP_ORB_12,
	PAUSE_SCREEN_HP_ORB_13,
	PAUSE_SCREEN_HP_ORB_14,
	PAUSE_SCREEN_HP_ORB_15,
	PAUSE_SCREEN_CP_ORB_0,
	PAUSE_SCREEN_CP_ORB_1,
	PAUSE_SCREEN_CP_ORB_2,
	PAUSE_SCREEN_CP_ORB_3,
	PAUSE_SCREEN_CP_ORB_4,
	PAUSE_SCREEN_CP_ORB_5,
	PAUSE_SCREEN_CP_ORB_6,
	PAUSE_SCREEN_CP_ORB_7,
	PAUSE_SCREEN_CP_ORB_8,
	PAUSE_SCREEN_CP_ORB_9,
	PAUSE_SCREEN_CP_ORB_10,
	PAUSE_SCREEN_CP_ORB_11,
	PAUSE_SCREEN_CP_ORB_12,
	PAUSE_SCREEN_CP_ORB_13,
	PAUSE_SCREEN_CP_ORB_14,
	PAUSE_SCREEN_CP_ORB_15,
	PAUSE_SCREEN_DEBUG,
	PAUSE_SCREEN_ROOM_SELECT,
	PAUSE_SCREEN_SOUND_TEST,
	PAUSE_SCREEN_PROGRESS_EDIT,
} PauseScreen;

typedef struct O_Pause
{
	Obj head;
	MdButton buttons_prev;
	PauseScreen screen;
	PauseScreen screen_prev;

	int16_t cursor_flash_frame;
	int16_t cursor_flash_cnt;

	int16_t dismissal_delay_cnt;

	struct
	{
		int16_t main_cursor;
		int16_t room_cursor;
		int16_t room_last_page;
		int16_t chosen_room_id;

		int16_t sound_cursor;
		uint8_t bgm_id;
		uint8_t sfx_id;

		int16_t progress_cursor_main;
		int16_t progress_cursor_bit;
	} debug;

	int16_t paused;
	int16_t window;  // nonzero if window should be shown.
} O_Pause;

void pause_set_screen(PauseScreen screen);

int16_t pause_want_window(void);
int16_t pause_get_debug_room_id(void);

void o_load_pause(Obj *o, uint16_t data);
void o_unload_pause(void);


#endif  // OBJ_PAUSE_H
