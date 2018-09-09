#ifndef __SUPERVISION_H__
#define __SUPERVISION_H__

#include "types.h"
#include "gpu.h"
#include "timer.h"
#include "controls.h"
#include "memorymap.h"

#ifdef GP2X
#include "menues.h"
#endif

#include "./m6502/m6502.h"

#define COLOUR_SCHEME_DEFAULT 0
#define COLOUR_SCHEME_AMBER   1
#define COLOUR_SCHEME_GREEN   2
#define COLOUR_SCHEME_BLUE    3

void supervision_init(void);
void supervision_reset(void);
void supervision_done(void);
void supervision_exec(int16 *backbuffer, BOOL bRender);
BOOL supervision_load(uint8 *rom, uint32 romSize);
BOOL supervision_update_input(void);
void supervision_set_colour_scheme(int ws_colourScheme);
M6502 *supervision_get6502regs(void);
void supervision_turnSound(BOOL bOn);

int sv_loadState(const char *statepath, int id);
int sv_saveState(const char *statepath, int id);

#endif
