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
	GFX_METAGRUB,

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

// Loads Gfx structure g into VRAM address load_pos.
// Returns the tile index at which data was loaded in VRAM.
uint16_t gfx_load(const Gfx *g, uint16_t load_pos);

#endif  // GFX_H
