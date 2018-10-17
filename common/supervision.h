#ifndef __SUPERVISION_H__
#define __SUPERVISION_H__

#include "types.h"
#include "gpu.h"
#include "timer.h"
#include "controls.h"
#include "memorymap.h"

#include "./m6502/m6502.h"

#ifdef __cplusplus
extern "C" {
#endif

void supervision_init(void);
void supervision_reset(void);
void supervision_done(void);
void supervision_exec(uint16 *backbuffer);
BOOL supervision_load(uint8 **rom, uint32 romSize);
BOOL supervision_update_input(void);
void supervision_set_colour_scheme(int colourScheme);
void supervision_set_ghosting(int frameCount);
M6502 *supervision_get6502regs(void);

BOOL supervision_save_state(const char *statePath, int id);
BOOL supervision_load_state(const char *statePath, int id);

#ifdef __cplusplus
}
#endif

#endif
