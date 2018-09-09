#ifndef __TIMER_H__
#define __TIMER_H__

#include "supervision.h"

void timer_reset(void);
void timer_write(uint8 data);
void timer_exec(uint32 cycles);

#endif
