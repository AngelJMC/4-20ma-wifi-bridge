

#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "config-mng.h"
#include "mqtt_task.h"
#include "SPIFFS.h"

enum {
    verbose = 1
};


//##########################  configuration and variables  ##################

String LOCAL_IP ;
String abc;
String jsonwifis;
AsyncWebServer server(80);
static int scan = false;


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
void stringToIp(struct ip* dest, String src) {
    int i = 0;
    char* pch = strtok((char*)src.c_str(), ".");
    while (pch != NULL) {
        dest->ip[i] = atoi(pch);
        pch = strtok(NULL, ".");
        i++;
    }
}

static void ipToString(String* dest, struct ip src) {
    char buf[16];
    sprintf(buf, "%d.%d.%d.%d", src.ip[0], src.ip[1], src.ip[2], src.ip[3]);
    *dest = buf;
}


void printIp( char const* name, struct ip const* src ) {
    Serial.printf("%s : %d.%d.%d.%d\n", name, src->ip[0], src->ip[1], src->ip[2], src->ip[3]);
}

void print_ntpCfg( struct ntp_config const* ntp ) {
    Serial.printf("NTP HOST: %s\n", ntp->host);
    Serial.printf("NTP PORT: %d\n", ntp->port);
    Serial.printf("NTP PERIOD: %d\n", ntp->period);
}

void print_NetworkCfg( struct wifi_config const* ntwk ) {
        Serial.printf("WIFI SSID: %s\n", ntwk->ssid);
        Serial.printf("WIFI PASS: %s\n", ntwk->pass);
        Serial.printf("WIFI MoDE: %s\n", ntwk->mode );
        
        if ( strcmp( ntwk->mode, "static") == 0 ) {
            printIp("WIFI IP: ", &ntwk->ip);
            printIp("WIFI NETMASK: ", &ntwk->netmask);
            printIp("WIFI GATEWAY: ", &ntwk->gateway);
            printIp("WIFI PRIMARY DNS: ", &ntwk->primaryDNS);
            printIp("WIFI SECONDARY DNS: ", &ntwk->secondaryDNS);
        }
}

void print_ServiceCfg( struct service_config const* srvc ) {

        Serial.printf("MQTT HOST IP: %s\n", srvc->host_ip);
        Serial.printf("MQTT PORT: %d\n", srvc->port);
        Serial.printf("MQTT CLIENT_ID: %s\n", srvc->client_id);
        Serial.printf("MQTT USER: %s\n", srvc->username);
        Serial.printf("MQTT PASS: %s\n", srvc->password);
        Serial.printf("MQTT TEMP_TOPIC: %s\n", srvc->temp.topic);
        Serial.printf("MQTT TEMP_INTER: %d\n", srvc->temp.period);
        Serial.printf("MQTT TEMP_UND: %s\n", srvc->temp.unit);
        Serial.printf("MQTT PING_TOPIC: %s\n", srvc->ping.topic);
        Serial.printf("MQTT PING_INTER: %d\n", srvc->ping.period);
        Serial.printf("MQTT PING_UND: %s\n", srvc->ping.unit);
        Serial.printf("MQTT REL1_TOPIC: %s\n", srvc->relay1.topic);
        Serial.printf("MQTT REL2_TOPIC: %s\n", srvc->relay2.topic);
        Serial.printf("MQTT EN_TOPIC: %s\n", srvc->enableTemp.topic);
}

void print_apCfg( struct ap_config const* ap ) {
    Serial.printf("AP SSID: %s\n", ap->ssid );
    Serial.printf("AP PASS: %s\n", ap->pass );
    Serial.printf("AP IP:   %s\n", ap->addr );
}

