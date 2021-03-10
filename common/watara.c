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

static M6502	m6502_registers;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
byte Loop6502(register M6502 *R)
{
	return(INT_QUIT);
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
void supervision_init(void)
{
	//fprintf(log_get(), "supervision: init\n");
	#ifndef DEBUG
	//iprintf("supervision: init\n");
	#endif

	memorymap_init();
	io_init();
	gpu_init();
	timer_init();
	controls_init();
	sound_init();
	interrupts_init();
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
BOOL supervision_load(uint8 *rom, uint32 romSize)
{
	//uint32 supervision_programRomSize;
	//uint8 *supervision_programRom = memorymap_rom_load(szPath, &supervision_programRomSize);
	#ifdef DEBUG
	//iprintf("supervision: load\n");
	#endif

	memorymap_load(rom, romSize);
	supervision_reset();

	return(TRUE);
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
void supervision_reset(void)
{
	//fprintf(log_get(), "supervision: reset\n");


	memorymap_reset();
	io_reset();
	gpu_reset();
	timer_reset();
	controls_reset();
	sound_reset();
	interrupts_reset();

	Reset6502(&m6502_registers);
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
void supervision_reset_handler(void)
{
	//fprintf(log_get(), "supervision: reset handler\n");
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
void supervision_done(void)
{
	//fprintf(log_get(), "supervision: done\n");
	memorymap_done();
	io_done();
	gpu_done();
	timer_done();
	controls_done();
	sound_done();
	interrupts_done();
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
void supervision_set_colour_scheme(int sv_colourScheme)
{
	gpu_set_colour_scheme(sv_colourScheme);
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
M6502	*supervision_get6502regs(void)
{
	return(&m6502_registers);
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
BOOL supervision_update_input(void)
{
	return(controls_update());
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
void supervision_exec(uint16 *backbuffer, BOOL bRender)
{
	uint32 supervision_scanline, scan1=0;

	for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++)
	{
		m6502_registers.ICount = 512; 
		timer_exec(m6502_registers.ICount);
#ifdef GP2X
		if(currentConfig.enable_sound) sound_exec(11025/160);
#else
		//sound_exec(22050/160);
#endif
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
	}

	if (Rd6502(0x2026)&0x01)
		Int6502(supervision_get6502regs(), INT_NMI);
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
void supervision_turnSound(BOOL bOn)
{
	audio_turnSound(bOn);
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

int	sv_loadState(char *statepath, int id)
{
	FILE* fp;
	char newPath[256];

	strcpy(newPath,statepath);
	sprintf(newPath+strlen(newPath)-3,".s%d",id);

#ifdef GP2X
	gp2x_printf(0,10,220,"newPath = %s",newPath);
	gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
	iprintf("\nnewPath = %s",newPath);
#endif

	fp=fopen(newPath,"rb");

	if (fp) {
		fread(&m6502_registers, 1, sizeof(m6502_registers), fp);
		fread(memorymap_programRom, 1, sizeof(memorymap_programRom), fp);
		fread(memorymap_lowerRam, 1, 0x2000, fp);
		fread(memorymap_upperRam, 1, 0x2000, fp);
		fread(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
		fread(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
		fread(memorymap_regs, 1, 0x2000, fp);
		fclose(fp);
	}

#ifdef GP2X
	sleep(1);
#endif

	return(1);
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

int	sv_saveState(char *statepath, int id)
{
	FILE* fp;
	char newPath[256];

	strcpy(newPath,statepath);
	sprintf(newPath+strlen(newPath)-3,".s%d",id);

#ifdef GP2X
	gp2x_printf(0,10,220,"newPath = %s",newPath);
	gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
	iprintf("\nnewPath = %s",newPath);
#endif

	fp=fopen(newPath,"wb");

	if (fp) {
		fwrite(&m6502_registers, 1, sizeof(m6502_registers), fp);
		fwrite(memorymap_programRom, 1, sizeof(memorymap_programRom), fp);
		fwrite(memorymap_lowerRam, 1, 0x2000, fp);
		fwrite(memorymap_upperRam, 1, 0x2000, fp);
		fwrite(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
		fwrite(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
		fwrite(memorymap_regs, 1, 0x2000, fp);
		fflush(fp);
		fclose(fp);
#ifdef GP2X
		sync();
#endif
	}

#ifdef GP2X
	sleep(1);
#endif

	return(1);
}

uint32 sv_saveStateBufSize(void)
{
	return sizeof(m6502_registers) +       /* m6502_registers */
			 0x2000 +                        /* memorymap_lowerRam */
			 0x2000 +                        /* memorymap_upperRam */
			 0x2000 +                        /* memorymap_regs */
			 sizeof(uint64) +                /* lower_rom_bank_offset */
			 sizeof(uint64) +                /* upper_rom_bank_offset */
			 (sizeof(uint8) * 2) +           /* timer_regs */
			 sizeof(int32) +                 /* timer_cycles */
			 sizeof(BOOL) +                  /* timer_activated */
			 (sizeof(uint8) * 4) +           /* gpu_regs */
			 (sizeof(SVISION_CHANNEL) * 2) + /* m_channel */
			 sizeof(SVISION_NOISE) +         /* m_noise */
			 sizeof(SVISION_DMA) +           /* m_dma */
			 sizeof(uint8);                  /* io_data */
}


BOOL sv_loadStateBuf(const void *data, uint32 size)
{
	uint8 *buffer = (uint8*)data;
	uint64 lower_rom_bank_offset;
	uint64 upper_rom_bank_offset;
	uint8 io_data;

	if (!buffer || (size < sv_saveStateBufSize()))
		return FALSE;

	memcpy(&m6502_registers, buffer, sizeof(m6502_registers));
	buffer += sizeof(m6502_registers);

	memcpy(memorymap_lowerRam, buffer, 0x2000);
	buffer += 0x2000;

	memcpy(memorymap_upperRam, buffer, 0x2000);
	buffer += 0x2000;

	memcpy(memorymap_regs, buffer, 0x2000);
	buffer += 0x2000;

	memcpy(&lower_rom_bank_offset, buffer, sizeof(uint64));
	buffer += sizeof(uint64);

	memcpy(&upper_rom_bank_offset, buffer, sizeof(uint64));
	buffer += sizeof(uint64);

	memcpy(timer_regs, buffer, sizeof(uint8) * 2);
	buffer += sizeof(uint8) * 2;

	memcpy(&timer_cycles, buffer, sizeof(int32));
	buffer += sizeof(int32);

	memcpy(&timer_activated, buffer, sizeof(BOOL));
	buffer += sizeof(BOOL);

	memcpy(gpu_regs, buffer, sizeof(uint8) * 4);
	buffer += sizeof(uint8) * 4;

	memcpy(m_channel, buffer, sizeof(SVISION_CHANNEL) * 2);
	buffer += sizeof(SVISION_CHANNEL) * 2;

	memcpy(&m_noise, buffer, sizeof(SVISION_NOISE));
	buffer += sizeof(SVISION_NOISE);

	memcpy(&m_dma, buffer, sizeof(SVISION_DMA));
	buffer += sizeof(SVISION_DMA);

	memcpy(&io_data, buffer, sizeof(uint8));

	memorymap_lowerRomBank = memorymap_programRom + lower_rom_bank_offset;
	memorymap_upperRomBank = memorymap_programRom + upper_rom_bank_offset;

	io_write(0, io_data);

	return TRUE;
}

BOOL sv_saveStateBuf(void *data, uint32 size)
{
	uint8 *buffer                = (uint8*)data;
	uint64 lower_rom_bank_offset = memorymap_lowerRomBank - memorymap_programRom;
	uint64 upper_rom_bank_offset = memorymap_upperRomBank - memorymap_programRom;
	uint8 io_data                = io_read(0);

	if (!buffer || (size < sv_saveStateBufSize()))
		return FALSE;

	memcpy(buffer, &m6502_registers, sizeof(m6502_registers));
	buffer += sizeof(m6502_registers);

	memcpy(buffer, memorymap_lowerRam, 0x2000);
	buffer += 0x2000;

	memcpy(buffer, memorymap_upperRam, 0x2000);
	buffer += 0x2000;

	memcpy(buffer, memorymap_regs, 0x2000);
	buffer += 0x2000;

	memcpy(buffer, &lower_rom_bank_offset, sizeof(uint64));
	buffer += sizeof(uint64);

	memcpy(buffer, &upper_rom_bank_offset, sizeof(uint64));
	buffer += sizeof(uint64);

	memcpy(buffer, timer_regs, sizeof(uint8) * 2);
	buffer += sizeof(uint8) * 2;

	memcpy(buffer, &timer_cycles, sizeof(int32));
	buffer += sizeof(int32);

	memcpy(buffer, &timer_activated, sizeof(BOOL));
	buffer += sizeof(BOOL);

	memcpy(buffer, gpu_regs, sizeof(uint8) * 4);
	buffer += sizeof(uint8) * 4;

	memcpy(buffer, m_channel, sizeof(SVISION_CHANNEL) * 2);
	buffer += sizeof(SVISION_CHANNEL) * 2;

	memcpy(buffer, &m_noise, sizeof(SVISION_NOISE));
	buffer += sizeof(SVISION_NOISE);

	memcpy(buffer, &m_dma, sizeof(SVISION_DMA));
	buffer += sizeof(SVISION_DMA);

	memcpy(buffer, &io_data, sizeof(uint8));

	return TRUE;
}
