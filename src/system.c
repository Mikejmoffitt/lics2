#include "system.h"
#include "md/megadrive.h"
#include "vram_map.h"

int16_t g_system_is_ntsc;
int16_t g_system_debug_enabled;

static uint32_t s_rand_value;

int system_init(void)
{
	megadrive_init();
	vdp_set_plane_size(VDP_PLANESIZE_64x64);
	vdp_set_plane_base(VDP_PLANE_A, PLANE_A_NT_VRAM_POSITION);
	vdp_set_plane_base(VDP_PLANE_B, PLANE_B_NT_VRAM_POSITION);
	vdp_set_plane_base(VDP_PLANE_WINDOW, WINDOW_NT_VRAM_POSITION);
	vdp_set_sprite_base(SPRITE_LIST_VRAM_POSITION);
	vdp_set_hscroll_base(HSCROLL_VRAM_POSITION);
	g_system_is_ntsc = (vdp_get_status() & VDP_STATUS_PAL) ? 0 : 1;
	vdp_set_raster_height(system_is_ntsc() ? 224 : 240);

	// Some environmental sanity.
	SYSTEM_ASSERT(sizeof(int32_t) == 4);
	SYSTEM_ASSERT(sizeof(int16_t) == 2);
	SYSTEM_ASSERT(sizeof(int8_t) == 1);
	SYSTEM_ASSERT(sizeof(int32_t) > sizeof(int16_t));

	int32_t a = 70000;
	int16_t b = 32;

	SYSTEM_ASSERT(a + b == 70032);
	a += b;
	SYSTEM_ASSERT(a == 70032);

	system_srand(0x12345678);

	return 1;
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
	if (seed == 0) seed = vdp_get_hv_count();
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
	if (g_system_debug_enabled) pal_set(0, color);
}

void system_print_error(const char *expression,
                        const char *file,
                        const char *line_string)
{
	text_init(res_font_bin, sizeof(res_font_bin), 0x8000, 0, 3);
	text_puts(VDP_PLANE_WINDOW, 0, 0, "ASSERT FAILED!   ");
	text_puts(VDP_PLANE_WINDOW, 0, 1, "--------------   ");
	text_puts(VDP_PLANE_WINDOW, 0, 3, expression);
	text_puts(VDP_PLANE_WINDOW, 0, 5, file);
	text_puts(VDP_PLANE_WINDOW, 0, 6, line_string);

	text_puts(VDP_PLANE_WINDOW, 0, 8, "Push START to reset.");
	vdp_set_window_top(16);
	pal_set(0, PALRGB(0, 0, 0));
	pal_set(63, PALRGB(7, 7, 7));
	vdp_set_display_en(1);
	while (!(md_io_pad_read(0) & BTN_START)) megadrive_finish();
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
