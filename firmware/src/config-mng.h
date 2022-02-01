
#ifndef _CONFIG_MNG_H_
#define _CONFIG_MNG_H_                   

#ifdef __cplusplus

//extern "C"{
//#endif

#include "Arduino.h"
#include <stdint.h>
#include <stdbool.h>

enum {
    SSID_SIZE = 32,
    PASS_SIZE = 32,
    APADDR_SIZE = 32
};

/** ip struct */
struct ip {
    uint8_t ip[4];
};

enum time_unit{
    TIME_UNIT_SECOND,
    TIME_UNIT_MINUTE,
    TIME_UNIT_HOUR
};

struct pub_topic {
    char topic[64];
    int32_t interval; /* in seconds */
    char unit[16];
};

struct sub_topic {
    char topic[64];
};

struct wifi_config {
    char ssid[SSID_SIZE];
    char pass[PASS_SIZE];
    char mode[16];
    struct ip ip;
    struct ip netmask;
    struct ip gateway;
    struct ip primaryDNS;
    struct ip secondaryDNS;
};

/** Create a structure to hold the configuration data. */
struct config  {
    char ssid[SSID_SIZE];
    char pass[PASS_SIZE];
    char apaddr[APADDR_SIZE];

    struct wifi_config wifi;

    struct service_config {
        char host_ip[SSID_SIZE];
        uint16_t port;
        char client_id[64];
        char username[32];
        char password[32];
        struct pub_topic temp;
        struct pub_topic ping;
        struct sub_topic relay1;
        struct sub_topic relay2;
        struct sub_topic enableTemp;
        uint8_t qos;
    } service;
};


extern struct config cfg;

void config_load( void ); 

void config_savecfg( );

void config_setdefault( void );
//#ifdef __cplusplus
//} // extern "C"
#endif

#endif 
 
