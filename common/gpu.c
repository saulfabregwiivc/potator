#include "gpu.h"
#include <stdlib.h>
#include <string.h>

#ifdef GP2X
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif
#ifdef _ODSDL_
#include "../platform/opendingux/shared.h"
#endif

#ifdef NDS
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R)))|(1<<15))
#else
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R))))
#endif

const static uint8 palettes[SV_COLOR_SCHEME_COUNT][12] = {
{
    252, 252, 252,
    168, 168, 168,
     84,  84,  84,
      0,   0,   0,
},
{
    252, 154,   0,
    168, 102,   0,
     84,  51,   0,
      0,   0,   0,
},
{
     50, 227,  50,
     34, 151,  34,
     17,  76,  17,
      0,   0,   0,
},
{
      0, 154, 252,
      0, 102, 168,
      0,  51,  84,
      0,   0,   0,
},
{
    224, 248, 208,
    136, 192, 112,
     52, 104,  86,
      8,  24,  32,
},
{
    0x70, 0xb0, 0x78,
    0x48, 0x98, 0x90,
    0x24, 0x58, 0x60,
    0x08, 0x24, 0x30,
},
};

static uint16 *supervision_palette;

#define SB_MAX (SV_GHOSTING_MAX + 1)
static int ghostCount = 0;
static uint8 *screenBuffers[SB_MAX];
static uint8 screenBufferStartX[SB_MAX];

static void add_ghosting(uint32 scanline, uint16 *backbuffer, uint8 start_x, uint8 end_x);

void gpu_init(void)
{
    supervision_palette = (uint16*)malloc(4 * sizeof(int16));
}

void gpu_reset(void)
{
    gpu_set_color_scheme(SV_COLOR_SCHEME_DEFAULT);
    gpu_set_ghosting(0);
}

void gpu_done(void)
{
    free(supervision_palette);
    supervision_palette = NULL;
    gpu_set_ghosting(0);
}

void gpu_set_color_scheme(int colorScheme)
{
    float c[12];
    int i;
    for (i = 0; i < 12; i++) {
        c[i] = palettes[colorScheme][i] / 255.0f;
    }
#if defined(GP2X)
    supervision_palette[3] = gp2x_video_RGB_color16(255*c[9],255*c[10],255*c[11]);
    supervision_palette[2] = gp2x_video_RGB_color16(255*c[6],255*c[ 7],255*c[ 8]);
    supervision_palette[1] = gp2x_video_RGB_color16(255*c[3],255*c[ 4],255*c[ 5]);
    supervision_palette[0] = gp2x_video_RGB_color16(255*c[0],255*c[ 1],255*c[ 2]);
#elif defined(_ODSDL_)
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format, (int)(255*c[9]),(int)(255*c[10]),(int)(255*c[11]));
    supervision_palette[2] = PIX_TO_RGB(actualScreen->format, (int)(255*c[6]),(int)(255*c[ 7]),(int)(255*c[ 8]));
    supervision_palette[1] = PIX_TO_RGB(actualScreen->format, (int)(255*c[3]),(int)(255*c[ 4]),(int)(255*c[ 5]));
    supervision_palette[0] = PIX_TO_RGB(actualScreen->format, (int)(255*c[0]),(int)(255*c[ 1]),(int)(255*c[ 2]));
#else
    supervision_palette[3] = RGB555(31*c[9],31*c[10],31*c[11]);
    supervision_palette[2] = RGB555(31*c[6],31*c[ 7],31*c[ 8]);
    supervision_palette[1] = RGB555(31*c[3],31*c[ 4],31*c[ 5]);
    supervision_palette[0] = RGB555(31*c[0],31*c[ 1],31*c[ 2]);
#endif
}

void gpu_render_scanline(uint32 scanline, uint16 *backbuffer)
{
    uint8 *m_reg = memorymap_getRegisters();
    uint8 *vram_line = memorymap_getUpperRamPointer() + (m_reg[SV_XPOS] / 4 + m_reg[SV_YPOS] * 0x30) + (scanline * 0x30);
    uint8 x;

    for (x = 0; x < 160; x += 4) {
        uint8 b = *(vram_line++);
        backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
        backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
        backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
        backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
        backbuffer += 4;
    }
}

