#ifndef GFX_H
#define GFX_H

#include <stdint.h>

typedef enum GfxId
{
	GFX_LYLE,
	GFX_CUBES,
	GFX_TEMPLATE,

} GfxId;

typedef struct Gfx
{
	const void *data;
	const uint16_t size;
} Gfx;

int gfx_init(void);
const Gfx *gfx_get(GfxId id);
void gfx_load(const Gfx *g, uint16_t vram_pos);

#endif  // GFX_H
