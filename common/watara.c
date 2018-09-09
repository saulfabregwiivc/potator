#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "supervision.h"
#include "memorymap.h"
#include "sound.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef GP2X
#include "menues.h"
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif

static M6502 m6502_registers;

static BOOL irq = FALSE;

void m6502_set_irq_line(int assertLine)
{
    m6502_registers.IRequest = assertLine ? INT_IRQ : INT_NONE;
    irq = assertLine;
}

byte Loop6502(register M6502 *R)
{
    if (irq) {
        irq = FALSE;
        return INT_IRQ;
    }
    return INT_QUIT;
}

void supervision_init(void)
{
    memorymap_init();
    gpu_init();
}

BOOL supervision_load(uint8 *rom, uint32 romSize)
{
    memorymap_load(rom, romSize);
    supervision_reset();

    return TRUE;
}

void supervision_reset(void)
{
    memorymap_reset();
    gpu_reset();
    timer_reset();
    controls_reset();

    Reset6502(&m6502_registers);

    irq = FALSE;
}

void supervision_done(void)
{
}

void supervision_set_colour_scheme(int sv_colourScheme)
{
    gpu_set_colour_scheme(sv_colourScheme);
}

M6502 *supervision_get6502regs(void)
{
    return &m6502_registers;
}

BOOL supervision_update_input(void)
{
    return controls_update();
}

void supervision_exec(int16 *backbuffer, BOOL bRender)
{
    uint32 supervision_scanline, scan1 = 0;

    uint8 *m_reg = memorymap_getRegisters();
    //if (!((m_reg[BANK] >> 3) & 1)) { printf("ndraw "); }
    scan1 = m_reg[XPOS] / 4 + m_reg[YPOS] * 0x30;

    for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++)
    {
        m6502_registers.ICount = 512; 
        timer_exec(m6502_registers.ICount);

        Run6502(&m6502_registers);
#ifdef NDS
        gpu_render_scanline(supervision_scanline, backbuffer);
        backbuffer += 160+96;
#else
        //gpu_render_scanline(supervision_scanline, backbuffer);
        gpu_render_scanline_fast(scan1, backbuffer);
        backbuffer += 160;
        scan1 += 0x30;
#endif
        if (scan1 >= 0x1fe0)
            scan1 = 0; // SSSnake
    }

    if (Rd6502(0x2026)&0x01)
        Int6502(supervision_get6502regs(), INT_NMI);

    sound_decrement(); // MCFG_SCREEN_VBLANK_CALLBACK, svision.cpp
}

void supervision_turnSound(BOOL bOn)
{
}

int sv_loadState(const char *statepath, int id)
{
    FILE *fp;
    char newPath[256];

    strcpy(newPath, statepath);
    sprintf(newPath + strlen(newPath) - 3, ".s%d", id);

#ifdef GP2X
    gp2x_printf(0,10,220,"newPath = %s",newPath);
    gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
    iprintf("\nnewPath = %s",newPath);
#endif

    fp = fopen(newPath, "rb");

    if (fp) {
        fread(&m6502_registers,       1,        sizeof(m6502_registers), fp);
        fread(memorymap_programRom,   1,   sizeof(memorymap_programRom), fp);
        fread(memorymap_lowerRam,     1,                         0x2000, fp);
        fread(memorymap_upperRam,     1,                         0x2000, fp);
        fread(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
        fread(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
        fread(memorymap_regs,         1,                         0x2000, fp);
        fclose(fp);
    }

#ifdef GP2X
    sleep(1);
#endif

    return 1;
}

int sv_saveState(const char *statepath, int id)
{
    FILE *fp;
    char newPath[256];

    strcpy(newPath, statepath);
    sprintf(newPath + strlen(newPath) - 3, ".s%d", id);

#ifdef GP2X
    gp2x_printf(0,10,220,"newPath = %s",newPath);
    gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
    iprintf("\nnewPath = %s",newPath);
#endif

    fp = fopen(newPath, "wb");

    if (fp) {
        fwrite(&m6502_registers,       1,        sizeof(m6502_registers), fp);
        fwrite(memorymap_programRom,   1,   sizeof(memorymap_programRom), fp);
        fwrite(memorymap_lowerRam,     1,                         0x2000, fp);
        fwrite(memorymap_upperRam,     1,                         0x2000, fp);
        fwrite(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
        fwrite(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
        fwrite(memorymap_regs,         1,                         0x2000, fp);
        fflush(fp);
        fclose(fp);
#ifdef GP2X
        sync();
#endif
    }

#ifdef GP2X
    sleep(1);
#endif

    return 1;
}
