

#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "config-mng.h"
#include "mqtt_task.h"
#include "SPIFFS.h"
#include "uinterface.h"

enum {
    verbose = 1
};


enum {
    START_SERVER        = 1u << 0,
    STOP_SERVER         = 1u << 1,
    SCAN_WIFI           = 1u << 2,
    SAVE_CFG            = 1u << 3,
    UPDATE_SERVICE      = 1u << 4,
    UPDATE_CALIBRATION  = 1u << 5,
};

static EventGroupHandle_t eventGroup;
static bool isServerActive = false;

//##########################  configuration and variables  ##################

String jsonwifis;
AsyncWebServer server(80);



//################################  MAC ADDRESS FUNCTION  #########################################
String getMacAddress( void ) {
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[5], baseMac[4], baseMac[3], baseMac[2], baseMac[1], baseMac[0]);
    return String(baseMacChr);
}


//############   Conversion for acceesspoint ip into unsigned int ###################

void ipAdress(String& eap, String& iip1, String& iip2, String& iip3, String& iip4) {

    const int numberOfPieces = 4;
    String ipaddress[numberOfPieces];
    int counter = 0;
    int lastIndex = 0;
    for (int i = 0; i < eap.length(); i++) {
        if (eap.substring(i, i + 1) == ".") {
            ipaddress[counter] = eap.substring(lastIndex, i);
            lastIndex = i + 1;
            counter++;
        }
        if (i == eap.length() - 1) {
            ipaddress[counter] = eap.substring(lastIndex);
        }

    }
    iip1 = ipaddress[0];
    iip2 = ipaddress[1];
    iip3 = ipaddress[2];
    iip4 = ipaddress[3];
}


