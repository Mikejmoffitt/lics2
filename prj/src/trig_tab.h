#ifndef TRIG_TAB_H
#define TRIG_TAB_H
#include "util/fixed.h"
#define TRIG_TAB_ATAN_INPUT_RANGE 64

#define DEGTOUINT8(deg) ((int)((deg / 360.0) * 256) % 256)
#define RADTOUINT8(rad) ((int)((rad / 2 * 3.14159) * 256) % 256)

// Range 0 to 2 * pi
const fix16_t trig_tab_sin[256];
const fix16_t trig_tab_cos[256];
const fix16_t trig_tab_tan[256];
// Range 0 to 64
const fix16_t trig_tab_atan[4096];

#endif  // TRIG_TAB_H
