#ifndef OBJ_SECRETTV_H
#define OBJ_SECRETTV_H

#include "obj.h"
#include <stdbool.h>

typedef struct SecretTvFrame
{
	uint16_t code;
	uint16_t pal;
} SecretTvFrame;

typedef struct SecretTvAnim
{
	const SecretTvFrame *frames;
	uint16_t len;
	uint16_t loop;
	uint16_t delay;
} SecretTvAnim;

typedef struct O_SecretTv
{
	Obj head;
	const SecretTvAnim *anim;
	uint16_t anim_frame;
	uint16_t anim_cnt;
	uint16_t anim_delay;
} O_SecretTv;

void o_load_secrettv(Obj *o, uint16_t data);
void o_unload_secrettv(void);

#endif  // OBJ_SECRETTV_H
