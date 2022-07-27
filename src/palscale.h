#ifndef PALSCALE_H
#define PALSCALE_H

#include "system.h"

// Macros to scale constants by appropriate NTSC/PAL adjustment coefficients.

// TODO: Make an interface for g_game_speed, and set that up at the start.
extern float g_game_speed;

#ifndef LYLE_PALSCALE_STATIC_COEF

#define SPEED_DURATION_COEF (g_game_speed)
#define SPEED_1ST_COEF (1 / g_game_speed)
#define SPEED_2ND_COEF (SPEED_1ST_COEF * SPEED_1ST_COEF)

#else

// Master speed adjustment.
#define SPEED_COEF (1.0)

#define SPEED_DURATION_COEF (1 / SPEED_COEF)
#define SPEED_1ST_COEF (1 / SPEED_DURATION_COEF)
#define SPEED_2ND_COEF (SPEED_1ST_COEF * SPEED_1ST_COEF)

#endif

// Ratio that reflects the PAL vs NTSC framerate (50 vs 60hz)
#define PAL_NTSC_RATIO (50.0 / 60.0)

#define PALSCALE_DURATION_COEF (PAL_NTSC_RATIO)
#define PALSCALE_1ST_COEF (1 / PALSCALE_DURATION_COEF)
#define PALSCALE_2ND_COEF (PALSCALE_1ST_COEF * PALSCALE_1ST_COEF)

#define PALSCALE_MANUAL(x_n, x_p) ( system_is_ntsc() ? (x_n) : (x_p) )

// Used for durations (e.g. frame count); 
#define PALSCALE_DURATION(x)      ( SPEED_DURATION_COEF * (system_is_ntsc() ? (x) : ((x) * PALSCALE_DURATION_COEF) ))

// For speeds (e.g. dx)
#define PALSCALE_1ST(x)           ( SPEED_1ST_COEF * (system_is_ntsc() ? (x) : ((x) * PALSCALE_1ST_COEF) ))

// For accelerations (e.g. ddx)
#define PALSCALE_2ND(x)           ( SPEED_2ND_COEF * (system_is_ntsc() ? (x) : ((x) * PALSCALE_2ND_COEF) ))

void palscale_set_game_speed(float speed);

#endif  // PALSCALE_H
