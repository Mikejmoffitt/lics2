#ifndef OBJ_LAVAANIM_H
#define OBJ_LAVAANIM_H

#include "obj.h"

typedef enum LavaAnimVariant
{
	LAVA_ANIM_REGULAR,
	LAVA_ANIM_GREEN,
	LAVA_ANIM_TECHNO,
	LAVA_ANIM_ROOFTOP,
} LavaAnimVariant;

typedef struct LavaAnimInfo
{
	const uint8_t *pal;
	uint16_t pal_size;
	uint16_t src_data_offset;
	uint16_t src_anim_offset;
	uint16_t dest_vram_offset;
	uint16_t line_transfer_words;
} LavaAnimInfo;

typedef struct O_LavaAnim
{
	Obj head;

	int16_t anim_frame;
	int16_t anim_cnt;

	LavaAnimInfo info;
} O_LavaAnim;

void o_load_lavaanim(Obj *o, uint16_t data);
void o_unload_lavaanim(void);

#endif  // OBJ_LAVAANIM_H
