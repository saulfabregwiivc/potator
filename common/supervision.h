////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __SUPERVISION_H__
#define __SUPERVISION_H__

/*#include "log.h"*/
#include "types.h"
/*#include "memory.h"*/
#include "version.h"
#include "io.h"
#include "gpu.h"
#include "timer.h"
#include "controls.h"
#include "memorymap.h"
#include "interrupts.h"

#ifdef GP2X
#include "menues.h"
#endif

#include "./m6502/m6502.h"

#ifdef __LIBRETRO__
#define COLOUR_SCHEME_NONE                    0
#define COLOUR_SCHEME_SUPERVISION             1
#define COLOUR_SCHEME_GB_DMG                  2
#define COLOUR_SCHEME_GB_POCKET               3
#define COLOUR_SCHEME_GB_LIGHT                4
#define COLOUR_SCHEME_BLOSSOM_PINK            5
#define COLOUR_SCHEME_BUBBLES_BLUE            6
#define COLOUR_SCHEME_BUTTERCUP_GREEN         7
#define COLOUR_SCHEME_DIGIVICE                8
#define COLOUR_SCHEME_GAME_COM                9
#define COLOUR_SCHEME_GAMEKING               10
#define COLOUR_SCHEME_GAME_MASTER            11
#define COLOUR_SCHEME_GOLDEN_WILD            12
#define COLOUR_SCHEME_GREENSCALE             13
#define COLOUR_SCHEME_HOKAGE_ORANGE          14
#define COLOUR_SCHEME_LABO_FAWN              15
#define COLOUR_SCHEME_LEGENDARY_SUPER_SAIYAN 16
#define COLOUR_SCHEME_MICROVISION            17
#define COLOUR_SCHEME_MILLION_LIVE_GOLD      18
#define COLOUR_SCHEME_ODYSSEY_GOLD           19
#define COLOUR_SCHEME_SHINY_SKY_BLUE         20
#define COLOUR_SCHEME_SLIME_BLUE             21
#define COLOUR_SCHEME_TI_83                  22
#define COLOUR_SCHEME_TRAVEL_WOOD            23
#define COLOUR_SCHEME_VIRTUAL_BOY            24
#else
#define COLOUR_SCHEME_DEFAULT	0
#define COLOUR_SCHEME_AMBER	1
#define COLOUR_SCHEME_GREEN	2
#define COLOUR_SCHEME_BLUE		3
#endif

extern void supervision_init(void);
extern void supervision_reset(void);
extern void supervision_reset_handler(void);
extern void supervision_done(void);
extern void supervision_exec(uint16 *backbuffer, BOOL bRender);
extern void supervision_exec2(uint16 *backbuffer, BOOL bRender);
extern void supervision_exec3(uint16 *backbuffer, BOOL bRender);
extern void supervision_exec_fast(uint16 *backbuffer, BOOL bRender);
extern BOOL supervision_load(uint8 *rom, uint32 romSize);
extern BOOL supervision_update_input(void);
extern void supervision_set_colour_scheme(int ws_colourScheme);
extern M6502	*supervision_get6502regs(void);
extern void supervision_turnSound(BOOL bOn);

extern int sv_loadState(char *statepath, int id);
extern int sv_saveState(char *statepath, int id);

extern uint32 sv_saveStateBufSize(void);
extern BOOL sv_loadStateBuf(const void *data, uint32 size);
extern BOOL sv_saveStateBuf(void *data, uint32 size);

#endif
