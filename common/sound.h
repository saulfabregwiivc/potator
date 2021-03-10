#ifndef __SOUND_H__
#define __SOUND_H__

#include "supervision.h"

#define BPS			44100

typedef struct {
	unsigned char reg[4];
	int on;
	int waveform, volume;
	int pos;
	int size;
	int count;
} SVISION_CHANNEL;

typedef enum  {
	SVISION_NOISE_Type7Bit,
	SVISION_NOISE_Type14Bit
} SVISION_NOISE_Type;

typedef struct  {
	unsigned char reg[3];
	int on, right, left, play;
	SVISION_NOISE_Type type;
	int state;
	int volume;
	int count;
	double step, pos;
	int value; // currently simple random function
} SVISION_NOISE;

typedef struct  {
	unsigned char reg[5];
	int on, right, left;
	int ca14to16;
	int start,size;
	double pos, step;
	int finished;
} SVISION_DMA;

void sound_init();
void sound_reset();
void sound_done();
void sound_write(uint32 Addr, uint8 data);
void sound_noise_write(uint32 Addr, uint8 data);
void sound_audio_dma(uint32 Addr, uint8 data);
void sound_exec(uint32 cycles);
void sound_stream_update(uint8 *stream, int len);
void sound_decrement(void);
void audio_turnSound(BOOL bOn);

void soundport_w(int which, int offset, int data);
void svision_noise_w(int offset, int data);
void svision_sounddma_w(int offset, int data);

extern SVISION_CHANNEL m_channel[2];
extern SVISION_NOISE m_noise;
extern SVISION_DMA m_dma;

#endif
