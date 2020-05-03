#include "gfx.h"
#include "md/megadrive.h"

#include "res.h"

#define GFX(name) { res_gfx_##name##_bin, sizeof(res_gfx_##name##_bin) }

static const Gfx gfx[] =
{
	[GFX_LYLE] = GFX(obj_128_lyle),
	[GFX_CUBES] = GFX(obj_129_cubes),
	[GFX_TEMPLATE] = GFX(obj_255_template),

	[GFX_ENTRANCE] = GFX(obj_1_entrance),
	[GFX_METAGRUB] = GFX(obj_3_metagrub),
	[GFX_FLIP] = GFX(obj_4_flip),
	[GFX_BOINGO] = GFX(obj_5_boingo),
	[GFX_ITEM] = GFX(obj_6_item),
	[GFX_GAXTER] = GFX(obj_7_gaxter),
	[GFX_BUGGO] = GFX(obj_9_buggo),
	[GFX_DANCYFLOWER] = GFX(obj_11_dancyflower),
	[GFX_JRAFF] = GFX(obj_12_jraff),
	[GFX_PILLA] = GFX(obj_13_pilla),
	[GFX_HEDGEDOG] = GFX(obj_14_hedgedog),
	[GFX_SHOOT] = GFX(obj_15_shoot),
	[GFX_LASER] = GFX(obj_16_laser),
	[GFX_KILLZAM] = GFX(obj_17_killzam),
	[GFX_FLARGY] = GFX(obj_18_flargy),
	[GFX_PLANT] = GFX(obj_19_plant),
	[GFX_TOSSMUFFIN] = GFX(obj_20_tossmuffin),
	[GFX_TELEPORTER] = GFX(obj_21_teleporter),
	[GFX_MAGIBEAR] = GFX(obj_22_magibear),
	[GFX_LAVA] = GFX(obj_23_lava),
	[GFX_COW] = GFX(obj_24_cow),
	[GFX_ELEVATOR] = GFX(obj_31_elevator),
	[GFX_FISSINS1] = GFX(obj_33_fissins1),
	[GFX_BOSS1] = GFX(obj_34_boss1),
	[GFX_FISSINS2] = GFX(obj_39_fissins2),
	[GFX_LAVAANIM] = GFX(obj_43_lavaanim),

	[GFX_EX_ITEMS] = GFX(ex_items),
	[GFX_EX_PROJECTILES] = GFX(ex_projectiles),
	[GFX_EX_PARTICLES] = GFX(ex_particles),

	[GFX_TITLE_BOGOLOGO] = GFX(title_bogologo),
	[GFX_TITLE_TITLELOGO] = GFX(title_titlelogo),

	[GFX_BG_1] = GFX(bg_bg1),
	[GFX_BG_2] = GFX(bg_bg2),
	[GFX_BG_3] = GFX(bg_bg3),
	[GFX_BG_4] = GFX(bg_bg4),
	[GFX_BG_5] = GFX(bg_bg5),
	[GFX_BG_6] = GFX(bg_bg1),
	[GFX_BG_7] = GFX(bg_bg7),
	[GFX_BG_9] = GFX(bg_bg9),
	[GFX_BG_10] = GFX(bg_bg10),
	[GFX_BG_11] = GFX(bg_bg11),
	[GFX_BG_12] = GFX(bg_bg10),
	[GFX_BG_13] = GFX(bg_bg13),
	[GFX_BG_14] = GFX(bg_bg13),
	[GFX_BG_15] = GFX(bg_bg15),
	[GFX_BG_16] = GFX(bg_bg16),
	[GFX_BG_16_EX] = GFX(bg_bg16_ex),

	[GFX_BG_19] = GFX(bg_bg19),
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
	if (!g->data) return 0;
	// DMA operates in terms of words rather than bytes
	const uint16_t transfer_words = g->size / 2;
	dma_q_transfer_vram(load_pos, (void *)g->data, transfer_words, 2);
	return load_pos / 32;
}

