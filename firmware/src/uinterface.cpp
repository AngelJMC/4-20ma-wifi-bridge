#include "uinterface.h"
#include "Arduino.h"
#include "FreeRTOS.h"




/*Create a static freertos timer*/
static TimerHandle_t tmledMode;
static TimerHandle_t tmledState; 


struct ledctrl {
    TimerHandle_t timer;
    uint8_t gpio;
    uint8_t state;
    enum modes mode;
};

static struct ledctrl ledMode;
static struct ledctrl ledState;

enum ledstates{
    AP_MODE = 0,
    STATION_MODE = 1
};




static void ledctrl_init( struct ledctrl * self, uint8_t gpio, TimerHandle_t tmhdl ) {
    self->gpio = gpio;
    self->mode = OFF;
    self->state = LED_OFF;
    self->timer = tmhdl;
    digitalWrite(self->gpio, self->state);
}

static void ledctrl_update( struct ledctrl * self, enum modes mode ) {
    
    if( self->mode == mode ) {
        return;
    }

    self->mode = mode;       
    switch (self->mode) {
        case OFF:
            self->state = LED_OFF;
            xTimerStop( self->timer, pdMS_TO_TICKS( 5 ) );
            digitalWrite(self->gpio, self->state);
            break;
        case BLINK:
            self->state = LED_OFF;
            digitalWrite(self->gpio, self->state);
            xTimerStart( self->timer, pdMS_TO_TICKS( 5 ) );
            break;
        case ON:
            self->state = LED_ON;
            xTimerStop( self->timer, pdMS_TO_TICKS( 5 ) );
            digitalWrite(self->gpio, self->state);
            break;
        default:
            break;
    }


}


static void tmledmode_callback( TimerHandle_t xTimer ) {
    ledMode.state = ledMode.state == LOW ? HIGH : LOW;
    digitalWrite(ledMode.gpio, ledMode.state);
}

static void tmledstate_callback( TimerHandle_t xTimer ) {
    ledState.state = ledState.state == LOW ? HIGH : LOW;
    digitalWrite(ledState.gpio, ledState.state);
}



void interface_init( void ) {
    
    tmledState = xTimerCreate( "ledState", pdMS_TO_TICKS( 250 ), pdTRUE, NULL, tmledstate_callback );
    ledctrl_init( &ledState, LED2, tmledState );

    tmledMode = xTimerCreate( "ledMode",   pdMS_TO_TICKS( 250 ), pdTRUE, NULL, tmledmode_callback );
    ledctrl_init( &ledMode, LED1, tmledMode );




}

void interface_setMode( enum modes mode ) {
    ledctrl_update( &ledMode, mode );
}

void interface_setState( enum modes mode ) {
    ledctrl_update( &ledState, mode );
}

