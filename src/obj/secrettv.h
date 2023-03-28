#ifndef OBJ_SECRETTV_H
#define OBJ_SECRETTV_H

#include "obj.h"
#include <stdbool.h>

typedef struct O_SecretTv
{
	Obj head;
	uint16_t id;
	bool active;
	uint16_t anim_frame;
	uint16_t anim_cnt;
	uint16_t attr;
} O_SecretTv;

void o_load_secrettv(Obj *o, uint16_t data);
void o_unload_secrettv(void);

#endif  // OBJ_SECRETTV_H
