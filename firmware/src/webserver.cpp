/*
    Project: <https://github.com/AngelJMC/4-20ma-wifi-bridge>   
    Copyright (c) 2022 Angel Maldonado <angelgesus@gmail.com>. 
    Licensed under the MIT License: <http://opensource.org/licenses/MIT>.
    SPDX-License-Identifier: MIT 
*/

#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "config-mng.h"
#include "SPIFFS.h"
#include "uinterface.h"

enum {
    verbose = 1
};


enum {
    START_SERVER          = 1u << 0,
    STOP_SERVER           = 1u << 1,
    SCAN_WIFI             = 1u << 2,
    SAVE_CFG              = 1u << 3,
    UPDATE_SERVICE        = 1u << 4,
    UPDATE_CALIBRATION    = 1u << 5,
    UPDATE_NETWORK        = 1u << 6,
    OVERWRITE_CALIBRATION = 1u << 7
};

static EventGroupHandle_t eventGroup;
static bool isServerActive = false;
static String jsonwifis;
static AsyncWebServer server(80);


/* Get MAC address for WiFi station*/
static String getMacAddress( void ) {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    return String(baseMacChr);
}

/** Funtion to convert a String to ip struct passed by reference */
static int stringToIp(struct ip* dest, String src) {
    int i = 0;
    struct ip tmp;
    char* pch = strtok((char*)src.c_str(), ".");
    while (pch != NULL) {
        if( 3 < i )
            return -1;
        tmp.ip[i] = atoi(pch);
        pch = strtok(NULL, ".");
        i++;
    }
    
    if ( 4 != i )
        return -1;

    *dest = tmp;
    return 0;
}

/** Funtion to convert a ip struct to String */
static void ipToString(String* dest, struct ip src) {
    char buf[16];
    sprintf(buf, "%d.%d.%d.%d", src.ip[0], src.ip[1], src.ip[2], src.ip[3]);
    *dest = buf;
}

bool webserver_isNetworkUpdated( void ) {
    EventBits_t bits = xEventGroupGetBits( eventGroup );
    if ( bits & UPDATE_NETWORK ) {
        xEventGroupClearBits( eventGroup, UPDATE_NETWORK );
        return 1;
    }
    return 0;
}

bool webserver_isServiceUpdated( void ) {
    EventBits_t bits = xEventGroupGetBits( eventGroup );
    if ( bits & UPDATE_SERVICE ) {
        xEventGroupClearBits( eventGroup, UPDATE_SERVICE );
        return 1;
    }
    return 0;
}

bool webserver_isCalibrationUpdated( void ) {
    EventBits_t bits = xEventGroupGetBits( eventGroup );
    if ( bits & UPDATE_CALIBRATION ) {
        xEventGroupClearBits( eventGroup, UPDATE_CALIBRATION );
        return 1;
    }
    return 0;
}

void webserver_start( void ){ 
    xEventGroupSetBits( eventGroup, START_SERVER );
}

void webserver_stop( void ) { 
    xEventGroupSetBits( eventGroup, STOP_SERVER );
}


