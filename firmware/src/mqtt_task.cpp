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
#include "uinterface.h"

enum {
    verbose = 1
};

/*Calibration equation*/
struct caleq {
    double m;
    double b;
};

WiFiClient wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;



/*Create a static freertos timer*/
static TimerHandle_t tmTemp;
static TimerHandle_t tmPing;

static bool isServerActive = false;

/* Declare a variable to hold the created event group. */
static EventGroupHandle_t events;
static struct service_config scfg;
static struct caleq eq = { .m = 1.0, .b = 0.0 };

enum flags {
    START_AP_WIFI = 1 << 0,
    CONNECT_WIFI  = 1 << 1,
    CONNECT_MQTT  = 1 << 2,
    UPDATE_CAL    = 1 << 3
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

void print_APcfg( struct ap_config const* ap ) {
    String myIP = WiFi.softAPIP().toString();
    Serial.printf("Set up access point. SSID: %s, PASS: %s\n", ap->ssid, ap->pass);
    Serial.printf("#### SERVER STARTED ON THIS: %s ###\n", myIP.c_str());
    printIp( "Access point ADDRESS: ", &ap->addr );
}


void enterConfigMode( void ) {
    

    WiFi.mode(WIFI_AP_STA);
    vTaskDelay(pdMS_TO_TICKS(1000));
    isServerActive = true;
    webserver_start( );
    
}

void exitConfigMode( void ) {
    webserver_stop( );
    WiFi.mode(WIFI_STA);
    isServerActive = false;
}

void toggleMode( void ) {
    if( isServerActive ) {
        exitConfigMode( );
    }
    else {
        enterConfigMode( );
    }
}




/*Calculate the equation of a straight line used to calibrate*/
static void getCalibrationEquation( struct caleq* eq, double x0, double y0, double x1, double y1 ) {
    double num = 0 == (x1 - x0 ) ? 1 : (x1 - x0 );
    eq->m = (y1 - y0) / num;
    eq->b = y0 - eq->m * x0;
    if( verbose ) {
        Serial.printf("Calibration equation: y = %.2f * x + %.2f\n", eq->m, eq->b);
    }
}

static double applyCalibration( struct caleq* eq, double x ) {
    return eq->m * x + eq->b;
} 

static void tmtemp_callback( TimerHandle_t xTimer ) {
    struct service_config const* sc = &scfg;
    char payload[32];
    int16_t raw = analogRead(SENS);
    double converted = applyCalibration( &eq, raw );
    sprintf(payload, "%d, %f",raw, converted); 
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
    
    if( verbose ) {
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
            xTimerStart(tmTemp, 100 );
        else if ( strcmp( value, "OFF") == 0 )
            xTimerStop(tmTemp, 100 );
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

void updateCalibration( void ) {
    xEventGroupSetBits( events,   UPDATE_CAL );
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


    interface_setMode( OFF );
    tmTemp = xTimerCreate( "temperature", pdMS_TO_TICKS( 10000 ), pdTRUE, NULL, tmtemp_callback );
    tmPing = xTimerCreate( "ping", pdMS_TO_TICKS( 10000 ), pdTRUE, NULL, tmping_callback );

    struct acq_cal const* cal = &cfg.cal;
    getCalibrationEquation( &eq, cal->val[0].x, cal->val[0].y, cal->val[1].x, cal->val[1].y );
    /* Attempt to create the event group. */
    events = xEventGroupCreate();
    EventBits_t bitfied = xEventGroupSetBits( events, START_AP_WIFI | CONNECT_WIFI );

    
    
    for(;;){ // infinite loop
        
        bitfied = xEventGroupGetBits( events );

        if( bitfied & START_AP_WIFI ) {
            interface_setMode( OFF );
            struct ap_config const* ap  = &cfg.ap;
            IPAddress ip(ap->addr.ip[0], ap->addr.ip[1], ap->addr.ip[2], ap->addr.ip[3]);
            IPAddress nmask(255, 255, 0, 0);
            WiFi.softAP( ap->ssid, ap->pass);
            WiFi.softAPConfig(ip, ip, nmask );

            if( verbose )
                print_APcfg( ap );
            
            enterConfigMode( );
            xEventGroupClearBits( events, START_AP_WIFI );
        }

        if( bitfied & CONNECT_WIFI ) {
            interface_setMode( OFF );
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
            
            if( verbose )
                print_NetworkCfg( wf );
            
            WiFi.begin( wf->ssid, wf->pass );
            if ( !testWifi() ) {
                Serial.println("Wifi connection failed");
                vTaskDelay( pdMS_TO_TICKS(1000) );
                continue;
            }
            
            exitConfigMode(  );
            Serial.printf("Connected to %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str() );            
            interface_setMode( BLINK );
            xEventGroupSetBits( events,   CONNECT_MQTT );
            xEventGroupClearBits( events, CONNECT_WIFI );
        }

        if( bitfied & UPDATE_CAL ) {
            struct acq_cal const* cal = &cfg.cal;
            getCalibrationEquation( &eq, cal->val[0].x, cal->val[0].y, cal->val[1].x, cal->val[1].y );
            xEventGroupClearBits( events, UPDATE_CAL );
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
                client.subscribe( scfg.relay1.topic);
                client.subscribe( scfg.relay2.topic);
                client.subscribe( scfg.enableTemp.topic);
                
                xTimerChangePeriod( tmTemp, pdMS_TO_TICKS( getupdatePeriod( &scfg.temp )), 100 );
                xTimerStop( tmTemp, 100 );
                xTimerChangePeriod( tmPing, pdMS_TO_TICKS( getupdatePeriod( &scfg.ping )), 100 );
                xTimerStart( tmPing, 100 );
                
                if (verbose ) {
                    Serial.println("Connected to broker");
                    Serial.printf("Subscribed to: %s\n, %s\n, %s\n", 
                        scfg.relay1.topic, scfg.relay2.topic, scfg.enableTemp.topic );
                }
            } 
            else {
                Serial.printf("Failed broker connection, rc= %d %s\n", client.state(), "try again in 5 seconds");
                vTaskDelay( pdMS_TO_TICKS(5000) ); // Wait 5 seconds before retrying
                continue;
            }
            
            // Init and get the time
            const long  gmtOffset_sec = 3600;
            const int   daylightOffset_sec = 3600;
            configTime(gmtOffset_sec, daylightOffset_sec, cfg.ntp.host);
            interface_setMode( ON );
            xEventGroupClearBits( events, CONNECT_MQTT );
        }
        
        if( ( WiFi.status() != WL_CONNECTED ) ) {
            xEventGroupSetBits( events, CONNECT_WIFI );
            enterConfigMode( );
            continue;
        }
        
        client.loop();
        vTaskDelay( pdMS_TO_TICKS(250) );
    }
}




