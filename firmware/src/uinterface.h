#ifndef __U_INTERFACE__
#define __U_INTERFACE__

#define LED_OFF 0x1
#define LED_ON 0x0

enum {  RELAY1 = 26, 
        RELAY2 = 27,
        SENS   = 36,
        LED1   = 0,
        LED2   = 2,
        SWITCH = 15 
    };


enum modes {
    OFF = 0,
    BLINK = 1,
    ON = 2
};


void interface_init( void );


void interface_setMode( enum modes mode );
void interface_setState( enum modes mode );



#endif //__U_INTERFACE__ 
