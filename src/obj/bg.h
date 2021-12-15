#ifndef OBJ_BG_H
#define OBJ_BG_H

#include "obj.h"

typedef struct BgDescriptor
{
	GfxId gfx_id;
	const uint8_t *palette;
	const uint8_t *mapping;
	uint16_t mapping_size;
} BgDescriptor;

typedef struct O_Bg
{
	Obj head;
	uint8_t bg_id;
	int16_t x_scroll;
	int16_t y_scroll;
	int16_t last_x_scroll;
	int16_t last_y_scroll;
	const BgDescriptor *descriptor;
} O_Bg;

void o_load_bg(Obj *o, uint16_t data);
void o_unload_bg(void);

void bg_upload_palette(void);

#endif  // OBJ_BG_H