void webserver_task( void * parameter ) {

    eventGroup = xEventGroupCreate();
    interface_setState( OFF );
    if( eventGroup == NULL ){
        Serial.println("Event group not created");
    }
    
    //#########################  HTML+JS+CSS  HANDLING #####################################
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/main.html", "text/html");
    });
    server.on("/main.html", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/main.html", "text/html");
    });
    server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/js/bootstrap.min.js", "text/javascript");
    });
    server.on("/js/jquery-1.12.3.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/js/jquery-1.12.3.min.js", "text/javascript");
    });
    server.on("/js/pixie-custom.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/js/pixie-custom.js", "text/javascript");
    });
    server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
    });
    server.on("/css/pixie-main.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(cfg.ap.web_user, cfg.ap.web_pass) )
            return request->requestAuthentication();
        request->send(SPIFFS, "/css/pixie-main.css", "text/css");
    });
    
    //############################# IMAGES HANDLING  ######################################################
    server.on("/images/ap.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/ap.png", "image/png");
    });
    server.on("/images/eye-close.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/eye-close.png", "image/png");
    });
    server.on("/images/light.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/light.png", "image/png");
    });
    server.on("/images/network.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/network.png", "image/png");
    });
    server.on("/images/other.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/other.png", "image/png");
    });
    server.on("/images/periperal.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/periperal.png", "image/png");
    });
    server.on("/images/reboot.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/reboot.png", "image/png");
    });
    server.on("/images/service.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/service.png", "image/png");
    });
    server.on("/images/status.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/status.png", "image/png");
    });
    server.on("/images/upgrade.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/upgrade.png", "image/png");
    });
    server.on("/images/timezone.png", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/images/timezone.png", "image/png");
    });


    /*Send json with device information*/
    server.on("/main", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 512 );
        json["mac"]     = getMacAddress();
        json["myIP"]    = WiFi.softAPIP().toString();
        json["wsid"]    = cfg.wifi.ssid;
        json["localIP"] = WiFi.localIP().toString();
        json["apname"]  = cfg.ap.ssid;
        
        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    /*Send json with network configuration*/
    server.on("/networkData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 1024 );
        String temporal;
        json["mode"]  = std::string(cfg.wifi.mode, strlen(cfg.wifi.mode));
        ipToString( &temporal, cfg.wifi.ip );
        json["ip"]    = temporal;
        ipToString( &temporal, cfg.wifi.gateway );
        json["gw"]    = temporal;
        ipToString( &temporal, cfg.wifi.netmask );
        json["nm"]    = temporal;
        ipToString( &temporal, cfg.wifi.primaryDNS );
        json["dns1"]  = temporal;
        ipToString( &temporal, cfg.wifi.secondaryDNS );
        json["dns2"]  = temporal;

        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    /*Send json with NTP configuration*/
    server.on("/ntpData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 128 );
        json["host"]    = std::string( cfg.ntp.host, strlen(cfg.ntp.host) );
        json["port"]    = cfg.ntp.port;

        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    /*Send json with mqtt broker and topic configuration*/
    server.on("/serviceData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 2*1024 );
        json["host"]    = std::string( cfg.service.host_ip, strlen(cfg.service.host_ip));
        json["port"]    = cfg.service.port;
        json["id"]      = std::string(cfg.service.client_id, strlen(cfg.service.client_id));
        json["user"]    = std::string(cfg.service.username, strlen(cfg.service.username));
        json["pass"]    = std::string(cfg.service.password, strlen(cfg.service.password));  
        json["temp_tp"] = std::string(cfg.service.temp.topic, strlen(cfg.service.temp.topic));
        json["temp_tm"] = cfg.service.temp.period;
        json["temp_ud"] = std::string(cfg.service.temp.unit, strlen(cfg.service.temp.unit));
        json["ping_tp"] = std::string(cfg.service.ping.topic, strlen(cfg.service.ping.topic));
        json["ping_tm"] = cfg.service.ping.period;
        json["ping_ud"] = std::string(cfg.service.ping.unit, strlen(cfg.service.ping.unit));
        json["rel1_tp"] = std::string(cfg.service.relay1.topic, strlen(cfg.service.relay1.topic));
        json["rel2_tp"] = std::string(cfg.service.relay2.topic, strlen(cfg.service.relay2.topic));
        json["en_tp"]   = std::string(cfg.service.enableTemp.topic , strlen(cfg.service.enableTemp.topic));

        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    /*Send json sensor calibration*/
    server.on("/calibrationData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 254 );
        json["x0"]    = cfg.cal.val[0].x;
        json["y0"]    = cfg.cal.val[0].y;
        json["x1"]    = cfg.cal.val[1].x;
        json["y1"]    = cfg.cal.val[1].y;

        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    /*Send json with sensor sample*/
    server.on("/sample", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument json( 32 );
        json["adcval"]    = getadcValue( );

        String content;
        serializeJson(json, content);
        request->send(200, "application/json", content);
        if( verbose ) Serial.println(content);
    });

    //###################################   ACTIONS FROM WEBPAGE BUTTTONS  ##############################

    /*Receive ap ssid, password and ip from web page and write to eeprom*/
    server.on("/applyAP", HTTP_GET, [] (AsyncWebServerRequest * request) {
        
        int err = 0;
        String txt = request->getParam("txtssid")->value();
        if ( txt.length() > 0 )
            strcpy(cfg.ap.ssid, txt.c_str());

        txt = request->getParam("txtpass")->value();
        if ( txt.length() > 0) 
            strcpy(cfg.ap.pass, txt.c_str());

        txt = request->getParam("txtaplan")->value();
        if ( txt.length() > 0) { 
            err = stringToIp( &cfg.ap.addr, txt ); 
        }    

        if( 0 == err ) { 
            request->send(200, "text/plain", "ok");
            xEventGroupSetBits( eventGroup, SAVE_CFG );
        }
        else {
            request->send(200, "text/plain", "error");
        }

        if( verbose )
            print_apCfg( &cfg.ap );
    });

    
    server.on("/scanWifi", HTTP_GET, [](AsyncWebServerRequest * request) {
        String scan_wifi = request->getParam("scan_wifi")->value();
        if ( verbose )
            Serial.println(jsonwifis);  
          
        request->send(200, "application/json", jsonwifis);
        xEventGroupSetBits( eventGroup, SCAN_WIFI );
        
    });

    /*Receive json with mqtt broker and topic configuration*/
    server.on("/applyNtp", HTTP_GET, [] (AsyncWebServerRequest * request) {
        
        String parameters = request->getParam("parameters")->value();
        if ( verbose )
            Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(15) + 128;
        DynamicJsonDocument doc(capacity);
        auto error = deserializeJson(doc, parameters);
        if (error) {
            Serial.println("parseObject() failed:");
            request->send(200, "text/plain", "error");
            return;
        }


        JsonObject root = doc.as<JsonObject>();
        memset( &cfg.ntp, 0, sizeof( cfg.ntp ) );
        if (root.containsKey("host"))  strcpy(cfg.ntp.host, root["host"]);
        if (root.containsKey("port"))  cfg.ntp.port = root["port"];
        
        xEventGroupSetBits( eventGroup, SAVE_CFG );
        request->send(200, "text/plain", "ok");
        
        if( verbose )
            print_ntpCfg( &cfg.ntp );
    });

    /*Receive WIFI credential and network configuration from web page*/
    server.on("/applyNetwork", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
        if ( verbose )
            Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(15) + 512;
        DynamicJsonDocument doc(capacity);
        auto error = deserializeJson(doc, parameters);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            request->send(200, "text/plain", "error");
            return;
        }
        
        if (root.containsKey("wifi_ssid"))     strcpy(cfg.wifi.ssid, root["wifi_ssid"]);
        if (root.containsKey("wifi_pass"))     strcpy(cfg.wifi.pass, root["wifi_pass"]);
        if (root.containsKey("wifi_MODE"))     strcpy(cfg.wifi.mode, root["wifi_MODE"]);

        int err = 0;
        if ( strcmp( cfg.wifi.mode, "static") == 0 ) {
            if (root.containsKey("txtipadd"))  err |= stringToIp(&cfg.wifi.ip, root["txtipadd"]);
            if (root.containsKey("net_m"))     err |= stringToIp(&cfg.wifi.netmask, root["net_m"]);
            if (root.containsKey("G_add"))     err |= stringToIp(&cfg.wifi.gateway, root["G_add"]);
            if (root.containsKey("P_dns"))     err |= stringToIp(&cfg.wifi.primaryDNS, root["P_dns"]);
            if (root.containsKey("S_dns"))     err |= stringToIp(&cfg.wifi.secondaryDNS, root["S_dns"]);           
        }
        
        if( 0 == err ) { 
            request->send(200, "text/plain", "ok");
            xEventGroupSetBits( eventGroup, SAVE_CFG | UPDATE_NETWORK);
        }
        else {
            request->send(200, "text/plain", "error");
        }

        if( verbose )
            print_NetworkCfg( &cfg.wifi );

    });


    /*Receive mqtt and topic configuration from web page*/
    server.on("/applyService", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
        if ( verbose )
            Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(15) + 512;
        DynamicJsonDocument doc(capacity);
        auto error = deserializeJson(doc, parameters);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            request->send(200, "text/plain", "error");
            return;
        }

        memset( &cfg.service, 0, sizeof( cfg.service ) );
        if (root.containsKey("host"))     strcpy(cfg.service.host_ip, root["host"]);
        if (root.containsKey("port"))     cfg.service.port = root["port"];
        if (root.containsKey("id"))       strcpy(cfg.service.client_id, root["id"]);
        if (root.containsKey("user"))     strcpy(cfg.service.username, root["user"]);
        if (root.containsKey("pass"))     strcpy(cfg.service.password, root["pass"]);
        if (root.containsKey("temp_tp"))  strcpy(cfg.service.temp.topic, root["temp_tp"]);
        if (root.containsKey("temp_tm"))  cfg.service.temp.period = root["temp_tm"];
        if (root.containsKey("temp_ud"))  strcpy(cfg.service.temp.unit, root["temp_ud"]);
        if (root.containsKey("ping_tp"))  strcpy(cfg.service.ping.topic, root["ping_tp"]); 
        if (root.containsKey("ping_tm"))  cfg.service.ping.period = root["ping_tm"];
        if (root.containsKey("ping_ud"))  strcpy(cfg.service.ping.unit, root["ping_ud"]);
        if (root.containsKey("rel1_tp"))  strcpy(cfg.service.relay1.topic, root["rel1_tp"]);
        if (root.containsKey("rel2_tp"))  strcpy(cfg.service.relay2.topic, root["rel2_tp"]);
        if (root.containsKey("en_tp"))    strcpy(cfg.service.enableTemp.topic, root["en_tp"]);

        xEventGroupSetBits( eventGroup, SAVE_CFG | UPDATE_SERVICE );
        request->send(200, "text/plain", "ok");
        
        if ( verbose )
            print_ServiceCfg( &cfg.service );
    });

    /*Receive sensor calibration from web page*/
    server.on("/applyCalibration", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
        if ( verbose )
            Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(15) + 128;
        DynamicJsonDocument doc(capacity);
        auto error = deserializeJson(doc, parameters);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            request->send(200, "text/plain", "error");
            return;
        }
        bool overwrite = false;
        if (root.containsKey("x0"))     cfg.cal.val[0].x =  root["x0"];
        if (root.containsKey("y0"))     cfg.cal.val[0].y =  root["y0"];
        if (root.containsKey("x1"))     cfg.cal.val[1].x =  root["x1"];
        if (root.containsKey("y1"))     cfg.cal.val[1].y =  root["y1"];
        if (root.containsKey("owrite")) overwrite = root["owrite"];

        xEventGroupSetBits( eventGroup, SAVE_CFG | UPDATE_CALIBRATION | ( overwrite ? OVERWRITE_CALIBRATION : 0) );
        request->send(200, "text/plain", "ok");
        
        if ( verbose )
            print_Calibration( &cfg.cal );
    });

    /*Receive restarting device*/
    server.on("/rebootbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

        if (request->getParam("reboot_btn")->value() == "reboot_device") {
            request->send(200, "text/plain", "ok");
            Serial.print("Restarting device");
            vTaskDelay(pdMS_TO_TICKS(5000));
            ESP.restart();
        }
    });

    /*Receive reset to default device*/
    server.on("/resetbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

        if (request->getParam("reset_btn")->value() == "reset_device") {
            request->send(200, "text/plain", "ok");
            factoryreset();
        }
    });

    enum {
        FOREVER = -1,
        CLEAR_ON_EXIT = pdTRUE,
        WAIT_ALL  = pdTRUE
    };

    for(;;){ 

        const EventBits_t waitbits = START_SERVER | STOP_SERVER | SCAN_WIFI | SAVE_CFG;
        EventBits_t ctrlflags = xEventGroupWaitBits( eventGroup, waitbits, !CLEAR_ON_EXIT, !WAIT_ALL, FOREVER );
        
        if ( ctrlflags & START_SERVER ) {
            xEventGroupClearBits( eventGroup, START_SERVER );
            server.begin();
            Serial.printf("\nStarting HTTP server\n");
            isServerActive = true;
            interface_setState( BLINK );
        }

        if ( ctrlflags & STOP_SERVER ) {
            xEventGroupClearBits( eventGroup, STOP_SERVER );
            server.end();
            Serial.println("\nStoping HTTP server\n");
            isServerActive = false;
            interface_setState( ON );
        }

        if( ctrlflags & SCAN_WIFI ) {
            xEventGroupClearBits( eventGroup, SCAN_WIFI );
            Serial.print("Scan wifi: ");
            uint8_t WIFI_SSIDs = WiFi.scanNetworks();     
            if( WIFI_SSIDs != WIFI_SCAN_FAILED) {
                jsonwifis = "[";
                if (WIFI_SSIDs == 0 || WIFI_SSIDs == 254 ){
                    Serial.println("no networks found");
                }
                else {
                    Serial.print(WIFI_SSIDs);
                    Serial.println(" networks found");
                    for (int i = 0; i < WIFI_SSIDs; ++i) {
                        if (i)
                            jsonwifis += ",";
                        jsonwifis += "{";
                        jsonwifis += "\"rssi\":" + String(WiFi.RSSI(i));
                        jsonwifis += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                        jsonwifis += "}";
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                jsonwifis += "]";
            }
        } 
        
        if( ctrlflags & SAVE_CFG ) {
            config_savecfg( );
            xEventGroupClearBits( eventGroup, SAVE_CFG );
        } 

        if( ctrlflags & OVERWRITE_CALIBRATION ) {
            config_overwritedefaultcal( &cfg.cal );
            xEventGroupClearBits( eventGroup, OVERWRITE_CALIBRATION );
        }       
    }
}




