#include "gfx.h"
#include "md/megadrive.h"

#include "res.h"

#define GFX(name) { res_gfx_##name##_bin, sizeof(res_gfx_##name##_bin) }

static const Gfx gfx[] =
{
	[GFX_METAGRUB] = GFX(obj_3_metagrub),

	[GFX_LYLE] = GFX(obj_128_lyle),
	[GFX_CUBES] = GFX(obj_129_cubes),

	[GFX_TEMPLATE] = GFX(obj_255_template),
};

int gfx_init(void)
{
	return 1;
}

const Gfx *gfx_get(GfxId id)
{
	return &gfx[id];
}

uint16_t gfx_load(const Gfx *g, uint16_t load_pos)
{
	// DMA operates in terms of words rather than bytes
	const uint16_t transfer_words = g->size / 2;
	dma_q_transfer_vram(load_pos, (void *)g->data, transfer_words, 2);
	return load_pos / 32;
}