void webserver_task( void * parameter ) {


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

        DynamicJsonDocument doc(1024);
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

    //###################################   ACTIONS FROM WEBPAGE BUTTTONS  ##############################



    //################################# AP SSID-PASSWORD-IP RECEIVING FROM WEB PAGE WRITING TO EEPROM  ###############################
    server.on("/applyAP", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String txtssid = request->getParam("txtssid")->value();
        if (txtssid.length() > 0)
            strcpy(cfg.ap.ssid, txtssid.c_str());

        String txtpass = request->getParam("txtpass")->value();
        if ( txtpass.length() > 0) 
            strcpy(cfg.ap.pass, txtpass.c_str());

        String txtapip = request->getParam("txtaplan")->value();
        if ( txtapip.length() > 0) 
            stringToIp( &cfg.ap.addr, txtapip );       

        config_savecfg( );
        request->send(200, "text/plain", "ok");

        if( verbose )
            print_apCfg( &cfg.ap );
    });


    server.on("/scanWifi", HTTP_GET, [](AsyncWebServerRequest * request) {
        String scan_wifi = request->getParam("scan_wifi")->value();
        if (scan_wifi) {
            Serial.println(jsonwifis);    
            request->send(200, "application/json", jsonwifis);
            scan = true;
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
        
        config_savecfg( );
        request->send(200, "text/plain", "ok");
        
        if( verbose )
            print_ntpCfg( &cfg.ntp );
    });




    //#####################################  Receving WIFI credential from WEB Page ############################
    server.on("/applyNetwork", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String  wifi_ssid = request->getParam("wifi_ssid")->value();
        if (wifi_ssid.length() > 0)
            strcpy(cfg.wifi.ssid, wifi_ssid.c_str());  
        
        String  wifi_pass = request->getParam("wifi_pass")->value();
        if ( wifi_pass.length() > 0)
            strcpy(cfg.wifi.pass, wifi_pass.c_str());

        String  wifi_mode  = request->getParam("wifi_MODE")->value();
        strcpy(cfg.wifi.mode, wifi_mode.c_str());

        if (strcmp( wifi_mode.c_str(), "static") == 0) {

            String ipstatic = request->getParam("txtipadd")->value();
            if ( ipstatic.length() > 0)
                stringToIp(&cfg.wifi.ip, ipstatic);
            
            String netmask = request->getParam("net_m")->value();
            if ( netmask.length() > 0)
                stringToIp(&cfg.wifi.netmask, netmask);

            String gateway = request->getParam("G_add")->value();
            if ( gateway.length() > 0)
                stringToIp(&cfg.wifi.gateway, gateway);

            String dns1 = request->getParam("P_dns")->value();
            if ( dns1.length() > 0)
                stringToIp(&cfg.wifi.primaryDNS, dns1);

            String dns2 = request->getParam("S_dns")->value();
            if ( dns2.length() > 0)
                stringToIp(&cfg.wifi.secondaryDNS, dns2);
        }

        config_savecfg( );
        request->send(200, "text/plain", "ok");

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

        config_savecfg( );
        updateServiceCfg( );
        request->send(200, "text/plain", "ok");
        
        if ( verbose )
            print_ServiceCfg( &cfg.service );
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


    vTaskDelay(pdMS_TO_TICKS(5000));
    server.begin();
    Serial.println("HTTP server started");

    for(;;){ // infinite loop

        if( scan ) {
            scan = false;
            Serial.println("scan_wifi");
            uint8_t WIFI_SSIDs = WiFi.scanNetworks();     
            if( WIFI_SSIDs != WIFI_SCAN_FAILED) {
                jsonwifis = "[";
                if (WIFI_SSIDs == 0){
                    Serial.println("no networks found");
                }
                else {
                    Serial.print(WIFI_SSIDs);
                    Serial.println(" networks found");
                    for (int i = 0; i < WIFI_SSIDs; ++i) {
                        // Print SSID and RSSI for each network found
                        if (i)
                            jsonwifis += ",";
                        jsonwifis += "{";
                        jsonwifis += "\"rssi\":" + String(WiFi.RSSI(i));
                        jsonwifis += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                        jsonwifis += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                        jsonwifis += ",\"channel\":" + String(WiFi.channel(i));
                        jsonwifis += ",\"secure\":" + String(WiFi.encryptionType(i));
                        //                json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
                        jsonwifis += "}";
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                    
                }
                jsonwifis += "]";
            }
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }

}




