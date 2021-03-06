/*
    Project: <https://github.com/AngelJMC/4-20ma-wifi-bridge>   
    Copyright (c) 2022 Angel Maldonado <angelgesus@gmail.com>. 
    Licensed under the MIT License: <http://opensource.org/licenses/MIT>.
    SPDX-License-Identifier: MIT 
*/

#ifndef __CTRL_TASK__
#define __CTRL_TASK__


/**
 * @brief Freertos task to manage the WIFI and MQTT connections.
 * @param parameter */
void ctrl_task( void * parameter );


/**
 * @brief Enter in the configuration mode: enable wifi access point and webserver.*/
void ctrl_enterConfigMode( void );

/**
 * @brief Exit from the configuration mode: disable wifi access point and webserver.*/
void ctrl_exitConfigMode( void );

/**
 * @brief Check if the configuration mode is enabled.
 * @return true if the configuration mode is enabled, false otherwise. */
bool ctrl_isConfigModeEnable( void );


#endif //__CTRL_TASK__