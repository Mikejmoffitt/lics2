#include "obj/staffroll.h"
#include <stdlib.h>
#include "obj.h"
#include "system.h"
#include "gfx.h"
#include "md/megadrive.h"
#include "cube.h"
#include "palscale.h"
#include "map.h"
#include "hud.h"
#include "lyle.h"
#include "game.h"
#include <md/sys.h>

static int16_t s_v_scroll_buffer[GAME_SCREEN_W_CELLS / 2];

static uint16_t s_vram_pos;

static void vram_load(void)
{
	if (s_vram_pos) return;

	const Gfx *g = gfx_get(GFX_STAFFROLL);
	s_vram_pos = gfx_load(g, obj_vram_alloc(g->size));
}

// Lookup of ASCII characters to tiles
static const uint8_t kchar_tile_offset[256] =
{
	['0'] = 0x00,
	['1'] = 0x01,
	['2'] = 0x02,
	['3'] = 0x03,
	['4'] = 0x04,
	['5'] = 0x05,
	['6'] = 0x06,
	['7'] = 0x07,
	['8'] = 0x08,
	['9'] = 0x09,

	['A'] = 0x0A,
	['B'] = 0x0B,
	['C'] = 0x0C,
	['D'] = 0x0D,
	['E'] = 0x0E,
	['F'] = 0x0F,
	['G'] = 0x20,
	['H'] = 0x21,
	['I'] = 0x22,
	['J'] = 0x23,
	['K'] = 0x24,
	['L'] = 0x25,
	['M'] = 0x26,
	['N'] = 0x27,
	['O'] = 0x28,
	['P'] = 0x29,
	['Q'] = 0x2A,
	['R'] = 0x2B,
	['S'] = 0x2C,
	['T'] = 0x2D,
	['U'] = 0x2E,
	['V'] = 0x2F,
	['W'] = 0x40,
	['X'] = 0x41,
	['Y'] = 0x42,
	['Z'] = 0x43,

	['.'] = 0x44,
	[','] = 0x45,
	['!'] = 0x46,
	['?'] = 0x47,
	['('] = 0x48,
	[')'] = 0x49,
	['\"'] = 0x4A,
	[':'] = 0x4B,
	['&'] = 0x4C,
	['\''] = 0x4D,
	['-'] = 0x4E,

	['a'] = 0x0A,
	['b'] = 0x0B,
	['c'] = 0x0C,
	['d'] = 0x0D,
	['e'] = 0x0E,
	['f'] = 0x0F,
	['g'] = 0x20,
	['h'] = 0x21,
	['i'] = 0x22,
	['j'] = 0x23,
	['k'] = 0x24,
	['l'] = 0x25,
	['m'] = 0x26,
	['n'] = 0x27,
	['o'] = 0x28,
	['p'] = 0x29,
	['q'] = 0x2A,
	['r'] = 0x2B,
	['s'] = 0x2C,
	['t'] = 0x2D,
	['u'] = 0x2E,
	['v'] = 0x2F,
	['w'] = 0x40,
	['x'] = 0x41,
	['y'] = 0x42,
	['z'] = 0x43,

	[' '] = 0x4F,
};

static const char *kstaffroll_str[] =
{
	"CREDITS",
	"",
	"",
	"",
	"",
	"GRAPHICS:",
	"NIC DESTEFANO",
	"MIKE "MOF" MOFFITT",
	"",
	"",
	"",
	"",
	"GAME DESIGN:",
	"NIC DESTEFANO",
	"",
	"",
	"",
	"",
	"PROGRAMMING:",
	"MIKE "MOF" MOFFITT",
	"",
	"",
	"",
	"",
	"SOUND EFFECTS:",
	"MIKE "MOF" MOFFITT",
	"",
	"",
	"",
	"",
	"MUSIC:",
	"DOWNLOADED FROM MOD SITES",
	"(SEE MUSIC FILE FOR SONGS & ARTISTS)",
	"",
	"",
	"",
	"",
	"MUSIC ARRANGEMENT:",
	"MIKE "MOF" MOFFITT",
	"",
	"",
	"",
	"",
	"SPECIAL THANKS TO:",
	"",
	"COWS",
	"",
	"THE GOOD OL' NES",
	"",
	"THAT GUY",
	"",
	"ISOLATION FROM SOCIETY",
	"",
	"CLICKTEAM",
	"",
	"THE GOOD SEMI-OL' N64",
	"",
	"CHICKENCAKES MCGEE",
	"",
	"THE C PROGRAMMING LANGUAGE",
	"",
	"MOTOROLA 68000",
	"",
	"BIRDLAND",
	"",
	"KVCLAB",
	"",
	"",
	"",
	"",
	"NO THANKS TO:",
	"",
	"SCHOOL!!!",
	"",
	"THE ENSLAVEMENT OF CHILDREN",
	"",
	"SQUIRREL FECES",
	"",
	"GIANT INTERSTELLER SPACE BEINGS WHO",
	"DEVOUR PLANETS (SHAME ON YOU)",
	"",
	"PEOPLE WHO HAVE \"LIVES\"",
	"",
	"EVIL",
	"",
	"THE GNU ASSEMBLER SYNTAX",
	"",
	"",
	"",
	"TODAY'S GAME WAS BROUGHT TO YOU BY:",
	"",
	"THE LETTER 6",
	"",
	"THE NUMBER Q",
	"",
	"& 7 YEARS OF ON-AND-OFF DEVELOPMENT",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};

