#ifndef TRIG_H
#define TRIG_H

// Fixed point trigonometry.

#include "common.h"

#include "util/fixed.h"
#include "trig_tab.h"

#define TRIG_FIX16_PI INTTOFIX16((3.1415926535897))

// sin, cos, and tan accept an angle from 0 - 255, representing 0 - 2 * pi.
static inline fix16_t fix_sin(uint8_t angle)
{
	return trig_tab_sin[angle];
}

static inline fix16_t fix_cos(uint8_t angle)
{
	return trig_tab_cos[angle];
}

static inline fix16_t fix_tan(uint8_t angle)
{
	return trig_tab_tan[angle];
}

// atan takes a fix16 fraction representing y / x, normalized to be positive.
uint8_t fix_atan(fix32_t ratio);

#endif  // TRIG_H
