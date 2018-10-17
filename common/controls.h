#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "supervision.h"

#ifdef __cplusplus
extern "C" {
#endif

void controls_reset(void);
uint8 controls_read(void);
BOOL controls_update(void);
void controls_state_write(uint8 type, uint8 data);

#ifdef __cplusplus
}
#endif

#endif
