#include "gfx.h"
#include "md/megadrive.h"

#include "res.h"

#define GFX(name) { res_gfx_##name##_bin, sizeof(res_gfx_##name##_bin) }

static const Gfx gfx[] =
{
	[GFX_SYS_CUBES] = GFX(sys_cubes),
	[GFX_SYS_PAUSE] = GFX(sys_pause),
	[GFX_SYS_LYLE] = GFX(sys_lyle),
	[GFX_SYS_HUD] = GFX(sys_hud),
	[GFX_SYS_POWERUP] = GFX(sys_powerup),
	[GFX_SYS_PROJECTILE] = GFX(sys_projectile),
	[GFX_SYS_PARTICLE] = GFX(sys_particle),
	[GFX_SYS_KANA_FONT] = GFX(sys_kana_font),

	[GFX_TEMPLATE] = GFX(obj_255_template),

	[GFX_METAGRUB] = GFX(obj_3_metagrub),
	[GFX_FLIP] = GFX(obj_4_flip),
	[GFX_BOINGO] = GFX(obj_5_boingo),

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
	[GFX_BALL] = GFX(obj_25_ball),
	[GFX_HOOP] = GFX(obj_26_hoop),
	[GFX_CP_GIVER] = GFX(obj_28_cp_giver),
	[GFX_CP_METER] = GFX(obj_29_cp_meter),
	[GFX_DOG] = GFX(obj_30_dog),
	[GFX_ELEVATOR] = GFX(obj_31_elevator),
	[GFX_FISSINS1] = GFX(obj_33_fissins1),
	[GFX_BOSS1] = GFX(obj_34_boss1),
	[GFX_BOSS2] = GFX(obj_35_boss2),
	[GFX_VYLE1] = GFX(obj_36_vyle1),
	[GFX_VYLE2] = GFX(obj_37_vyle2),
	[GFX_EGG] = GFX(obj_38_egg),
	[GFX_FISSINS2] = GFX(obj_39_fissins2),

	[GFX_SMALL_EGG] = GFX(obj_41_small_egg),
	[GFX_BASKETBALL] = GFX(obj_42_basketball),
	[GFX_LAVAANIM] = GFX(obj_43_lavaanim),
	[GFX_SPOOKO] = GFX(obj_44_spooko),

	[GFX_RADIO] = GFX(obj_48_radio),
	[GFX_CHIMNEY] = GFX(obj_49_chimney),
	[GFX_CORK] = GFX(obj_50_cork),
	[GFX_BROKEN_EGG] = GFX(obj_51_broken_egg),
	[GFX_CHICK] = GFX(obj_52_chick),

	[GFX_ROCKMAN_DOOR] = GFX(obj_54_rockman_door),
	[GFX_KEDDUMS] = GFX(obj_56_keddums),
	[GFX_TVSCREEN] = GFX(obj_57_tvscreen),
	[GFX_POINTYSIGN] = GFX(obj_59_pointysign),
	[GFX_CLOAK] = GFX(obj_60_cloak),
	[GFX_BIGEXPLOSION] = GFX(obj_61_bigexplosion),

	[GFX_STAFFROLL] = GFX(obj_117_staffroll),
	[GFX_GAMEOVER] = GFX(obj_118_gameover),
	[GFX_PURPLETREE] = GFX(obj_123_purpletree),
	[GFX_WNDWBACK] = GFX(obj_124_wndwback),

	[GFX_TITLE_SCR] = GFX(obj_126_title_scr),
	[GFX_BOGOLOGO] = GFX(obj_127_bogologo),

	[GFX_EX_CREDITS] = GFX(ex_credits),
	[GFX_EX_KEDDUMS_INTRO] = GFX(ex_keddums_intro),
	[GFX_EX_CLOAKDUDE] = GFX(ex_cloakdude),
	[GFX_EX_GRASSES] = GFX(ex_grasses),
	[GFX_EX_COLUMNS] = GFX(ex_columns),
	[GFX_EX_TECHNOBGH] = GFX(ex_technobgh),
	[GFX_EX_TECHNOBGV] = GFX(ex_technobgv),
	[GFX_EX_CITYBG] = GFX(ex_citybg),
	[GFX_EX_TITLE_MENU] = GFX(ex_title_menu),
	[GFX_EX_VYLE2_GROUND] = GFX(ex_vyle2_ground),
	[GFX_EX_PWAVE_MARQUEE] = GFX(ex_pwave_marquee),
	[GFX_EX_PWAVE_SCREENBACK] = GFX(ex_pwave_screenback),
	[GFX_EX_PWAVE_CATFACE] = GFX(ex_pwave_catface),
	[GFX_EX_PWAVE_LIGHTR] = GFX(ex_pwave_lightr),
	[GFX_EX_PWAVE_LIGHTG] = GFX(ex_pwave_lightg),
	[GFX_EX_PWAVE_ARM] = GFX(ex_pwave_arm),
	[GFX_EX_PWAVE_GLASSBALL] = GFX(ex_pwave_glassball),
	[GFX_EX_PWAVE_BUTTON] = GFX(ex_pwave_button),
	[GFX_EX_ROCKET] = GFX(ex_rocket),
	[GFX_EX_SECRETTV] = GFX(ex_secrettv),

	[GFX_BG_1] = GFX(bg_bg1),
	[GFX_BG_2] = GFX(bg_bg2),
	[GFX_BG_3] = GFX(bg_bg3),
	[GFX_BG_4] = GFX(bg_bg4),
	[GFX_BG_5] = GFX(bg_bg5),
	[GFX_BG_6] = GFX(bg_bg1),
	[GFX_BG_7] = GFX(bg_bg7),
	[GFX_BG_7_EX] = GFX(bg_bg7_ex),
	[GFX_BG_8] = GFX(bg_bg8),
	[GFX_BG_9] = GFX(bg_bg9),
	[GFX_BG_10] = GFX(bg_bg10),
	[GFX_BG_11] = GFX(bg_bg11),
	[GFX_BG_12] = GFX(bg_bg10),
	[GFX_BG_13] = GFX(bg_bg13),
	[GFX_BG_14] = GFX(bg_bg13),
	[GFX_BG_15] = GFX(bg_bg15),
	[GFX_BG_16] = GFX(bg_bg16),
	[GFX_BG_16_EX] = GFX(bg_bg16_ex),
	[GFX_BG_18] = GFX(bg_bg18),
	[GFX_BG_19] = GFX(bg_bg19),
	[GFX_BG_22] = GFX(bg_bg22),
	[GFX_BG_23] = GFX(bg_bg23),
	[GFX_BG_23_EX] = GFX(bg_bg23_ex),
	[GFX_BG_24] = GFX(bg_bg24),
	[GFX_BG_26] = GFX(bg_bg26),
	[GFX_BG_27] = GFX(bg_bg27),
};

int gfx_init(void)
{
	return 1;
}

const Gfx *gfx_get(GfxId id)
{
	return &gfx[id];
}

uint16_t gfx_load_ex(const Gfx *g, uint16_t start, uint16_t size, uint16_t load_pos)
{
	if (!g->data) return 0;
	if (size > g->size) size = g->size;
//	// DMA operates in terms of words rather than bytes
	const uint16_t transfer_words = size / 2;
	const uint8_t *gdata = (const uint8_t *)g->data;
	md_dma_transfer_vram(load_pos, &gdata[start], transfer_words, 2);
	return load_pos / 32;
}

uint16_t gfx_load(const Gfx *g, uint16_t load_pos)
{
	if (!g->data) return 0;
	return gfx_load_ex(g, 0, g->size, load_pos);
}
