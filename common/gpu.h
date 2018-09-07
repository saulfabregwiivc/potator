#ifndef __GPU_H__
#define __GPU_H__

#include "supervision.h"

void gpu_init(void);
void gpu_reset(void);
void gpu_set_colour_scheme(int colourScheme);
void gpu_write(uint32 addr, uint8 data);
uint8 gpu_read(uint32 addr);
void gpu_set_colour_scheme(int ws_colourScheme);
void gpu_render_scanline(uint32 scanline, int16 *backbuffer);
void gpu_render_scanline_fast(uint32 scanline, uint16 *backbuffer);

#endif
