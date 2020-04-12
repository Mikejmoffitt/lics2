#ifndef _MUSIC_H
#define _MUSIC_H

#include <stdint.h>

int music_init(void);
void music_play(uint8_t track);
void music_stop(void);


#endif  // _MUSIC_H
