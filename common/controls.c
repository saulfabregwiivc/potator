#include "controls.h"

#ifdef GP2X
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif

static uint8 controls_state;

void controls_reset(void)
{
    controls_state = 0;
}

void controls_state_write(uint8 type, uint8 data)
{
     if (controls_state == data)
         return; 
     else 
         controls_state = 0;

     if (type)
         controls_state |= data;
     else
         controls_state = data;
}

uint8 controls_read(void)
{
    return controls_state ^ 0xff; 
}

BOOL controls_update(void)
{
    controls_state = 0;

#if defined(GP2X)
    unsigned long pad = gp2x_joystick_read(0);

    if (pad & GP2X_UP)     controls_state|=0x08;
    if (pad & GP2X_RIGHT)  controls_state|=0x01;
    if (pad & GP2X_LEFT)   controls_state|=0x02;
    if (pad & GP2X_DOWN)   controls_state|=0x04;
    if (pad & GP2X_UP)     controls_state|=0x08;
    if (pad & GP2X_X)      controls_state|=0x10;
    if (pad & GP2X_A)      controls_state|=0x20;
    if (pad & GP2X_START)  controls_state|=0x80;
    if (pad & GP2X_SELECT) controls_state|=0x40;
#elif defined(NDS)
    if (!(REG_KEYINPUT & KEY_RIGHT))  controls_state|=0x01;
    if (!(REG_KEYINPUT & KEY_LEFT))   controls_state|=0x02;
    if (!(REG_KEYINPUT & KEY_DOWN))   controls_state|=0x04;
    if (!(REG_KEYINPUT & KEY_UP))     controls_state|=0x08;
    if (!(REG_KEYINPUT & KEY_B))      controls_state|=0x10;
    if (!(REG_KEYINPUT & KEY_A))      controls_state|=0x20;
    if (!(REG_KEYINPUT & KEY_SELECT)) controls_state|=0x40;
    if (!(REG_KEYINPUT & KEY_START))  controls_state|=0x80;
#endif

    return TRUE;
}