static void write_line(O_Staffroll *e)
{
	if (!e->line_ready) return;

	if (e->vram_write_addr >= md_vdp_get_plane_base(VDP_PLANE_A) + (GAME_PLANE_W_CELLS * 2 * GAME_PLANE_H_CELLS))
	{
		e->vram_write_addr = md_vdp_get_plane_base(VDP_PLANE_A);
	}

	if (e->line_idx >= ARRAYSIZE(kstaffroll_str))
	{
		md_vdp_set_autoinc(2);
		for (uint16_t half = 0; half < 2; half++)
		{
			md_vdp_set_addr(e->vram_write_addr);
			for (uint16_t i = 0; i < GAME_PLANE_W_CELLS * 2; i += 2)
			{
				md_vdp_write(0);
			}
			e->vram_write_addr += GAME_PLANE_W_CELLS * 2;
		}
		return;
	}

	const bool ints_en = md_sys_di();

	const char *str = kstaffroll_str[e->line_idx];
	const uint16_t x_offs = 2 * (20 - (strlen(str) / 2));

	md_vdp_set_autoinc(2);
	for (uint16_t half = 0; half < 2; half++)
	{
		// Clear out the line
		md_vdp_set_addr(e->vram_write_addr);
		for (uint16_t i = 0; i < GAME_PLANE_W_CELLS * 2; i += 2)
		{
			md_vdp_write(0);
		}
		// Write the string, centered
		md_vdp_set_addr(e->vram_write_addr + x_offs);
		const char *s = str;
		while (*s)
		{
			const uint16_t tile_offs = (uint16_t)*s;
			const uint16_t tile = s_vram_pos + kchar_tile_offset[tile_offs];
			md_vdp_write(VDP_ATTR(tile + (half ? 0x10 : 0x00), 0, 0, LYLE_PAL_LINE, 0));
			s++;
		}
		e->vram_write_addr += GAME_PLANE_W_CELLS * 2;
	}

	if (ints_en) md_sys_ei();

	e->line_idx++;
	e->line_ready = false;
}

// Store static constants here.

static inline void set_constants(void)
{
	static bool s_constants_set;
	if (s_constants_set) return;
	// Set constants here.

	s_constants_set = true;
}

static void scroll(O_Staffroll *e)
{
	for (uint16_t i = 0; i < ARRAYSIZE(s_v_scroll_buffer); i++)
	{
		s_v_scroll_buffer[i] = -FIX32TOINT(e->scroll);
	}

	const fix16_t scroll_mag = INTTOFIX16(PALSCALE_1ST(0.5 * 5.0 / 6.0));
	e->scroll -= scroll_mag;
	e->scroll_acc += scroll_mag;

	if (e->scroll_acc >= INTTOFIX16(16.0))
	{
		e->scroll_acc -= INTTOFIX16(16.0);
		e->line_ready = true;
	}

	// HACK: As long as Map hasn't been told the camera has moved, it won't
	// write to the scroll buffer in VRAM. We use this to move plane A.
	md_dma_transfer_vsram(0, s_v_scroll_buffer, sizeof(s_v_scroll_buffer) / 2, 4);
}


static void main_func(Obj *o)
{
	O_Staffroll *e = (O_Staffroll *)o;
	if (!e->initialized)
	{
		lyle_set_control_en(false);
		lyle_set_master_en(false);
		lyle_set_scroll_h_en(false);
		lyle_set_scroll_v_en(false);
		lyle_set_pos(INTTOFIX32(-32), INTTOFIX32(-32));
		hud_set_visible(false);
		e->initialized = true;
		md_dma_fill_vram(md_vdp_get_plane_base(VDP_PLANE_A), 0, GAME_PLANE_W_CELLS * GAME_PLANE_H_CELLS, 2);
	}

	scroll(e);
	write_line(e);
	if (e->line_idx >= ARRAYSIZE(kstaffroll_str))
	{
		map_set_next_room(63, 0);
		map_set_exit_trigger(MAP_EXIT_OTHER);
	}
}

void o_load_staffroll(Obj *o, uint16_t data)
{
	O_Staffroll *e = (O_Staffroll *)o;
	_Static_assert(sizeof(*e) <= sizeof(ObjSlot),
	               "Object size exceeds sizeof(ObjSlot)");
	(void)data;
	set_constants();
	vram_load();

	obj_basic_init(o, "Staffroll", OBJ_FLAG_ALWAYS_ACTIVE,
	               INTTOFIX16(-1), INTTOFIX16(1), INTTOFIX16(-2), 127);
	o->main_func = main_func;
	o->cube_func = NULL;

	e->vram_write_addr = md_vdp_get_plane_base(VDP_PLANE_A) + (GAME_PLANE_W_CELLS * 2 * 32);
}

void o_unload_staffroll(void)
{
	s_vram_pos = 0;
}
