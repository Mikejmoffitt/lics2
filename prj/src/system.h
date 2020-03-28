#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

int system_init(void);
uint16_t system_is_ntsc(void);

// TODO: Put in some sort of actual assert.
#define SYSTEM_ASSERT(expression) { while(!(expression)) continue; }

#endif  // SYSTEM
