/*
    Project: <https://github.com/AngelJMC/4-20ma-wifi-bridge>   
    Copyright (c) 2022 Angel Maldonado <angelgesus@gmail.com>. 
    Licensed under the MIT License: <http://opensource.org/licenses/MIT>.
    SPDX-License-Identifier: MIT 
*/

#ifndef __WEB_SERVER__
#define __WEB_SERVER__

#include "stdbool.h"

/**
 * @brief Freertos task to handle the web server
 * @param parameter */
void webserver_task( void * parameter );

/**
 * @brief Start the web server. */
void webserver_start( void );

/**
 * @brief Stop the web server. */
void webserver_stop( void );

/**
 * @brief Check if service configuration data has been updated from webserver.
 * It's clean the status flag if it has been updated. 
 * @return true configuration data has been updated, false otherwise. */
bool webserver_isServiceUpdated( void );

/**
 * @brief Check if calibration  data has been updated from webserver.
 * It's clean the status flag if it has been updated.
 * @return true calibration data has been updated, false otherwise. */
bool webserver_isCalibrationUpdated( void );

/**
 * @brief Check if network configuration has been updated from webserver.
 * It's clean the status flag if it has been updated.
 * @return true network configuration has been updated, false otherwise. */
bool webserver_isNetworkUpdated( void );

#endif //__LIGHT_CTRL__