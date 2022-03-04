#ifndef __MQTT_TASK__
#define __MQTT_TASK__

void updateServiceCfg( void );

void mqtt_task( void * parameter );

void toggleMode( void );

void updateCalibration( void );

void factoryreset( void );

#endif //__MQTT_TASK__