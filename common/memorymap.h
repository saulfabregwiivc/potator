#ifndef __MEMORYMAP_H__
#define __MEMORYMAP_H__

#include "supervision.h"
#include <stdio.h>

enum
{
    SV_XSIZE = 0x00,
    SV_XPOS  = 0x02,
    SV_YPOS  = 0x03,
    SV_BANK  = 0x26,
};

void memorymap_set_dma_finished(void);
void memorymap_set_timer_shot(void);

void memorymap_init(void);
void memorymap_reset(void);
void memorymap_done(void);
uint8  memorymap_registers_read(uint32 Addr);
void memorymap_registers_write(uint32 Addr, uint8 Value);
void memorymap_load(uint8 **rom, uint32 size);

void memorymap_save_state(FILE *fp);
void memorymap_load_state(FILE *fp);

uint8 *memorymap_getUpperRamPointer(void);
uint8 *memorymap_getLowerRamPointer(void);
uint8 *memorymap_getUpperRomBank(void);
uint8 *memorymap_getLowerRomBank(void);
uint8 *memorymap_getRegisters(void);
uint8 *memorymap_getRomPointer(void);

extern uint8 *memorymap_programRom;
extern uint8 *memorymap_lowerRam;
extern uint8 *memorymap_upperRam;
extern uint8 *memorymap_lowerRomBank;
extern uint8 *memorymap_upperRomBank;
extern uint8 *memorymap_regs;

#endif
