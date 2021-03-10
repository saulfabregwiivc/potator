#ifndef __TIMER_H__
#define __TIMER_H__

#include "supervision.h"

void timer_init();
void timer_done();
void timer_reset();
void timer_write(uint32 addr, uint8 data);
uint8 timer_read(uint32 addr);
void timer_exec(uint32 cycles);

extern uint8 timer_regs[2];
extern int32 timer_cycles;
extern BOOL  timer_activated;

#endif
