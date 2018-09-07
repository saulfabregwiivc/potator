#include "memorymap.h"
#include "sound.h"
#include <stdlib.h>
#include <string.h>

uint8 *memorymap_programRom;
uint8 *memorymap_lowerRam;
uint8 *memorymap_upperRam;
uint8 *memorymap_lowerRomBank;
uint8 *memorymap_upperRomBank;
uint8 *memorymap_regs;

static uint32 memorymap_programRomSize;

BOOL dma_finished = FALSE;
BOOL timer_shot = FALSE;
static void check_irq() {
    int irq = timer_shot && ((memorymap_regs[BANK] >> 1) & 1);
    irq = irq || (dma_finished && ((memorymap_regs[BANK] >> 2) & 1));

    void m6502_set_irq_line(int);
    m6502_set_irq_line(irq);
}

/*!
 * WRITE_LINE_MEMBER(svision_state::sound_irq_w), MAME
 */
void memorymap_set_dma_finished()
{
    dma_finished = TRUE;
    check_irq();
}

void memorymap_set_timer_shot()
{
    timer_shot = TRUE;
    check_irq();
}

uint8 *memorymap_getRomPointer(void)
{
    return(memorymap_programRom);
}

void memorymap_init()
{
    memorymap_lowerRam = (uint8*)malloc(0x2000);
    memorymap_upperRam = (uint8*)malloc(0x2000);
    memorymap_regs     = (uint8*)malloc(0x2000);

}

void memorymap_reset()
{
    memorymap_lowerRomBank = memorymap_programRom + 0x0000;
    memorymap_upperRomBank = memorymap_programRom + (memorymap_programRomSize==0x10000?0xc000:0x4000);

    memset(memorymap_lowerRam, 0x00, 0x2000);
    memset(memorymap_upperRam, 0x00, 0x2000);
}

uint8 memorymap_registers_read(uint32 Addr)
{
    uint8 data = memorymap_regs[Addr & 0x1fff];
    switch (Addr & 0x1fff) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
        break;
        //return (gpu_read(Addr));
    case 0x20:
        return (controls_read(Addr));
    case 0x21:
        data &= ~0xf;
        data |= memorymap_regs[0x22] & 0xf;
        break;
    case 0x24:
        timer_shot = FALSE;
        check_irq();
        break;
    case 0x27:
        data &= ~3;
        if (timer_shot) {
            data |= 1;
        }
        if (dma_finished) {
            data |= 2;
        }
        break;
    case 0x25:
        dma_finished = FALSE;
        check_irq();
        break;
    }
    return (data);
}

void memorymap_registers_write(uint32 Addr, uint8 Value)
{
    memorymap_regs[Addr & 0x1fff] = Value;

    switch (Addr & 0x1fff) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
        gpu_write(Addr, Value);
        break;
    case 0x23:
        timer_write(Value);
        break;
    case 0x26:
        //printf( "memorymap: writing 0x%.2x to rom bank register\n", Value);
        memorymap_lowerRomBank = memorymap_programRom + ((((uint32)Value) & 0x60) << 9);
        memorymap_upperRomBank = memorymap_programRom + (memorymap_programRomSize == 0x10000 ? 0xc000 : 0x4000);
        check_irq();
        return;
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x14: case 0x15: case 0x16: case 0x17:
        soundport_w(((Addr & 0x4) >> 2), Addr & 3, Value);
        break;
        //sound_write(Addr&7, Value); break;
    case 0x28:
    case 0x29:
    case 0x2a:
        svision_noise_w(Addr & 0x07, Value);
        break;
        //sound_noise_write(Addr&0x07, Value); break;
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
        svision_sounddma_w(Addr & 0x07, Value);
        break;
        //sound_audio_dma(Addr&0x07, Value); break;
    }
}

void Wr6502(register word Addr, register byte Value) {
    Addr &= 0xffff;
    switch (Addr >> 12) {
    case 0x0:
    case 0x1:
        memorymap_lowerRam[Addr] = Value;
        return;
    case 0x2:
    case 0x3:
        memorymap_registers_write(Addr, Value);
        return;
    case 0x4:
    case 0x5:
        memorymap_upperRam[Addr & 0x1fff] = Value;
        return;
    }
}

byte Rd6502(register word Addr) {
    Addr &= 0xffff;

    switch (Addr >> 12) {
    case 0x0:
    case 0x1:
        return (memorymap_lowerRam[Addr]);
    case 0x2:
    case 0x3:
        return (memorymap_registers_read(Addr));
    case 0x4:
    case 0x5:
        return (memorymap_upperRam[Addr & 0x1fff]);
    case 0x6:
    case 0x7:
        return (memorymap_programRom[Addr & 0x1fff]);
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        return (memorymap_lowerRomBank[Addr & 0x3fff]);
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        return (memorymap_upperRomBank[Addr & 0x3fff]);
    }
    return (0xff);
}

void memorymap_load(uint8 *rom, uint32 size) {
    memorymap_programRomSize = size;
    memorymap_programRom = rom;

    if (memorymap_programRomSize == 32768) {
        uint8 *tmp = (uint8 *)malloc(0x10000);
        memcpy(tmp + 0x0000, memorymap_programRom, 0x8000);
        memcpy(tmp + 0x8000, memorymap_programRom, 0x8000);
        free(memorymap_programRom);
        memorymap_programRom = tmp;
        memorymap_programRomSize = 0x10000;
    }
}

uint8 *memorymap_getUpperRamPointer(void) {
    return (memorymap_upperRam);
}

uint8 *memorymap_getLowerRamPointer(void) {
    return (memorymap_lowerRam);
}

uint8 *memorymap_getUpperRomBank(void) {
    return (memorymap_upperRomBank);
}

uint8 *memorymap_getLowerRomBank(void) {
    return (memorymap_lowerRomBank);
}

uint8 *memorymap_getRegisters(void) {
    return (memorymap_regs);
}
