#include "gfx.h"
#include "md/megadrive.h"

#include "res.h"

#define GFX(name) { res_gfx_##name##_bin, sizeof(res_gfx_##name##_bin) }

static const Gfx gfx[] =
{
	GFX(lyle),
	GFX(template),
};

int gfx_init(void)
{
	return 1;
}

const Gfx *gfx_get(GfxId id)
{
	return &gfx[id];
}

void gfx_load(const Gfx *g, uint16_t vram_pos)
{
	dma_q_transfer_vram(vram_pos, (void *)g->data, g->size, 2);
}

