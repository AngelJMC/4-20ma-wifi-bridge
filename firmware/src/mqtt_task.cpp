#include <WiFi.h>
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "webserver.h"
#include "config-mng.h"
#include "mqtt_task.h"
#include "time.h"

enum {
    debug = 0
};

enum {  RELAY1 = 26, 
        RELAY2 = 27,
        SENS   = 36,
        LED1   = 8,
        LED2   = 7,
        SWITCH = 6 
    };

WiFiClient wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;

    /* An array of StaticTimer_t structures, which are used to store
    the state of each created timer. */
StaticTimer_t xTimerBuffers[ 2 ];  

/*Create a static freertos timer*/
static TimerHandle_t tmTemp;
static TimerHandle_t tmPing;

/* Declare a variable to hold the created event group. */
static EventGroupHandle_t events;
static struct service_config scfg;
enum flags {
    START_AP_WIFI = 1 << 0,
    CONNECT_WIFI  = 1 << 1,
    CONNECT_MQTT  = 1 << 2,
};

static void printLocalTime(){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print("Day of week: ");
    Serial.println(&timeinfo, "%A");
    Serial.print("Month: ");
    Serial.println(&timeinfo, "%B");
    Serial.print("Day of Month: ");
    Serial.println(&timeinfo, "%d");
    Serial.print("Year: ");
    Serial.println(&timeinfo, "%Y");
    Serial.print("Hour: ");
    Serial.println(&timeinfo, "%H");
    Serial.print("Hour (12 hour format): ");
    Serial.println(&timeinfo, "%I");
    Serial.print("Minute: ");
    Serial.println(&timeinfo, "%M");
    Serial.print("Second: ");
    Serial.println(&timeinfo, "%S");

    Serial.println("Time variables");
    char timeHour[3];
    strftime(timeHour,3, "%H", &timeinfo);
    Serial.println(timeHour);
    char timeWeekDay[10];
    strftime(timeWeekDay,10, "%A", &timeinfo);
    Serial.println(timeWeekDay);
    Serial.println();
}


static void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        status = WiFi.status();
        if ( status != WL_CONNECTED) {
            WiFi.begin( cfg.wifi.ssid, cfg.wifi.pass);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }
            Serial.println("Connected to AP");
        }
        Serial.print("Connecting broker ...");


        if (client.connect( cfg.service.client_id, cfg.service.username, cfg.service.password )) {
            Serial.println( "[DONE]" );
        } 
        else {
            Serial.print( "[FAILED] [ rc = " );
            Serial.print( client.state() );
            Serial.println( " : retrying in 5 seconds]" );
            // Wait 5 seconds before retrying
            delay( 4000 );
        }
    }
}

//####################################### testing wifi connection function ############################


static void tmtemp_callback( TimerHandle_t xTimer ) {
    struct service_config const* sc = &scfg;
    char payload[32];
    sprintf(payload, "%d", analogRead(SENS)); 
    client.publish( sc->temp.topic, payload);
    Serial.printf("Publishing temperature %s\n", payload);          
}

static void tmping_callback( TimerHandle_t xTimer ) {
    struct service_config const* sc = &scfg;
    char payload[32];
    time_t now;
    time(&now);
    sprintf(payload, "%ld", now); 
    client.publish( sc->ping.topic, payload );
    Serial.printf("Publishing timestamp %s\n", payload);
    
    if( debug ) {
        printLocalTime();
    }
}

/*Funtion to convert byte* to char* */
static void byteToChar(char* dest, byte* src, int len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
}

