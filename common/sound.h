#ifndef __SOUND_H__
#define __SOUND_H__

#include "supervision.h"

#define BPS 44100

void sound_stream_update(uint8 *stream, int len);
void sound_decrement();
void soundport_w(int which, int offset, int data);
void svision_sounddma_w(int offset, int data);
void svision_noise_w(int offset, int data);

#endif
