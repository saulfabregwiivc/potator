#ifndef __MEMORYMAP_H__
#define __MEMORYMAP_H__

#include "supervision.h"

enum
{
    XSIZE = 0x00,
    XPOS  = 0x02,
    YPOS  = 0x03,
    BANK  = 0x26,
};

void memorymap_set_dma_finished(void);
void memorymap_set_timer_shot(void);

void memorymap_init(void);
void memorymap_reset(void);
uint8  memorymap_registers_read(uint32 Addr);
void memorymap_registers_write(uint32 Addr, uint8 Value);
void memorymap_load(uint8 *rom, uint32 size);

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
