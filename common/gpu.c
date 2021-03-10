////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
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
#ifdef _OPENDINGUX_
#include "../platform/opendingux/shared.h"
#endif
#ifdef _RS97_
#include "../platform/rs97/shared.h"
#endif

static uint16	*supervision_palette;
uint8          gpu_regs[4];
#ifdef NDS
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R)))|BIT(15))
#else
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R))))
#endif

#ifdef __LIBRETRO__
static const uint32 palette_colours_none[] = {
	0xE0E0E0, 0xB9B9B9, 0x545454, 0x121212
};

static const uint32 palette_colours_supervision[] = {
	0x7CC67C, 0x54868C, 0x2C6264, 0x0C322C
};

static const uint32 palette_colours_gb_dmg[] = {
	0x578200, 0x317400, 0x005121, 0x00420C
};

static const uint32 palette_colours_gb_pocket[] = {
	0xA7B19A, 0x86927C, 0x535f49, 0x2A3325
};

static const uint32 palette_colours_gb_light[] = {
	0x01CBDF, 0x01B6D5, 0x269BAD, 0x00778D
};

static const uint32 palette_colours_blossom_pink[] = {
	0xF09898, 0xA86A6A, 0x603C3C, 0x180F0F
};

static const uint32 palette_colours_bubbles_blue[] = {
	0x88D0F0, 0x5F91A8, 0x365360, 0x0D1418
};

static const uint32 palette_colours_buttercup_green[] = {
	0xB8E088, 0x809C5F, 0x495936, 0x12160D
};

static const uint32 palette_colours_digivice[] = {
	0x8C8C73, 0x646453, 0x38382E, 0x000000
};

static const uint32 palette_colours_game_com[] = {
	0xA7BF6B, 0x6F8F4F, 0x0F4F2F, 0x000000
};

static const uint32 palette_colours_gameking[] = {
	0x8CCE94, 0x6B9C63, 0x506541, 0x184221
};

static const uint32 palette_colours_game_master[] = {
	0x829FA6, 0x5A787E, 0x384A50, 0x2D2D2B
};

static const uint32 palette_colours_golden_wild[] = {
	0xB99F65, 0x816F46, 0x4A3F28, 0x120F0A
};

static const uint32 palette_colours_greenscale[] = {
	0x9CBE0C, 0x6E870A, 0x2C6234, 0x0C360C
};

static const uint32 palette_colours_hokage_orange[] = {
	0xEA8352, 0xA35B39, 0x5D3420, 0x170D08
};

static const uint32 palette_colours_labo_fawn[] = {
	0xD7AA73, 0x967650, 0x56442E, 0x15110B
};

static const uint32 palette_colours_legendary_super_saiyan[] = {
	0xA5DB5A, 0x73993E, 0x425724, 0x101509
};

static const uint32 palette_colours_microvision[] = {
	0xA0A0A0, 0x787878, 0x505050, 0x303030
};

static const uint32 palette_colours_million_live_gold[] = {
	0xCDB261, 0x8F7C43, 0x524726, 0x141109
};

static const uint32 palette_colours_odyssey_gold[] = {
	0xC2A000, 0x877000, 0x4D4000, 0x131000
};

static const uint32 palette_colours_shiny_sky_blue[] = {
	0x8CB6DF, 0x627F9C, 0x384859, 0x0E1216
};

static const uint32 palette_colours_slime_blue[] = {
	0x2F8CCC, 0x20628E, 0x123851, 0x040E14
};

static const uint32 palette_colours_ti_83[] = {
	0x9CA684, 0x727C5A, 0x464A35, 0x181810
};

static const uint32 palette_colours_travel_wood[] = {
	0xF8D8B0, 0xA08058, 0x705030, 0x482810
};

static const uint32 palette_colours_virtual_boy[] = {
	0xE30000, 0x950000, 0x560000, 0x000000
};

