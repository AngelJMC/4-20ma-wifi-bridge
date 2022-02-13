#ifndef __WEB_SERVER__
#define __WEB_SERVER__

void webserver_task( void * parameter );

void ipAdress(String& eap, String& iip1, String& iip2, String& iip3, String& iip4);

void printIp(struct ip* src);

void stringToIp(struct ip* dest, String src);

void print_ntpCfg( struct ntp_config const* ntp );

void print_NetworkCfg( struct wifi_config const* ntwk );

void printIp( char const* name, struct ip const* src );

#endif //__LIGHT_CTRL__