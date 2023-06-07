#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "md/megadrive.h"
#include "util/text.h"
#include "res.h"
#include "gfx.h"

void system_init(void);
void system_srand(uint32_t seed);
uint32_t system_rand(void);
void system_set_debug_enabled(bool en);

void system_print_error(const char *expression,
                        const char *file,
                        const char *line_string);

extern bool g_system_is_ntsc;
static inline bool system_is_ntsc(void)
{
	return g_system_is_ntsc;
}

extern bool g_system_debug_enabled;
static inline bool system_is_debug_enabled(void)
{
	return g_system_debug_enabled;
}

void system_print_hex(VdpPlane p, int16_t x, int16_t y, uint8_t num);

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)


// TODO: Put in some sort of actual assert.
#define SYSTEM_ASSERT(x) (void)(x)

/*
#define SYSTEM_ASSERT(expression) \
{\
	if (!(expression))\
	{\
		system_print_error(#expression, __FILE__, LINE_STRING);\
	}\
}\
*/


#endif  // SYSTEM