#define RGB24_TO_RGB565(RGB24) ( ((RGB24 >> 8) & 0xF800) | ((RGB24 >> 5) & 0x7E0) | ((RGB24 >> 3) & 0x1F) )
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_init(void)
{
	#ifdef DEBUG
	printf("Gpu Init\n");
	#endif
	//fprintf(log_get(), "gpu: init\n");
	memory_malloc_secure((void**)&supervision_palette,  4*sizeof(int16), "Palette");
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_done(void)
{
	//fprintf(log_get(), "gpu: done\n");
	memory_free(supervision_palette);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_reset(void)
{
	#ifdef DEBUG
	printf("Gpu Reset\n");
	#endif

#ifdef GP2X
	supervision_palette[3] = gp2x_video_RGB_color16(0,0,0);
	supervision_palette[2] = gp2x_video_RGB_color16(85,85,85);
	supervision_palette[1] = gp2x_video_RGB_color16(170,170,170);
	supervision_palette[0] = gp2x_video_RGB_color16(170,170,170);
#endif
#ifdef NDS
	supervision_palette[3] = RGB555(0,0,0);
	supervision_palette[2] = RGB555(10,10,10);
	supervision_palette[1] = RGB555(20,20,20);
	supervision_palette[0] = RGB555(30,30,30);
#endif 
#ifdef _WIN_
    supervision_palette[3] = RGB555(0,0,0);
	supervision_palette[2] = RGB555(10,10,10);
	supervision_palette[1] = RGB555(20,20,20);
	supervision_palette[0] = RGB555(30,30,30);
#endif

#ifdef _ODSDL_
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,85,85,85);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,170,170,170);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,240,240,240);
#endif
#ifdef _OPENDINGUX_
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,85,85,85);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,170,170,170);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,240,240,240);
#endif
#ifdef _RS97_
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,85,85,85);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,170,170,170);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,240,240,240);
#endif
#ifdef __LIBRETRO__
	supervision_palette[3] = RGB24_TO_RGB565(palette_colours_none[3]);
	supervision_palette[2] = RGB24_TO_RGB565(palette_colours_none[2]);
	supervision_palette[1] = RGB24_TO_RGB565(palette_colours_none[1]);
	supervision_palette[0] = RGB24_TO_RGB565(palette_colours_none[0]);
