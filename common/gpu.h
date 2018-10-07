#ifndef __GPU_H__
#define __GPU_H__

#include "supervision.h"

enum
{
    COLOUR_SCHEME_DEFAULT,
    COLOUR_SCHEME_AMBER,
    COLOUR_SCHEME_GREEN,
    COLOUR_SCHEME_BLUE,
    COLOUR_SCHEME_BGB,
    COLOUR_SCHEME_YOUTUBE,
    COLOUR_SCHEME_COUNT
};

#define GHOSTING_MAX 8

void gpu_init(void);
void gpu_reset(void);
void gpu_set_colour_scheme(int colourScheme);
void gpu_render_scanline(uint32 scanline, uint16 *backbuffer);
void gpu_render_scanline_fast(uint32 scanline, uint16 *backbuffer);
void gpu_set_ghosting(int frameCount);

#endif
