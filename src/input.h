#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

typedef uint8_t LyleBtn;

#define LYLE_BTN_UP    0x0001
#define LYLE_BTN_DOWN  0x0002
#define LYLE_BTN_LEFT  0x0004
#define LYLE_BTN_RIGHT 0x0008
#define LYLE_BTN_START 0x0010
#define LYLE_BTN_CUBE  0x0020
#define LYLE_BTN_JUMP  0x0040

LyleBtn input_read(void);
void input_poll(void);

#endif  // INPUT_H
