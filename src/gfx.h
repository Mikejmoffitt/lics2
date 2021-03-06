#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "vram_map.h"

typedef enum GfxId
{
	GFX_NULL,
	GFX_LYLE,
	GFX_CUBES,
	GFX_PAUSE,
	GFX_TEMPLATE,

	GFX_METAGRUB,
	GFX_FLIP,
	GFX_BOINGO,
	GFX_ITEM,
	GFX_GAXTER,
	GFX_BUGGO,
	GFX_DANCYFLOWER,
	GFX_JRAFF,
	GFX_PILLA,
	GFX_HEDGEDOG,
	GFX_SHOOT,
	GFX_LASER,
	GFX_KILLZAM,
	GFX_FLARGY,
	GFX_PLANT,
	GFX_TOSSMUFFIN,
	GFX_TELEPORTER,
	GFX_MAGIBEAR,
	GFX_LAVA,
	GFX_COW,
	GFX_CONTAINER,
	GFX_HOOP,
	GFX_FALSEBLOCK,
	GFX_CP_PAD,
	GFX_CP_METER,
	GFX_DOG,
	GFX_ELEVATOR,
	GFX_ELEVATOR_STOP,
	GFX_FISSINS1,
	GFX_BOSS1,
	GFX_FISSINS2,
	GFX_LAVAANIM,

	GFX_PURPLETREE,
	GFX_WNDWBACK,
	GFX_TITLE_SCR,
	GFX_BOGOLOGO,

	GFX_EX_ITEMS,
	GFX_EX_PROJECTILES,
	GFX_EX_PARTICLES,
	GFX_EX_HUD,
	GFX_EX_POWERUPS,
	GFX_EX_CREDITS,
	GFX_EX_KEDDUMS_INTRO,
	GFX_EX_CLOAKDUDE,
	GFX_EX_GRASSES,
	GFX_EX_COLUMNS,
	GFX_EX_TITLE_MENU,

	GFX_BG_1,
	GFX_BG_2,
	GFX_BG_3,
	GFX_BG_4,
	GFX_BG_5,
	GFX_BG_6,  // 6 is a mapping mod of 1.
	GFX_BG_7,
	GFX_BG_7_EX,
	GFX_BG_8,
	GFX_BG_9,
	GFX_BG_10,
	GFX_BG_11,
	GFX_BG_12, // 12 is a mod of 10.
	GFX_BG_13,
	GFX_BG_14, // 14 is a mod of 13.
	GFX_BG_15,
	GFX_BG_16,
	GFX_BG_16_EX,
	GFX_BG_18,
	GFX_BG_19,
	GFX_BG_22,
	GFX_BG_23,
	GFX_BG_23_EX,
} GfxId;

typedef struct Gfx
{
	const uint8_t *data;
	const uint16_t size;
} Gfx;

int gfx_init(void);
const Gfx *gfx_get(GfxId id);

// Loads Gfx structure g into VRAM address load_pos.
// Returns the tile index at which data was loaded in VRAM.
uint16_t gfx_load(const Gfx *g, uint16_t load_pos);

#endif  // GFX_H
