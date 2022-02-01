#ifndef __WEB_SERVER__
#define __WEB_SERVER__

void webserver_task( void * parameter );

void ipAdress(String& eap, String& iip1, String& iip2, String& iip3, String& iip4);

void printIp(struct ip* src);

#endif //__LIGHT_CTRL__