#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include "md/megadrive.h"
#include "util/text.h"

int system_init(void);
uint16_t system_is_ntsc(void);
uint16_t system_rand(void);
void system_set_debug_enabled(int16_t en);

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

// TODO: Put in some sort of actual assert.
#define SYSTEM_ASSERT(expression) \
{\
	if (!(expression))\
	{\
		text_puts(VDP_PLANE_B, 0, 0, "ASSERT FAILED!   ");\
		text_puts(VDP_PLANE_B, 0, 1, "--------------   ");\
		text_puts(VDP_PLANE_B, 0, 3, #expression);\
		text_puts(VDP_PLANE_B, 0, 5, "File \""__FILE__"\"");\
		text_puts(VDP_PLANE_B, 0, 6, "Line "LINE_STRING);\
		pal_set(0, PALRGB(2, 0, 0));\
		pal_set(31, PALRGB(7, 7, 3));\
		while (1) continue;\
	}\
}\

#endif  // SYSTEM