static void callback(char* topic, byte *payload, unsigned int length) {

    
    char value[length+1];
    byteToChar(value, payload, length);
    Serial.printf("Topic: %s, value: %s\n", topic, value);
    
    struct service_config const* sc = &scfg;
    if( strcmp(sc->relay1.topic , topic) == 0 ) { 
        if ( strcmp( value, "ON") == 0 )
            digitalWrite(RELAY1, HIGH);
        else if ( strcmp( value, "OFF") == 0 )
            digitalWrite(RELAY1, LOW);
        else
            Serial.println("Invalid value relay 1");
    }
    else if( strcmp(sc->relay2.topic, topic) == 0 ) {
        if ( strcmp( value, "ON") == 0 )
            digitalWrite(RELAY2, HIGH);
        else if ( strcmp( value, "OFF") == 0 )
            digitalWrite(RELAY2, LOW);
        else
            Serial.println("Invalid value relay 2");
    }
    else if( strcmp(sc->enableTemp.topic, topic) == 0 ) {
        if ( strcmp( value, "ON") == 0 )
            xTimerStart(tmTemp, pdMS_TO_TICKS(10) );
        else if ( strcmp( value, "OFF") == 0 )
            xTimerStop(tmTemp, pdMS_TO_TICKS(10) );
        else
            Serial.println("Invalid value enable");
    }
    else{
        Serial.println("unknown topic");
    }
}

static bool testWifi( void ) {
    int tries = 0;
    Serial.println("Waiting for Wifi to connect");
    while ( tries < 30 ) {
        if ( WiFi.status() == WL_CONNECTED ) {
            return true;
        }
        delay(500);
        Serial.print( WiFi.status() );
        Serial.print(".");
        ++tries;
    }
return false;
}



void updateServiceCfg( void ) {
    xEventGroupSetBits( events,   CONNECT_MQTT );
}

static int getupdatePeriod( struct pub_topic const* tp ) {
    struct { char const* unit; int factor; } const units[] = {
        { "Second", 1 * 1000 },
        { "Minute", 60 * 1000 },
        { "Hour", 60 * 60 * 1000}
    };

    for( int i = 0; i < sizeof(units)/sizeof(units[0]); ++i ) {
        if ( strcmp( tp->unit, units[i].unit ) == 0 ) {
            return tp->period * units[i].factor;
        }
    }
    return -1;
}



