// Palette data is assigned as follows:
// 00 [       Map tile       ]
// 16 [BG specific][BG common]
// 32 [         Enemy        ]
// 48 [         Lyle         ]

#include "res.h"

#define MAP_TILE_CRAM_POSITION 0
#define BG_CRAM_POSITION 16
#define BG_COMMON_CRAM_POSITION 24
#define ENEMY_CRAM_POSITION 32
#define LYLE_CRAM_POSITION 48

#define MAP_PAL_LINE (MAP_TILE_CRAM_POSITION / 16)
#define BG_PAL_LINE (BG_CRAM_POSITION / 16)
#define ENEMY_PAL_LINE (ENEMY_CRAM_POSITION / 16)
#define LYLE_PAL_LINE (LYLE_CRAM_POSITION / 16)

#define MAP_TILE_VRAM_POSITION 0x0000
#define MAP_TILE_VRAM_LENGTH   0x2000

#define BG_TILE_VRAM_POSITION 0x2000
#define BG_TILE_VRAM_LENGTH   0x0C00

#define HSCROLL_VRAM_POSITION 0x2C00
#define HSCROLL_VRAM_LENGTH 0x400

#define WINDOW_NT_VRAM_POSITION 0x3000
#define WINDOW_NT_VRAM_LENGTH 0x1000

#define PLANE_A_NT_VRAM_POSITION 0x4000
#define PLANE_A_NT_VRAM_LENGTH 0x2000

#define PLANE_B_NT_VRAM_POSITION 0x6000
#define PLANE_B_NT_VRAM_LENGTH 0x2000

#define SPRITE_LIST_VRAM_POSITION 0x8000
#define SPRITE_LIST_VRAM_LENGTH 0x280

// Fixed graphical tile locations begin here

#define VRAM_FREE_START (SPRITE_LIST_VRAM_POSITION + SPRITE_LIST_VRAM_LENGTH)

#define HUD_VRAM_POSITION VRAM_FREE_START
#define CUBE_VRAM_POSITION (HUD_VRAM_POSITION + sizeof(res_gfx_sys_hud_bin))

#define LYLE_VRAM_POSITION (CUBE_VRAM_POSITION + sizeof(res_gfx_sys_cubes_bin))
#define POWERUP_VRAM_POSITION (LYLE_VRAM_POSITION + (9*32))
#define PARTICLE_VRAM_POSITION (POWERUP_VRAM_POSITION + sizeof(res_gfx_sys_powerup_bin))
#define PROJECTILE_VRAM_POSITION (PARTICLE_VRAM_POSITION + sizeof(res_gfx_sys_particle_bin))
#define PAUSE_VRAM_POSITION (PROJECTILE_VRAM_POSITION + sizeof(res_gfx_sys_projectile_bin))

#define OBJ_TILE_VRAM_POSITION (PAUSE_VRAM_POSITION + sizeof(res_gfx_sys_pause_bin))
#define OBJ_TILE_VRAM_LENGTH (0x10000 - OBJ_TILE_VRAM_POSITION)


