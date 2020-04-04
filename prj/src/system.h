#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include "md/megadrive.h"

int system_init(void);
uint16_t system_is_ntsc(void);
uint16_t system_rand(void);

// TODO: Put in some sort of actual assert.
#define SYSTEM_ASSERT(expression) { while(!(expression)) pal_set(0, PALRGB(7, 0, 0)); }

#endif  // SYSTEM