void gpu_render_scanline_fast(uint32 scanline, uint16 *backbuffer)
{
    uint8 *vram_line = memorymap_getUpperRamPointer() + scanline;
    uint8 x, j, b;

    uint8 *m_reg = memorymap_getRegisters();
    uint8 start_x = m_reg[SV_XPOS] & 3; //3 - (m_reg[SV_XPOS] & 3);
    uint8 end_x = (163 < (m_reg[SV_XSIZE] | 3) ? 163 : (m_reg[SV_XSIZE] | 3)) - 3; //(163 < (m_reg[SV_XSIZE] | 3) ? 163 : (m_reg[SV_XSIZE] | 3));
    //if (start_x != 0) printf("start_x = %u\n", start_x);
    //if (end_x != 160) printf("end_x = %u\n", end_x);
    j = start_x;

    // #1
    if (j & 3) {
        b = *vram_line++;
        b >>= (j & 3) * 2;
    }
    for (x = 0; x < end_x; x++, j++) {
        if (!(j & 3)) {
            b = *(vram_line++);
        }
        backbuffer[x] = supervision_palette[b & 3];
        b >>= 2;
    }
    // #2
    /*for (x = 0; x < end_x; x++, j++) {
        b = vram_line[j >> 2];
        backbuffer[x] = supervision_palette[(b >> ((j & 3) * 2)) & 3];
    }*/

    if (ghostCount != 0) {
        add_ghosting(scanline, backbuffer, start_x, end_x);
    }
}

void gpu_set_ghosting(int frameCount)
{
    int i;
    if (frameCount < 0)
        ghostCount = 0;
    else if (frameCount > SV_GHOSTING_MAX)
        ghostCount = SV_GHOSTING_MAX;
    else
        ghostCount = frameCount;

    if (ghostCount != 0) {
        if (screenBuffers[0] == NULL) {
            for (i = 0; i < SB_MAX; i++) {
                screenBuffers[i] = malloc(160 * 160 / 4);
            }
        }
        for (i = 0; i < SB_MAX; i++) {
            memset(screenBuffers[i], 0, 160 * 160 / 4);
        }
    }
    else {
        for (i = 0; i < SB_MAX; i++) {
            free(screenBuffers[i]);
            screenBuffers[i] = NULL;
        }
    }
}

static void add_ghosting(uint32 scanline, uint16 *backbuffer, uint8 start_x, uint8 end_x)
{
    static int curSB = 0;
    static int lineCount = 0;

    uint8 *vram_line = memorymap_getUpperRamPointer() + scanline;
    uint8 x, i, j;

    screenBufferStartX[curSB] = start_x;
    memset(screenBuffers[curSB] + lineCount * 160 / 4, 0, 160 / 4);
    for (j = start_x, x = 0; x < end_x; x++, j++) {
        uint8 b = vram_line[j >> 2];
        int pixInd = (x + lineCount * 160) / 4;
        uint8 innerInd = (j & 3) * 2;
        uint8 c = (b >> innerInd) & 3;
        if (c == 0) {
            for (i = 0; i < ghostCount; i++) {
                uint8 sbInd = (curSB + (SB_MAX - 1) - i) % SB_MAX;
                innerInd = ((screenBufferStartX[sbInd] + x) & 3) * 2;
                c = (screenBuffers[sbInd][pixInd] >> innerInd) & 3;
                if (c != 0) {
#if defined(GP2X) || defined(_ODSDL_)
                    backbuffer[x] = supervision_palette[3 - 3 * i / ghostCount];
#else
                    uint8 r = (supervision_palette[c] >>  0) & 31;
                    uint8 g = (supervision_palette[c] >>  5) & 31;
                    uint8 b = (supervision_palette[c] >> 10) & 31;
                    r = r + (((supervision_palette[0] >>  0) & 31) - r) * i / ghostCount;
                    g = g + (((supervision_palette[0] >>  5) & 31) - g) * i / ghostCount;
                    b = b + (((supervision_palette[0] >> 10) & 31) - b) * i / ghostCount;
                    backbuffer[x] = RGB555(r, g, b);
#endif
                    break;
                }
            }
        }
        else {
            screenBuffers[curSB][pixInd] |= c << innerInd;
        }
    }

    if (lineCount == 159) {
        curSB = (curSB + 1) % SB_MAX;
    }
    lineCount = (lineCount + 1) % 160;
}
