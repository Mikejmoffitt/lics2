#ifndef PALSCALE_H
#define PALSCALE_H

#include "system.h"

// Macros to scale constants by appropriate NTSC/PAL adjustment coefficients.

#define PALSCALE_DURATION_COEF (0.8333333)
#define PALSCALE_1ST_COEF (1.2)
#define PALSCALE_2ND_COEF (1.44)


// Used for durations (e.g. frame count); 
#define PALSCALE_DURATION(x)      ( system_is_ntsc() ? (x) : (x * PALSCALE_DURATION_COEF) )

// For speeds (e.g. dx)
#define PALSCALE_1ST(x)           ( system_is_ntsc() ? (x) : (x * PALSCALE_1ST_COEF) )

// For accelerations (e.g. ddx)
#define PALSCALE_2ND(x)           ( system_is_ntsc() ? (x) : (x * PALSCALE_2ND_COEF) )

#define PALSCALE_MANUAL(x_n, x_p) ( system_is_ntsc() ? (x_n) : (x_p) )

#endif  // PALSCALE_H
