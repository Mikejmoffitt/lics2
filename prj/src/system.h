#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include "md/megadrive.h"
#include "util/text.h"
#include "res.h"
#include "gfx.h"

int system_init(void);
uint16_t system_is_ntsc(void);
uint16_t system_rand(void);
int16_t system_is_debug_enabled(void);
void system_set_debug_enabled(int16_t en);
void system_profile(uint16_t color);

void system_print_error(const char *expression,
                        const char *file,
                        const char *line_string);

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)


// TODO: Put in some sort of actual assert.
#define SYSTEM_ASSERT(expression) \
{\
	if (!(expression))\
	{\
		system_print_error(#expression, __FILE__, LINE_STRING);\
	}\
}\

#endif  // SYSTEM
