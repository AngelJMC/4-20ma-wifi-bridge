
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
    int32_t period; /* in seconds */
    char unit[16];
};

struct sub_topic {
    char topic[64];
};

struct wifi_config {
    char ssid[32];
    char pass[32];
    char mode[16];
    struct ip ip;
    struct ip netmask;
    struct ip gateway;
    struct ip primaryDNS;
    struct ip secondaryDNS;
};

struct ntp_config {
    char host[64];
    int  port;
    int  period;
};

struct service_config {
    char host_ip[64];
    uint16_t port;
    char client_id[32];
    char username[32];
    char password[32];
    struct pub_topic temp;
    struct pub_topic ping;
    struct sub_topic relay1;
    struct sub_topic relay2;
    struct sub_topic enableTemp;
};

struct ap_config {
    char ssid[32];
    char pass[32];
    struct ip addr;
    char web_user[16];
    char web_pass[16];
};

/** Create a structure to hold the configuration data. */
struct config  {
    struct ap_config ap;
    struct wifi_config wifi;
    struct service_config  service;
    struct ntp_config ntp;
};


extern struct config cfg;

void config_load( void ); 

void config_savecfg( );

void config_setdefault( void );

void print_ntpCfg( struct ntp_config const* ntp );

void print_NetworkCfg( struct wifi_config const* ntwk );

void printIp( char const* name, struct ip const* src );

void print_apCfg( struct ap_config const* ap );

void print_ServiceCfg( struct service_config const* srvc );

//#ifdef __cplusplus
//} // extern "C"
#endif

#endif 
 
