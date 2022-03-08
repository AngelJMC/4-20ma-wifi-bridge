#ifndef __U_INTERFACE__
#define __U_INTERFACE__

#define LED_OFF 0x1
#define LED_ON 0x0

#include "stdint.h"

enum {  RELAY1 = 26, 
        RELAY2 = 27,
        SENS   = 34,
        LED1   = 0,
        LED2   = 15,
        SWITCH = 2 
    };


enum modes {
    OFF = 0,
    BLINK = 1,
    ON = 2
};


void interface_init( void );

void interface_setMode( enum modes mode );

void interface_setState( enum modes mode );

int16_t getadcValue( void );

void factoryreset( void );

#endif //__U_INTERFACE__ 