/** Funtion to convert a String yo ip struct passed by reference */
int stringToIp(struct ip* dest, String src) {
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

static void ipToString(String* dest, struct ip src) {
    char buf[16];
    sprintf(buf, "%d.%d.%d.%d", src.ip[0], src.ip[1], src.ip[2], src.ip[3]);
    *dest = buf;
}

void webserver_start( void ){ 
    xEventGroupSetBits( eventGroup, START_SERVER );
}

void webserver_stop( void ) { 
    xEventGroupSetBits( eventGroup, STOP_SERVER );
}


void webserver_toggleState( void ) {
    if( isServerActive ) {
        xEventGroupSetBits( eventGroup, STOP_SERVER );
    }
    else {
        xEventGroupSetBits( eventGroup, START_SERVER );
    }
}

void webserver_task( void * parameter ) {

    eventGroup = xEventGroupCreate();
    interface_setState( OFF );
    if( eventGroup == NULL ){
        /* The event group was not created because there was insufficient
        FreeRTOS heap available. */
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


    
    server.on("/main", HTTP_GET, [](AsyncWebServerRequest * request) {

        DynamicJsonDocument doc(512);
        doc["MAC"]     = getMacAddress();
        doc["myIP"]    = WiFi.softAPIP().toString();
        doc["wsid"]    = cfg.wifi.ssid;
        doc["localIP"] = WiFi.localIP().toString();
        doc["apname"]  = cfg.ap.ssid;
        //doc["s_pass"] = s_pass;
        
        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });


    server.on("/networkData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(1024);
        String temporal;
        doc["mode"]  = std::string(cfg.wifi.mode, strlen(cfg.wifi.mode));
        ipToString( &temporal, cfg.wifi.ip );
        doc["ip"]    = temporal;
        ipToString( &temporal, cfg.wifi.gateway );
        doc["gw"]    = temporal;
        ipToString( &temporal, cfg.wifi.netmask );
        doc["nm"]    = temporal;
        ipToString( &temporal, cfg.wifi.primaryDNS );
        doc["dns1"]  = temporal;
        ipToString( &temporal, cfg.wifi.secondaryDNS );
        doc["dns2"]  = temporal;

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });


    server.on("/ntpData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(128);
        doc["host"]    = std::string( cfg.ntp.host, strlen(cfg.ntp.host) );
        doc["port"]    = cfg.ntp.port;

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });


    server.on("/serviceData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(1024*2);
        doc["host"]    = std::string( cfg.service.host_ip, strlen(cfg.service.host_ip));
        doc["port"]    = cfg.service.port;
        doc["id"]      = std::string(cfg.service.client_id, strlen(cfg.service.client_id));
        doc["user"]    = std::string(cfg.service.username, strlen(cfg.service.username));
        doc["pass"]    = std::string(cfg.service.password, strlen(cfg.service.password));  
        doc["temp_tp"] = std::string(cfg.service.temp.topic, strlen(cfg.service.temp.topic));
        doc["temp_tm"] = cfg.service.temp.period;
        doc["temp_ud"] = std::string(cfg.service.temp.unit, strlen(cfg.service.temp.unit));
        doc["ping_tp"] = std::string(cfg.service.ping.topic, strlen(cfg.service.ping.topic));
        doc["ping_tm"] = cfg.service.ping.period;
        doc["ping_ud"] = std::string(cfg.service.ping.unit, strlen(cfg.service.ping.unit));
        doc["rel1_tp"] = std::string(cfg.service.relay1.topic, strlen(cfg.service.relay1.topic));
        doc["rel2_tp"] = std::string(cfg.service.relay2.topic, strlen(cfg.service.relay2.topic));
        doc["en_tp"]   = std::string(cfg.service.enableTemp.topic , strlen(cfg.service.enableTemp.topic));

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });

    server.on("/calibrationData", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(128*2);
        doc["x0"]    = cfg.cal.val[0].x;
        doc["y0"]    = cfg.cal.val[0].y;
        doc["x1"]    = cfg.cal.val[1].x;
        doc["y1"]    = cfg.cal.val[1].y;

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });

    server.on("/sample", HTTP_GET, [](AsyncWebServerRequest * request) {
        DynamicJsonDocument doc(32);
        uint32_t adc_reading = 0;
        enum {
            NO_OF_SAMPLES = 64
        };
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += analogRead(SENS); //adc1_get_raw((adc1_channel_t)get_adc1_chanel(ADC_BAT_PIN));
        }
        adc_reading /= NO_OF_SAMPLES;
        
        doc["adcval"]    = adc_reading;

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });

    //###################################   ACTIONS FROM WEBPAGE BUTTTONS  ##############################



    //################################# AP SSID-PASSWORD-IP RECEIVING FROM WEB PAGE WRITING TO EEPROM  ###############################
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
        if (scan_wifi) {
            Serial.println(jsonwifis);    
            request->send(200, "application/json", jsonwifis);
            xEventGroupSetBits( eventGroup, SCAN_WIFI );
        }
    });


    //############################### RECEIVING DATA SEND MMETHODS HTTP-MQTT-TCP ##############################
    server.on("/applyNtp", HTTP_GET, [] (AsyncWebServerRequest * request) {
        
        String parameters = request->getParam("parameters")->value();
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

        memset( &cfg.ntp, 0, sizeof( cfg.ntp ) );
        if (root.containsKey("host"))  strcpy(cfg.ntp.host, root["host"]);
        if (root.containsKey("port"))  cfg.ntp.port = root["port"];
        
        xEventGroupSetBits( eventGroup, SAVE_CFG );
        request->send(200, "text/plain", "ok");
        
        if( verbose )
            print_ntpCfg( &cfg.ntp );
    });




    //#####################################  Receving WIFI credential from WEB Page ############################
    server.on("/applyNetwork", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
        Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(15) + 512;
        DynamicJsonDocument doc(capacity);
        /*auto error = deserializeJson(doc, parameters);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            request->send(200, "text/plain", "error");
            return;
        }

        //int err = 0;
        if (root.containsKey("wifi_ssid"))     strcpy(cfg.wifi.ssid, root["wifi_ssid"]);
        if (root.containsKey("wifi_pass"))     strcpy(cfg.wifi.pass, root["wifi_pass"]);
        if (root.containsKey("wifi_MODE"))     strcpy(cfg.wifi.mode, root["wifi_MODE"]);

        if ( strcmp( cfg.wifi.mode, "static") == 0 ) {
            if (root.containsKey("txtipadd"))  stringToIp(&cfg.wifi.ip, root["txtipadd"]);
            if (root.containsKey("net_m"))     stringToIp(&cfg.wifi.netmask, root["net_m"]);
            if (root.containsKey("G_add"))     stringToIp(&cfg.wifi.gateway, root["G_add"]);
            if (root.containsKey("P_dns"))     stringToIp(&cfg.wifi.primaryDNS, root["P_dns"]);
            if (root.containsKey("S_dns"))     stringToIp(&cfg.wifi.secondaryDNS, root["S_dns"]);           
        }*/
        int err = 0;
        if( 0 == err ) { 
            request->send(200, "text/plain", "ok");
            //xEventGroupSetBits( eventGroup, SAVE_CFG );
        }
        else {
            request->send(200, "text/plain", "error");
        }

        if( verbose )
            print_NetworkCfg( &cfg.wifi );

    });


    //############################### RECEIVING DATA SEND MMETHODS HTTP-MQTT-TCP ##############################
    server.on("/applyService", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
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

    //############################### RECEIVING DATA SEND MMETHODS HTTP-MQTT-TCP ##############################
    server.on("/applyCalibration", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String parameters = request->getParam("parameters")->value();
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

        if (root.containsKey("x0"))     cfg.cal.val[0].x =  root["x0"];
        if (root.containsKey("y0"))     cfg.cal.val[0].y =  root["y0"];
        if (root.containsKey("x1"))     cfg.cal.val[1].x =  root["x1"];
        if (root.containsKey("y1"))     cfg.cal.val[1].y =  root["y1"];

        xEventGroupSetBits( eventGroup, SAVE_CFG | UPDATE_CALIBRATION );
        request->send(200, "text/plain", "ok");
        
        if ( verbose )
            print_Calibration( &cfg.cal );
    });
    //###############################  RESTARTING DEVICE ON REBOOT BUTTON ####################################

    server.on("/rebootbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

        if (request->getParam("reboot_btn")->value() == "reboot_device") {
            Serial.print("restarting device");
            request->send(200, "text/plain", "ok");
            vTaskDelay(pdMS_TO_TICKS(5000));
            ESP.restart();
        }
    });

    //################################   ADMIN password change function   ######################################

    server.on("/adminpasswordfunction", HTTP_GET, [](AsyncWebServerRequest * request) {
        String confirmpassword = request->getParam("confirmpassword")->value();
        Serial.print(confirmpassword);
    });

    //######################################    RESET to Default  ############################################
    server.on("/resetbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

        if (request->getParam("reset_btn")->value() == "reset_device") {
            config_setdefault( ); 
            Serial.print("restarting device");
            request->send(200, "text/plain", "ok");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
        }
    });

    enum {
        forever = -1,
        clearonexit = pdTRUE,
        waitall  = pdTRUE
    };

    for(;;){ 

        const EventBits_t waitbits = START_SERVER | STOP_SERVER | SCAN_WIFI | SAVE_CFG;
        EventBits_t ctrlflags = xEventGroupWaitBits( eventGroup, waitbits, !clearonexit, !waitall, forever );
        
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
            Serial.println("Scan wifi");
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
                        jsonwifis += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                        jsonwifis += ",\"channel\":" + String(WiFi.channel(i));
                        jsonwifis += ",\"secure\":" + String(WiFi.encryptionType(i));
                        jsonwifis += "}";
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                jsonwifis += "]";
            }
        } 
        
        if ( ctrlflags & SAVE_CFG ) {
            xEventGroupClearBits( eventGroup, SAVE_CFG );
            config_savecfg( );
            
            if ( ctrlflags & UPDATE_SERVICE ) {
                xEventGroupClearBits( eventGroup, UPDATE_SERVICE );
                updateServiceCfg( );
            }

            if( ctrlflags & UPDATE_CALIBRATION ) {
                xEventGroupClearBits( eventGroup, UPDATE_CALIBRATION );
                updateCalibration( );
            }
        }


        
    }

}




