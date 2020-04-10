#ifndef GFX_H
#define GFX_H

#include <stdint.h>

// Palette data is assigned as follows:
// +00 [       Map tile       ]
// +16 [BG specific][BG common]
// +32 [         Enemy        ]
// +48 [         Lyle         ]

#define MAP_TILE_CRAM_POSITION 0
#define BG_CRAM_POSITION 16
#define BG_COMMON_CRAM_POSITION 24
#define ENEMY_CRAM_POSITION 32
#define LYLE_CRAM_POSITION 48

#define BG_PAL_LINE (BG_CRAM_POSITION / 16)
#define ENEMY_PAL_LINE (ENEMY_CRAM_POSITION / 16)
#define LYLE_PAL_LINE (LYLE_CRAM_POSITION / 16)

#define MAP_TILE_VRAM_POSITION 0

typedef enum GfxId
{
	GFX_LYLE,
	GFX_CUBES,
	GFX_TEMPLATE,
	// These entries directly correspond to objects.
	GFX_METAGRUB,
	GFX_ENTRANCE,
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

	GFX_EX_ITEMS,
	GFX_EX_PROJECTILES,
	GFX_EX_PARTICLES,

	GFX_TITLE_BOGOLOGO,
	GFX_TITLE_TITLELOGO,
} GfxId;

typedef struct Gfx
{
	const void *data;
	const uint16_t size;
} Gfx;

int gfx_init(void);
const Gfx *gfx_get(GfxId id);

// Loads Gfx structure g into VRAM address load_pos.
// Returns the tile index at which data was loaded in VRAM.
uint16_t gfx_load(const Gfx *g, uint16_t load_pos);

#endif  // GFX_H
