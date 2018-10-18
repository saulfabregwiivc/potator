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

#ifdef NDS
#include <nds.h>
#endif

static M6502 m6502_registers;
static BOOL irq = FALSE;

void m6502_set_irq_line(BOOL assertLine)
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
    gpu_init();
    memorymap_init();
}

void supervision_reset(void)
{
    controls_reset();
    gpu_reset();
    memorymap_reset();
    sound_reset();
    timer_reset();

    Reset6502(&m6502_registers);
    irq = FALSE;
}

void supervision_done(void)
{
    gpu_done();
    memorymap_done();
}

BOOL supervision_load(uint8 **rom, uint32 romSize)
{
    memorymap_load(rom, romSize);
    supervision_reset();

    return TRUE;
}

BOOL supervision_update_input(void)
{
    return controls_update();
}

void supervision_set_color_scheme(int colorScheme)
{
    gpu_set_color_scheme(colorScheme);
}

void supervision_set_ghosting(int frameCount)
{
    gpu_set_ghosting(frameCount);
}

M6502 *supervision_get6502regs(void)
{
    return &m6502_registers;
}

void supervision_exec(uint16 *backbuffer)
{
    uint32 supervision_scanline, scan;
    uint8 *m_reg = memorymap_getRegisters();

    for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++) {
        // 256 * 256 -- 1 frame (61 FPS), 256 * 256 / 160 = 409.6 cycles per line
        m6502_registers.ICount = 410;
        timer_exec(m6502_registers.ICount);

        Run6502(&m6502_registers);
    }

    //if (!((m_reg[SV_BANK] >> 3) & 1)) { printf("LCD off\n"); }
    scan = m_reg[SV_XPOS] / 4 + m_reg[SV_YPOS] * 0x30;
    for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++) {
#ifdef NDS
        gpu_render_scanline(supervision_scanline, backbuffer);
        backbuffer += 160;
#else
        //gpu_render_scanline(supervision_scanline, backbuffer);
        gpu_render_scanline_fast(scan, backbuffer);
        backbuffer += 160;
        scan += 0x30;
#endif
        if (scan >= 0x1fe0)
            scan = 0; // SSSnake
    }

    if (Rd6502(0x2026) & 0x01)
        Int6502(supervision_get6502regs(), INT_NMI);

    sound_decrement();
}

static void get_state_path(const char *statePath, int id, char **newPath)
{
    int newPathLen;
    newPathLen = strlen(statePath);
    *newPath = (char *)malloc(newPathLen + 1 + 6); // strlen("X.svst")
    strcpy(*newPath, statePath);
    sprintf(*newPath + newPathLen, "%d.svst", id);
}

BOOL supervision_save_state(const char *statePath, int id)
{
    FILE *fp;
    char *newPath;

    get_state_path(statePath, id, &newPath);
    fp = fopen(newPath, "wb");
    free(newPath);
    if (fp) {
        fwrite(&m6502_registers, 1, sizeof(m6502_registers), fp);
        fwrite(&irq, 1, sizeof(irq), fp);

        memorymap_save_state(fp);
        timer_save_state(fp);

        fflush(fp);
        fclose(fp);
    }
    else {
        return FALSE;
    }
    return TRUE;
}

BOOL supervision_load_state(const char *statePath, int id)
{
    FILE *fp;
    char *newPath;

    get_state_path(statePath, id, &newPath);
    fp = fopen(newPath, "rb");
    free(newPath);
    if (fp) {
        sound_reset();

        fread(&m6502_registers, 1, sizeof(m6502_registers), fp);
        fread(&irq, 1, sizeof(irq), fp);

        memorymap_load_state(fp);
        timer_load_state(fp);

        fclose(fp);
    }
    else {
        return FALSE;
    }
    return TRUE;
}
