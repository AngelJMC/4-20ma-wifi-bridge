#include <WiFi.h>
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "webserver.h"
#include "config-mng.h"
#include "mqtt_task.h"

WiFiClient wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
String wsid ;
String wpass;
//String password ;
//String static_ip ;
String myIP ;
static String LOCAL_IP ;
//String apname ;

String U_name ;
String c_id ;
String s_pass ;

String host_ip ;
int port ;

unsigned long lastSend;
String p_topic;
String Service ;
String service_s ;

static void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        status = WiFi.status();
        if ( status != WL_CONNECTED) {
            WiFi.begin(wsid.c_str(), wpass.c_str());
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }
            Serial.println("Connected to AP");
        }
        Serial.print("Connecting to ThingsBoard node ...");
        // Attempt to connect (clientId, username, password)
        //,U_name.c_str(), s_pass.c_str()

        const char* cuname = NULL;
        if (U_name != "") {
            cuname  = U_name.c_str();
        }
        const char* cpass = NULL;
        if (s_pass != "") {
            cpass = s_pass.c_str();
        }

        if (client.connect(c_id.c_str(), cuname, cpass)) {
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
bool testWifi(void) {
    int c = 0;
    Serial.println("Waiting for Wifi to connect");
    while ( c < 30 ) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(500);
        Serial.print(WiFi.status());
        Serial.print(".");
        c++;
    }
    Serial.println("");
    Serial.println("Connect timed out, opening AP");
    LOCAL_IP = "Not Connected";
    WiFi.mode(WIFI_AP);
    return false;
}


void mqtt_task( void * parameter ) {

    WiFi.mode(WIFI_AP_STA);

    Serial.printf("Set up access point. SSID: %s, PASS: %s\n", cfg.ssid, cfg.pass);
    WiFi.softAP( cfg.ssid, cfg.pass);
    vTaskDelay( pdMS_TO_TICKS(800) );

    Serial.printf("Access point ADDRESS: %s\n", cfg.apaddr);
    String eap = cfg.apaddr;
    String ip1, ip2, ip3, ip4;
    ipAdress(eap, ip1, ip2, ip3, ip4);
    IPAddress Ip(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
    IPAddress NMask(255, 255, 0, 0);

Serial.printf("#### SERVER STARTED ON THIS: %s ###\n", myIP.c_str());

    WiFi.softAPConfig(Ip, Ip, NMask );
    myIP = WiFi.softAPIP().toString();
    
    //################################   WiFi settings Read   #####################################
    struct wifi_config* wf = &cfg.wifi;

    Serial.printf("\nWifi SSID: %s\n", wf->ssid );
    Serial.printf("Wifi PASSword: %s\n", wf->pass );
    Serial.printf("WI-FI_MODE: %s\n", wf->mode );
    
#warning TOBE delete
    if (wsid == NULL) {
        wsid = "Not Given";
        LOCAL_IP = "network not set";
    }


    //####################################### MODE CHECKING(DHCP-STATIC) AND WIFI BEGIN #######################################
    if ( strcmp( wf->mode, "dhcp" ) == 0 ) {
        WiFi.begin( wf->ssid, wf->pass );
        vTaskDelay( pdMS_TO_TICKS( 4000 ) );
        if ( testWifi() ) {
            Serial.print( WiFi.status());
            Serial.println("YOU ARE CONNECTED");
            LOCAL_IP = WiFi.localIP().toString();
            Serial.println(LOCAL_IP);
        }
    }

    if ( strcmp( wf->mode, "static") == 0 ) {
        
        printIp( &wf->ip );
        printIp( &wf->gateway );
        printIp( &wf->netmask );
        printIp( &wf->primaryDNS );
        printIp( &wf->secondaryDNS );
        
        IPAddress S_IP( wf->ip.ip[0], wf->ip.ip[1], wf->ip.ip[2], wf->ip.ip[3] );
        IPAddress gateway( wf->gateway.ip[0], wf->gateway.ip[1], wf->gateway.ip[2], wf->gateway.ip[3] );
        IPAddress subnet( wf->netmask.ip[0], wf->netmask.ip[1], wf->netmask.ip[2], wf->netmask.ip[3] );
        IPAddress primaryDNS( wf->primaryDNS.ip[0], wf->primaryDNS.ip[1], wf->primaryDNS.ip[2], wf->primaryDNS.ip[3] ); //optional
        IPAddress secondaryDNS( wf->secondaryDNS.ip[0], wf->secondaryDNS.ip[1], wf->secondaryDNS.ip[2], wf->secondaryDNS.ip[3] ); //optional

        if (!WiFi.config(S_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
            Serial.println("STA Failed to configure");
        }

        WiFi.begin( cfg.wifi.ssid, cfg.wifi.pass );
        vTaskDelay( pdMS_TO_TICKS( 4000 ) );

        if (testWifi()) {
            Serial.print(WiFi.status());
            Serial.println("YOU ARE CONNECTED");
            LOCAL_IP = WiFi.localIP().toString();
            Serial.println(LOCAL_IP);
        }
    }


    const char* mqttServer = "hairdresser.cloudmqtt.com";
    const int mqttPort =  	15529;
    const char* mqttUser = "eebnnvje";
    const char* mqttPassword = "zuZaQe403XZD";
    client.setServer(mqttServer, mqttPort);



    for(;;){ // infinite loop
        //if (WiFi.status() == WL_CONNECTED) {
        if(0) {
        // Loop until we're reconnected
        
            while (!client.connected()) {
                Serial.print("Attempting MQTT connection...");
                // Create a random client ID
                String clientId = "ESP32Client-ABCD";
                //clientId += String(random(0xffff), HEX);
                // Attempt to connect
                if (client.connect(clientId.c_str(),mqttUser,mqttPassword)) {
                    Serial.println("connected");
                    //Once connected, publish an announcement...
                    client.publish("/esp/test", "hello world");
                    // ... and resubscribe
                    //client.subscribe(MQTT_SERIAL_RECEIVER_CH);
                } else {
                    Serial.print("failed, rc=");
                    Serial.print(client.state());
                    Serial.println(" try again in 5 seconds");
                // Wait 5 seconds before retrying
                //delay(5000);
                vTaskDelay( pdMS_TO_TICKS(5000) );
                }
            
            }
        
        }

        vTaskDelay( pdMS_TO_TICKS(5000) );
        
        if (0) {    
            /*Connect to MQTT broker*/
            if (client.connect(cfg.service.client_id, cfg.service.username, cfg.service.password)) {
                Serial.println("MQTT Connected");
                //client.subscribe(cfg.service.temp.topic);
                //client.subscribe(cfg.service.ping.topic);
                client.subscribe(cfg.service.relay1.topic);
                client.subscribe(cfg.service.relay2.topic);
                client.subscribe(cfg.service.enableTemp.topic);
            }
            else {
                Serial.println("MQTT Connection failed");
            }
            
            
            
            client.setServer(host_ip.c_str(), port );
            if ( !client.connected() ) {
                reconnect();
            }

            if ( millis() - lastSend > 1000 ) { // Update and send only after 1 seconds
                String payload = "{";
                payload += "\"temperature2\":"; payload += 0000; payload += ",";
                payload += "\"humidity2\":"; payload += 9999;
                payload += "}";

                char attributes[800];
                payload.toCharArray(attributes, 800 );
                client.publish(p_topic.c_str(), attributes );
                Serial.println( attributes );
                Serial.println("Data sent successfully");
                //        lastSend = millis();
                service_s = "MQTT(CONNECTED)";
            }
            lastSend = millis();
            client.loop();

            
        }
        



    }

}




