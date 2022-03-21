/*
    Project: <https://github.com/AngelJMC/4-20ma-wifi-bridge>   
    Copyright (c) 2022 Angel Maldonado <angelgesus@gmail.com>. 
    Licensed under the MIT License: <http://opensource.org/licenses/MIT>.
    SPDX-License-Identifier: MIT 
*/

#ifndef __U_INTERFACE__
#define __U_INTERFACE__

#include "stdint.h"

#define LED_OFF 0x1
#define LED_ON 0x0

enum {  
    RELAY1 = 26, 
    RELAY2 = 27,
    SENS   = 34,
    LED1   = 0,
    LED2   = 15,
    SWITCH = 2 
};

/*Led status modes*/
enum modes {
    OFF = 0,
    BLINK = 1,
    ON = 2
};

/**
 * @brief Initialize the leds controls */
void interface_init( void );

/**
 * @brief Set the state of the led used as Mode indicator (RED)
 * @param mode, the state of the led: OFF, BLINK, ON */
void interface_setMode( enum modes mode );

/**
 * @brief Set the state of the led used as State indicator (GREEN)
 * @param mode, the state of the led: OFF, BLINK, ON */
void interface_setState( enum modes mode );

/**
 * @brief This function reads some ADC acquisitions and return the filtered value. */
int16_t getadcValue( void );

/**
 * @brief This function does a factory reset of the device. */
void factoryreset( void );

#endif //__U_INTERFACE__ 
