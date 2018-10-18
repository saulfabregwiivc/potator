#ifndef __GPU_H__
#define __GPU_H__

#include "supervision.h"

enum
{
    SV_COLOR_SCHEME_DEFAULT,
    SV_COLOR_SCHEME_AMBER,
    SV_COLOR_SCHEME_GREEN,
    SV_COLOR_SCHEME_BLUE,
    SV_COLOR_SCHEME_BGB,
    SV_COLOR_SCHEME_YOUTUBE,
    SV_COLOR_SCHEME_COUNT
};

#define SV_GHOSTING_MAX 8

void gpu_init(void);
void gpu_reset(void);
void gpu_done(void);
void gpu_set_color_scheme(int colorScheme);
void gpu_render_scanline(uint32 scanline, uint16 *backbuffer);
void gpu_render_scanline_fast(uint32 scanline, uint16 *backbuffer);
void gpu_set_ghosting(int frameCount);

#endif