void mqtt_task( void * parameter ) {


    
    tmTemp = xTimerCreateStatic( "temperature", pdMS_TO_TICKS( 10000 ), pdTRUE, NULL, tmtemp_callback, &xTimerBuffers[0] );
    tmPing = xTimerCreateStatic( "ping", pdMS_TO_TICKS( 10000 ), pdTRUE, NULL, tmping_callback, &xTimerBuffers[1] );

    /* Attempt to create the event group. */
    events = xEventGroupCreate();
    EventBits_t bitfied = xEventGroupSetBits( events, START_AP_WIFI | CONNECT_WIFI );
    

    for(;;){ // infinite loop
        
        
        bitfied = xEventGroupGetBits( events );

        if( bitfied & START_AP_WIFI ) {
            
            struct ap_config* ap = &cfg.ap;
            IPAddress ip(ap->addr.ip[0], ap->addr.ip[1], ap->addr.ip[2], ap->addr.ip[3]);
            IPAddress nmask(255, 255, 0, 0);

            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP( ap->ssid, ap->pass);
            WiFi.softAPConfig(ip, ip, nmask );
            vTaskDelay( pdMS_TO_TICKS(800) );
            
            String myIP = WiFi.softAPIP().toString();
            Serial.printf("Set up access point. SSID: %s, PASS: %s\n", ap->ssid, ap->pass);
            Serial.printf("#### SERVER STARTED ON THIS: %s ###\n", myIP.c_str());
            printIp( "Access point ADDRESS: ", &ap->addr );

            xEventGroupClearBits( events, START_AP_WIFI );

        }

        if( bitfied & CONNECT_WIFI ) {
            
            struct wifi_config* wf = &cfg.wifi;
            
            if( (wf->ssid[0] == 0 ) || wf->mode[0] == 0 ) {
                Serial.println("No wifi config found");
                vTaskDelay( pdMS_TO_TICKS(10000) );
                xEventGroupClearBits( events, CONNECT_WIFI );
                continue;
            }
            
            if ( strcmp( wf->mode, "static") == 0 ) {
                IPAddress addr( wf->ip.ip[0], wf->ip.ip[1], wf->ip.ip[2], wf->ip.ip[3] );
                IPAddress gateway( wf->gateway.ip[0], wf->gateway.ip[1], wf->gateway.ip[2], wf->gateway.ip[3] );
                IPAddress subnet( wf->netmask.ip[0], wf->netmask.ip[1], wf->netmask.ip[2], wf->netmask.ip[3] );
                IPAddress primaryDNS( wf->primaryDNS.ip[0], wf->primaryDNS.ip[1], wf->primaryDNS.ip[2], wf->primaryDNS.ip[3] ); //optional
                IPAddress secondaryDNS( wf->secondaryDNS.ip[0], wf->secondaryDNS.ip[1], wf->secondaryDNS.ip[2], wf->secondaryDNS.ip[3] ); //optional
                if (!WiFi.config( addr, gateway, subnet, primaryDNS, secondaryDNS)) {
                    Serial.println("STA Failed to configure");
                }
            }
            
            print_NetworkCfg( wf );
            WiFi.begin( wf->ssid, wf->pass );
            if ( !testWifi() ) {
                Serial.println("Wifi connection failed");
                vTaskDelay( pdMS_TO_TICKS(1000) );
                continue;
            }
            
            WiFi.mode(WIFI_STA);
            Serial.printf("Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str() );            
            xEventGroupSetBits( events,   CONNECT_MQTT );
            xEventGroupClearBits( events, CONNECT_WIFI );
        }

        if( bitfied & CONNECT_MQTT ) {
            memcpy( &scfg, &cfg.service, sizeof(cfg.service) );
            
            if( scfg.host_ip[0] == 0 || scfg.client_id == 0 ) {
                Serial.println("No mqtt config found");
                vTaskDelay( pdMS_TO_TICKS(10000) );
                xEventGroupClearBits( events, CONNECT_MQTT );
                continue;
            }
            
            client.setServer( scfg.host_ip, scfg.port);
            client.setCallback(callback);
            
            Serial.print("Attempting MQTT connection...");
            // Attempt to connect
            if ( client.connect( scfg.client_id, scfg.username, scfg.password ) ) {
                Serial.println("connected");
                
                //Once connected, publish an announcement...
                //client.publish( scfg.temp.topic, "temperature");
                //client.publish( scfg.ping.topic, "timestamp");
                
                client.subscribe( scfg.relay1.topic);
                client.subscribe( scfg.relay2.topic);
                client.subscribe( scfg.enableTemp.topic);

                Serial.printf("Subscribed to: %s\n, %s\n, %s\n", 
                    scfg.relay1.topic, scfg.relay2.topic, scfg.enableTemp.topic );
                
                if( xTimerChangePeriod( tmTemp, pdMS_TO_TICKS( getupdatePeriod( &scfg.temp )), 100 ) != pdPASS ) {
                    Serial.println("Failed to change period");
                    continue;
                }
                else {
                    if( xTimerStop( tmPing, 0 ) != pdPASS ) {
                        Serial.println("Failed to stop ping timer");
                        continue;
                    }
                }

                if( xTimerChangePeriod( tmPing, pdMS_TO_TICKS( getupdatePeriod( &scfg.ping )), 100 ) != pdPASS ) {
                    Serial.println("Failed to change period");
                    continue;
                }
                else {
                    if( xTimerStart( tmPing, 0 ) != pdPASS ) {
                        Serial.println("Failed to start timer 2");
                        continue;
                    }
                }
            } 
            else {
                Serial.printf("failed, rc= %d %s\n", client.state(), "try again in 5 seconds");
                // Wait 5 seconds before retrying
                vTaskDelay( pdMS_TO_TICKS(5000) );
                continue;
            }
            
            // Init and get the time
            const long  gmtOffset_sec = 3600;
            const int   daylightOffset_sec = 3600;
            configTime(gmtOffset_sec, daylightOffset_sec, cfg.ntp.host);
            xEventGroupClearBits( events, CONNECT_MQTT );
        }
        
        if( ( WiFi.status() != WL_CONNECTED ) ) {
            xEventGroupSetBits( events, CONNECT_WIFI );
            vTaskDelay( pdMS_TO_TICKS(1000) );
            continue;
        }
        
        //if ( !client.connected() ) {
        //    reconnect();
        //}
        
        client.loop();
        vTaskDelay( pdMS_TO_TICKS(250) );
    }
}




