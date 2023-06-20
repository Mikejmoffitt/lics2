#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

void timer_stop(void);
void timer_start(void);
void timer_poll(void);

#endif  // TIMER_H
