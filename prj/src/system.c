#include "system.h"
#include "md/megadrive.h"

static int16_t is_ntsc_cache;
static int16_t is_debug_enabled;

static volatile char activity[64];

int system_init(void)
{
	megadrive_init();
	is_ntsc_cache = (vdp_get_status() & VDP_STATUS_PAL) ? 0 : 1;
	vdp_set_raster_height(is_ntsc_cache ? 224 : 240);
	return 1;
}

uint16_t system_is_ntsc(void)
{
	return is_ntsc_cache;
}

uint16_t system_rand(void)
{
	// TODO: Implement a better random, maybe an LSFR
	return vdp_get_hv_count();
}

int16_t system_is_debug_enabled(void)
{
	return is_debug_enabled;
}

void system_set_debug_enabled(int16_t en)
{
	is_debug_enabled = en;
}

void system_profile(uint16_t color)
{
	if (is_debug_enabled) pal_set(0, color);
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
	while (!(io_pad_read(0) & BTN_START)) megadrive_finish();
	__asm("jmp (0x000004).l");
}