#endif
	memset(gpu_regs, 0, 4);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_set_colour_scheme(int colourScheme)
{
	#ifdef DEBUG
	printf("Gpu Set Color Scheme\n");
	#endif

#ifdef __LIBRETRO__
	const uint32 *palette_colours = palette_colours_none;

	switch (colourScheme)
	{
	case COLOUR_SCHEME_SUPERVISION:
		palette_colours = palette_colours_supervision;
		break;
	case COLOUR_SCHEME_GB_DMG:
		palette_colours = palette_colours_gb_dmg;
		break;
	case COLOUR_SCHEME_GB_POCKET:
		palette_colours = palette_colours_gb_pocket;
		break;
	case COLOUR_SCHEME_GB_LIGHT:
		palette_colours = palette_colours_gb_light;
		break;
	case COLOUR_SCHEME_BLOSSOM_PINK:
		palette_colours = palette_colours_blossom_pink;
		break;
	case COLOUR_SCHEME_BUBBLES_BLUE:
		palette_colours = palette_colours_bubbles_blue;
		break;
	case COLOUR_SCHEME_BUTTERCUP_GREEN:
		palette_colours = palette_colours_buttercup_green;
		break;
	case COLOUR_SCHEME_DIGIVICE:
		palette_colours = palette_colours_digivice;
		break;
	case COLOUR_SCHEME_GAME_COM:
		palette_colours = palette_colours_game_com;
		break;
	case COLOUR_SCHEME_GAMEKING:
		palette_colours = palette_colours_gameking;
		break;
	case COLOUR_SCHEME_GAME_MASTER:
		palette_colours = palette_colours_game_master;
		break;
	case COLOUR_SCHEME_GOLDEN_WILD:
		palette_colours = palette_colours_golden_wild;
		break;
	case COLOUR_SCHEME_GREENSCALE:
		palette_colours = palette_colours_greenscale;
		break;
	case COLOUR_SCHEME_HOKAGE_ORANGE:
		palette_colours = palette_colours_hokage_orange;
		break;
	case COLOUR_SCHEME_LABO_FAWN:
		palette_colours = palette_colours_labo_fawn;
		break;
	case COLOUR_SCHEME_LEGENDARY_SUPER_SAIYAN:
		palette_colours = palette_colours_legendary_super_saiyan;
		break;
	case COLOUR_SCHEME_MICROVISION:
		palette_colours = palette_colours_microvision;
		break;
	case COLOUR_SCHEME_MILLION_LIVE_GOLD:
		palette_colours = palette_colours_million_live_gold;
		break;
	case COLOUR_SCHEME_ODYSSEY_GOLD:
		palette_colours = palette_colours_odyssey_gold;
		break;
	case COLOUR_SCHEME_SHINY_SKY_BLUE:
		palette_colours = palette_colours_shiny_sky_blue;
		break;
	case COLOUR_SCHEME_SLIME_BLUE:
		palette_colours = palette_colours_slime_blue;
		break;
	case COLOUR_SCHEME_TI_83:
		palette_colours = palette_colours_ti_83;
		break;
	case COLOUR_SCHEME_TRAVEL_WOOD:
		palette_colours = palette_colours_travel_wood;
		break;
	case COLOUR_SCHEME_VIRTUAL_BOY:
		palette_colours = palette_colours_virtual_boy;
		break;
	case COLOUR_SCHEME_NONE:
	default:
		colourScheme = COLOUR_SCHEME_NONE;
		break;
	}

	supervision_palette[3] = RGB24_TO_RGB565(palette_colours[3]);
	supervision_palette[2] = RGB24_TO_RGB565(palette_colours[2]);
	supervision_palette[1] = RGB24_TO_RGB565(palette_colours[1]);
	supervision_palette[0] = RGB24_TO_RGB565(palette_colours[0]);
#else
	float greenf=1;
	float bluef=1;
	float redf=1;

	switch (colourScheme)
	{
	case COLOUR_SCHEME_DEFAULT:
		break;
	case COLOUR_SCHEME_AMBER:
		greenf=0.61f;
		bluef=0.00f;
		redf=1.00f;
		break;
	case COLOUR_SCHEME_GREEN:
		greenf=0.90f;
		bluef=0.20f;
		redf=0.20f;
		break;
	case COLOUR_SCHEME_BLUE:
		greenf=0.30f;
		bluef=0.75f;
		redf=0.30f;
		break;
	default: 
		colourScheme=0; 
		break;
	}
#ifdef GP2X
	supervision_palette[3] = gp2x_video_RGB_color16(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = gp2x_video_RGB_color16(85*redf,85*greenf,85*bluef);
	supervision_palette[1] = gp2x_video_RGB_color16(170*redf,170*greenf,170*bluef);
	supervision_palette[0] = gp2x_video_RGB_color16(255*redf,255*greenf,255*bluef);
#endif
#ifdef NDS
	supervision_palette[3] = RGB555(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = RGB555(10*redf,10*greenf,10*bluef);
	supervision_palette[1] = RGB555(20*redf,20*greenf,20*bluef);
	supervision_palette[0] = RGB555(30*redf,30*greenf,30*bluef);
#endif
#ifdef _WIN_
	supervision_palette[3] = RGB555(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = RGB555(10*redf,10*greenf,10*bluef);
	supervision_palette[1] = RGB555(20*redf,20*greenf,20*bluef);
	supervision_palette[0] = RGB555(30*redf,30*greenf,30*bluef);
#endif

#ifdef _ODSDL_
	int p11 = (int) 85*redf; int p12 = (int) 85*greenf; int p13 = (int) 85*bluef;
	int p21 = (int) 170*redf; int p22 = (int) 170*greenf; int p23 = (int) 170*bluef;
	int p31 = (int) 255*redf; int p32 = (int) 255*greenf; int p33 = (int) 255*bluef;
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,p11, p12, p13);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,p21, p22, p23);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,p31, p32, p33);
#endif
#ifdef _OPENDINGUX_
    	int p11 = (int) 85*redf; int p12 = (int) 85*greenf; int p13 = (int) 85*bluef;
	int p21 = (int) 170*redf; int p22 = (int) 170*greenf; int p23 = (int) 170*bluef;
	int p31 = (int) 255*redf; int p32 = (int) 255*greenf; int p33 = (int) 255*bluef;
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,p11, p12, p13);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,p21, p22, p23);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,p31, p32, p33);
#endif
#ifdef _RS97_
    	int p11 = (int) 85*redf; int p12 = (int) 85*greenf; int p13 = (int) 85*bluef;
	int p21 = (int) 170*redf; int p22 = (int) 170*greenf; int p23 = (int) 170*bluef;
	int p31 = (int) 255*redf; int p32 = (int) 255*greenf; int p33 = (int) 255*bluef;
    supervision_palette[3] = PIX_TO_RGB(actualScreen->format,0,0,0);
	supervision_palette[2] = PIX_TO_RGB(actualScreen->format,p11, p12, p13);
	supervision_palette[1] = PIX_TO_RGB(actualScreen->format,p21, p22, p23);
	supervision_palette[0] = PIX_TO_RGB(actualScreen->format,p31, p32, p33);
#endif
#endif
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_write(uint32 addr, uint8 data)
{
	gpu_regs[(addr&0x03)] = data;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
uint8 gpu_read(uint32 addr)
{
	return(gpu_regs[(addr&0x03)]);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_render_scanline(uint32 scanline, uint16 *backbuffer)
{
	uint8 *vram_line = &(memorymap_getUpperRamPointer())[(gpu_regs[2] >> 2) + (scanline*0x30)];
	uint8 x;

	for (x =0; x < 160; x += 4)
	{
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
	uint8 *vram_line = &(memorymap_getUpperRamPointer())[(gpu_regs[2] >> 2) + (scanline)];
	uint8 x;
	uint32 *buf = (uint32 *) backbuffer;
	
	for (x =0; x < 160; x += 4)
	{
		uint8 b = *(vram_line++);
		*(buf++) = (supervision_palette[((b >> 2) & 0x03)]<<16) | (supervision_palette[((b) & 0x03)]);
		*(buf++) = (supervision_palette[((b >> 6) & 0x03)]<<16) | (supervision_palette[((b >> 4) & 0x03)]);
	}
}
