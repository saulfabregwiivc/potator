#ifndef __SOUND_H__
#define __SOUND_H__

#include "supervision.h"

#define BPS 44100

void sound_reset(void);
/*!
 * Generate U8, 2 channels.
 * \param len in bytes.
 */
void sound_stream_update(uint8 *stream, int len);
void sound_decrement(void);
void sound_soundport_w(int which, int offset, int data);
void sound_sounddma_w(int offset, int data);
void sound_noise_w(int offset, int data);

#endif
