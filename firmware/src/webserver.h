#ifndef __WEB_SERVER__
#define __WEB_SERVER__

#include "stdbool.h"

/**
 * @brief 
 * 
 * @param mode 
 */
void webserver_task( void * parameter );

/**
 * @brief 
 * 
 * @param mode 
 */
void webserver_start( void );

/**
 * @brief 
 * 
 */
void webserver_stop( void );

/**
 * @brief 
 * 
 * @param mode 
 */
void webserver_toggleState( void );

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool webserver_isServiceUpdated( void );

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool webserver_isCalibrationUpdated( void );

#endif //__LIGHT_CTRL__