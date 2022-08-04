#include "system.h"
#include "md/megadrive.h"
#include "vram_map.h"

int8_t g_system_is_ntsc;
int8_t g_system_debug_enabled;

static uint32_t s_rand_value;

void system_init(void)
{
	megadrive_init();
	g_system_is_ntsc = (md_vdp_get_status() & VDP_STATUS_PAL) ? 0 : 1;
	md_vdp_set_plane_size(VDP_PLANESIZE_64x64);
	md_vdp_set_plane_base(VDP_PLANE_A, PLANE_A_NT_VRAM_POSITION);
	md_vdp_set_plane_base(VDP_PLANE_B, PLANE_B_NT_VRAM_POSITION);
	md_vdp_set_plane_base(VDP_PLANE_WINDOW, WINDOW_NT_VRAM_POSITION);
	md_vdp_set_sprite_base(SPRITE_LIST_VRAM_POSITION);
	md_vdp_set_hscroll_base(HSCROLL_VRAM_POSITION);
	md_vdp_set_vmode(g_system_is_ntsc ? VDP_VMODE_V28 : VDP_VMODE_V30);

	system_srand(0x68000);
}

static void rand_step(void)
{
	static const uint32_t feedback_masks[] =
	{
		1 << 1,
		1 << 5,
		1 << 6,
		1 << 31
	};
	for (int16_t i = 0; i < 8; i++)
	{
		const uint16_t feedback =
		    ((s_rand_value & feedback_masks[0]) ? 1 : 0) ^
		    ((s_rand_value & feedback_masks[1]) ? 1 : 0) ^
		    ((s_rand_value & feedback_masks[2]) ? 1 : 0) ^
		    ((s_rand_value & feedback_masks[3]) ? 1 : 0);
		s_rand_value = s_rand_value >> 1;
		if (feedback) s_rand_value |= (1 << 31);
	}
}

void system_srand(uint32_t seed)
{
	if (seed == 0) seed = md_vdp_get_hv_count();
	s_rand_value = seed;
	rand_step();
}

uint32_t system_rand(void)
{
	rand_step();
	return s_rand_value;
}

void system_set_debug_enabled(int16_t en)
{
	g_system_debug_enabled = en;
}

void system_profile(uint16_t color)
{
	if (g_system_debug_enabled) md_pal_set(0, color);
}

void system_print_error(const char *expression,
                        const char *file,
                        const char *line_string)
{
	text_init(res_font_bin, sizeof(res_font_bin), 0x8000, 0, 3);
	text_puts(VDP_PLANE_WINDOW, 0, 0, "RUNTIME ASSERT FAILED!");
	text_puts(VDP_PLANE_WINDOW, 0, 1, "----------------------");
	text_puts(VDP_PLANE_WINDOW, 0, 3, expression);
	text_puts(VDP_PLANE_WINDOW, 0, 5, file);
	text_puts(VDP_PLANE_WINDOW, 0, 6, line_string);

	text_puts(VDP_PLANE_WINDOW, 0, 8, "Push START to reset.");
	md_vdp_set_window_top(16);
	md_pal_set(0, PALRGB(0, 0, 0));
	md_pal_set(63, PALRGB(7, 7, 7));
	md_vdp_set_display_en(1);
	while (!(g_md_pad[0] & BTN_START)) megadrive_finish();
	__asm("jmp (0x000004).l");
}

void system_print_hex(VdpPlane p, int16_t x, int16_t y, uint8_t num)
{
	char nums[3];
	nums[2] = '\0';

	const uint8_t low = num & 0x0F;
	const uint8_t high = (num & 0xF0) >> 4;

	if (low < 10) nums[1] = '0' + low;
	else nums[1] = 'A' + (low - 0xA);
	if (high < 10) nums[0] = '0' + high;
	else nums[0] = 'A' + (high - 0xA);

	text_puts(p, x, y, nums);
}
