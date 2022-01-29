#include <WiFi.h>
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

#include "config-mng.h"


WiFiClient wifiClient;
//##########################  configuration and variables  ##################
int status = WL_IDLE_STATUS;
unsigned long lastSend;
PubSubClient client(wifiClient);
String wsid ;
String wpass;
String password ;
String static_ip ;
String myIP ;
String LOCAL_IP ;
String apname ;
String abc;

const char* file = "/config.json";   //Enter your file name
const char* s_data_file = "/ServiceData_jsonfile.txt";
String Service ;
String service_s ;
String host_ip ;
int port ;
int uinterval ;
String u_time;
String c_id ;

int QOS ; // 0
String U_name ;
String s_pass ;
String p_topic;
String Http_requestpath;

AsyncWebServer server(80);

static void reconnect();

//################################  MAC ADDRESS FUNCTION  #########################################
String getMacAddress() {
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[5], baseMac[4], baseMac[3], baseMac[2], baseMac[1], baseMac[0]);
    return String(baseMacChr);
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

//############   Conversion for acceesspoint ip into unsigned int ###################
const int numberOfPieces = 4;
String ipaddress[numberOfPieces];
void ipAdress(String& eap, String& iip1, String& iip2, String& iip3, String& iip4)
{

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
static void stringToIp(struct ip* dest, String src) {
    int i = 0;
    char* pch = strtok((char*)src.c_str(), ".");
    while (pch != NULL) {
        dest->ip[i] = atoi(pch);
        pch = strtok(NULL, ".");
        i++;
    }
}


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

void webserver_task( void * parameter ) {

    vTaskDelay(pdMS_TO_TICKS(2000));
    config_load(  );

    //########################  reading config file ########################################
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    Serial.println("SPIFF successfully mounted");
    File dataFile = SPIFFS.open(file, "r");   //Open File for reading
    Serial.println("Reading Configuration Data from File:");
    if ( !dataFile) {
        Serial.println("Count file open failed on read.");
    }
    else {
        for (int i = 0; i < dataFile.size(); i++) { //Read upto complete file size
            abc += (char)dataFile.read();
        }
        dataFile.close();
    }

    Serial.print(abc);
    Serial.println("");
    Serial.println("");


    WiFi.mode(WIFI_AP_STA);
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@  EEPROM read FOR SSID-PASSWORD- ACCESS POINT IP @@@@@@@@@@@@@@@@@@@@@@@@@@@@
    

    Serial.printf("Access point SSID: %s\n", cfg.ssid);
    Serial.printf("Access point PASSWORD: %s\n", cfg.pass);
    Serial.printf("Access point ADDRESS: %s\n", cfg.apaddr);
    //##############################   ACESS POINT begin on given credential #################################

    if ( (cfg.ssid[0] == 0 ) && (cfg.pass[0] == 0) ) {
        Serial.println("\n###  FIRST TIME SSID PASSWORD SET  ### ");
        StaticJsonDocument<500> doc;
        auto error = deserializeJson(doc, abc);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }
        strcpy( cfg.ssid, root["AP_name"] );
        strcpy( cfg.pass, root["AP_pass"] );
        config_savecfg( );
    }
    
    Serial.printf("Set up access point. SSID: %s, PASS: %s\n", cfg.ssid, cfg.pass);
    WiFi.softAP( cfg.ssid, cfg.pass);
    apname = cfg.ssid;
    vTaskDelay( pdMS_TO_TICKS(800) );


    if ( cfg.apaddr[0] == 0 ) {
        Serial.println("\nFIRST TIME AP ADDRESS SETTING 192.168.4.1");
        StaticJsonDocument<500> doc;
        auto error = deserializeJson(doc, abc);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }
        strcpy(cfg.apaddr, root["AP_IP"]);
        config_savecfg( );
    }

    Serial.printf("Access point ADDRESS: %s\n", cfg.apaddr);
    String eap = cfg.apaddr;
    String ip1, ip2, ip3, ip4;
    ipAdress(eap, ip1, ip2, ip3, ip4);
    IPAddress Ip(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
    IPAddress NMask(255, 255, 0, 0);

    WiFi.softAPConfig(Ip, Ip, NMask );
    myIP = WiFi.softAPIP().toString();

    Serial.printf("#### SERVER STARTED ON THIS: %s ###\n", myIP.c_str());
  

    //#########################  HTML+JS+CSS  HANDLING #####################################

    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/login.html", "text/html");
    });
    server.on("/main.html", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/main.html", "text/html");
    });
    server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/js/bootstrap.min.js", "text/javascript");
    });
    server.on("/js/jquery-1.12.3.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/js/jquery-1.12.3.min.js", "text/javascript");
    });
    server.on("/js/pixie-custom.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/js/pixie-custom.js", "text/javascript");
    });
    server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
    });
    server.on("/css/pixie-main.css", HTTP_GET, [](AsyncWebServerRequest * request) {
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

    //###################################   ACTIONS FROM WEBPAGE BUTTTONS  ##############################

    server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
        StaticJsonDocument<500> doc;
        auto error = deserializeJson(doc, abc);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }
        request->send(200, "text/plain", root["Admin_pass"]);
    });

    server.on("/main", HTTP_GET, [](AsyncWebServerRequest * request) {
        
        String content = "{\"myIP\":\"" + myIP + "\",\"localIP\":\"" + LOCAL_IP + "\",\"s_pass\":\"" + s_pass + "\",\"wsid\":\"" + wsid + "\",\"c_id\":\"" + c_id + "\",\"Service\":\"" + Service + "\",\"host_ip\":\"" + host_ip + "\",\"port\":\"" + port + "\",\"topic\":\"" + p_topic + "\",\"apname\":\"" + apname + "\",\"service\":\"" + service_s + "\",\"MAC\":\"" + getMacAddress() + "\"}";
        Serial.println(content);
        request->send(200, "application/json", content);
    });

    server.on("/serviceData", HTTP_GET, [](AsyncWebServerRequest * request) {
        //struct service_config const* serv = &cfg.service;
        DynamicJsonDocument doc(1024);

        doc["host"]    = std::string( cfg.service.host_ip, strlen(cfg.service.host_ip));
        doc["port"]    = cfg.service.port;
        doc["id"]      = std::string(cfg.service.client_id, strlen(cfg.service.client_id));
        doc["QOS"]     = cfg.service.qos;
        doc["user"]    = std::string(cfg.service.username, strlen(cfg.service.username));
        doc["pass"]    = std::string(cfg.service.password, strlen(cfg.service.password));  
        doc["temp_tp"] = std::string(cfg.service.temp.topic, strlen(cfg.service.temp.topic));
        doc["temp_tm"] = cfg.service.temp.interval;
        doc["temp_ud"] = std::string(cfg.service.temp.unit, strlen(cfg.service.temp.unit));
        doc["ping_tp"] = std::string(cfg.service.ping.topic, strlen(cfg.service.ping.topic));
        doc["ping_tm"] = cfg.service.ping.interval;
        doc["ping_ud"] = std::string(cfg.service.ping.unit, strlen(cfg.service.ping.unit));
        doc["rel1_tp"] = std::string(cfg.service.relay1.topic, strlen(cfg.service.relay1.topic));
        doc["rel2_tp"] = std::string(cfg.service.relay2.topic, strlen(cfg.service.relay2.topic));
        doc["en_tp"]   = std::string(cfg.service.enableTemp.topic , strlen(cfg.service.enableTemp.topic));

        String content;
        serializeJson(doc, content);
        Serial.println(content);
        request->send(200, "application/json", content);
    });

    server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest * request) {
        String scan_wifi = request->getParam("scan_wifi")->value();
        if (scan_wifi) {
            Serial.println("scan_wifi");
            String json = "[";
            int n = WiFi.scanNetworks();
            if (n == 0){
                Serial.println("no networks found");
            }
            else {
                Serial.print(n);
                Serial.println(" networks found");
                for (int i = 0; i < n; ++i) {
                    // Print SSID and RSSI for each network found
                    if (i)
                        json += ", ";
                    json += " {";
                    json += "\"rssi\":" + String(WiFi.RSSI(i));
                    json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                    json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                    json += ",\"channel\":" + String(WiFi.channel(i));
                    json += ",\"secure\":" + String(WiFi.encryptionType(i));
                    //                json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
                    json += "}";
                    if (i == (n - 1)) {
                        json += "]";
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(10));
                Serial.println(json);
                request->send(200, "application/json", json);
            }
        }
    });

    //################################# AP SSID-PASSWORD-IP RECEIVING FROM WEB PAGE WRITING TO EEPROM  ###############################

    server.on("/applyBtnFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String txtssid = request->getParam("txtssid")->value();
        if (txtssid.length() > 0) {
            strcpy(cfg.ssid, txtssid.c_str());
            Serial.printf("WRITING AP SSID :: %s\n", cfg.ssid);
        }

        String txtpass = request->getParam("txtpass")->value();
        if ( txtpass.length() > 0) {
            strcpy(cfg.pass, txtpass.c_str());
            Serial.printf("WRITING AP PASS :: %s\n", cfg.pass);
        }

        String txtapip = request->getParam("txtaplan")->value();
        if ( txtapip.length() > 0) {
            strcpy(cfg.apaddr , txtapip.c_str());
            Serial.printf("WRITING AP IP :: %s\n", cfg.apaddr);
        }
        
        config_savecfg( );
        request->send(200, "text/plain", "ok");
    });


    //#####################################  Receving WIFI credential from WEB Page ############################

    server.on("/connectBtnFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {

        String  wifi_ssid = request->getParam("wifi_ssid")->value();
        if (wifi_ssid.length() > 0) {
            strcpy(cfg.wifi.ssid, wifi_ssid.c_str());
            Serial.printf("WRITING WIFI SSID :: %s\n", cfg.wifi.ssid);
        }
        
        String  wifi_pass = request->getParam("wifi_pass")->value();
        if ( wifi_pass.length() > 0) {
            strcpy(cfg.wifi.pass, wifi_pass.c_str());
            Serial.printf("WRITING WIFI PASS :: %s\n", cfg.wifi.pass);
        }

        String  wifi_MODE  = request->getParam("wifi_MODE")->value();
        if (wifi_MODE == "dhcp") {
            cfg.wifi.dhcp = true;
            Serial.printf("WRITING WIFI DHCP :: %s\n", cfg.wifi.dhcp ? "true" : "false"); 
        }
        else if (wifi_MODE == "static") {
            
            String ipstatic = request->getParam("txtipadd")->value();
            String netmask  = request->getParam("net_m")->value();
            String gateway  = request->getParam("G_add")->value();
            String dns1     = request->getParam("P_dns")->value();
            String dns2     = request->getParam("S_dns")->value();
        
            Serial.println(ipstatic);
            Serial.println(netmask);
            Serial.println(gateway);
            Serial.println(dns1);
            Serial.println(dns2);

            if ( wifi_MODE.length() > 0) {
                cfg.wifi.dhcp = false;
                Serial.println(" Mode STATIC selected ");
            }

            if ( ipstatic.length() > 0)
                stringToIp(&cfg.wifi.ip, ipstatic);
            
            if ( netmask.length() > 0)
                stringToIp(&cfg.wifi.netmask, netmask);

            if ( gateway.length() > 0)
                stringToIp(&cfg.wifi.gateway, gateway);

            if ( dns1.length() > 0)
                stringToIp(&cfg.wifi.primaryDNS, dns1);

            if ( dns2.length() > 0)
                stringToIp(&cfg.wifi.secondaryDNS, dns2);
        }

        config_savecfg( );
        request->send(200, "text/plain", "ok");

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
            //config_setdefault( ); 
            Serial.print("restarting device");
            //SPIFFS.remove("/ServiceData_jsonfile.txt");
        }
        request->send(200, "text/plain", "ok");
    });
    
    //############################### RECEIVING DATA SEND MMETHODS HTTP-MQTT-TCP ##############################
    server.on("/applyServiceFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {
        String parameters = request->getParam("parameters")->value();
        Serial.println(parameters);
        
        const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
        DynamicJsonDocument doc(capacity);
        auto error = deserializeJson(doc, parameters);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }

        memset( &cfg.service, 0, sizeof( cfg.service ) );

        if (root.containsKey("host")) 
            strcpy(cfg.service.host_ip, root["host"]);

        if (root.containsKey("port")) 
            cfg.service.port = root["port"];

        if (root.containsKey("id")) 
            strcpy(cfg.service.client_id, root["id"]);

        if (root.containsKey("QOS")) 
            cfg.service.qos = root["QOS"];

        if (root.containsKey("user")) 
            strcpy(cfg.service.username, root["user"]);

        if (root.containsKey("pass")) 
            strcpy(cfg.service.password, root["pass"]);

        if (root.containsKey("temp_tp")) 
            strcpy(cfg.service.temp.topic, root["temp_tp"]);
        
        if (root.containsKey("temp_tm")) 
            cfg.service.temp.interval = root["temp_tm"];

        if (root.containsKey("temp_ud")) {
            strcpy(cfg.service.temp.unit, root["temp_ud"]);
        }

        if (root.containsKey("ping_tp")) 
            strcpy(cfg.service.ping.topic, root["ping_tp"]);
        
        if (root.containsKey("ping_tm")) 
            cfg.service.ping.interval = root["ping_tm"];

        if (root.containsKey("ping_ud")) {
            strcpy(cfg.service.ping.unit, root["ping_ud"]);
        }

        if (root.containsKey("rel1_tp")) 
            strcpy(cfg.service.relay1.topic, root["rel1_tp"]);

        if (root.containsKey("rel2_tp")) 
            strcpy(cfg.service.relay2.topic, root["rel2_tp"]);

        if (root.containsKey("en_tp")) 
            strcpy(cfg.service.enableTemp.topic, root["en_tp"]);

        
        Serial.printf("WRITING HOST IP :: %s\n", cfg.service.host_ip);
        Serial.printf("WRITING PORT :: %d\n", cfg.service.port);
        Serial.printf("WRITING CLIENT_ID :: %s\n", cfg.service.client_id);
        Serial.printf("WRITING QOS :: %d\n", cfg.service.qos);
        Serial.printf("WRITING USER :: %s\n", cfg.service.username);
        Serial.printf("WRITING PASS :: %s\n", cfg.service.password);
        Serial.printf("WRITING TEMP_TOPIC :: %s\n", cfg.service.temp.topic);
        Serial.printf("WRITING TEMP_INTER :: %d\n", cfg.service.temp.interval);
        Serial.printf("WRITING TEMP_UND :: %s\n", cfg.service.temp.unit);
        Serial.printf("WRITING PING_TOPIC :: %s\n", cfg.service.ping.topic);
        Serial.printf("WRITING PING_INTER :: %d\n", cfg.service.ping.interval);
        Serial.printf("WRITING PING_UND :: %s\n", cfg.service.ping.unit);
        Serial.printf("WRITING REL1_TOPIC :: %s\n", cfg.service.relay1.topic);
        Serial.printf("WRITING REL2_TOPIC :: %s\n", cfg.service.relay2.topic);
        Serial.printf("WRITING EN_TOPIC :: %s\n", cfg.service.enableTemp.topic);

        config_savecfg( );
        request->send(200, "text/plain", "ok");
    });
    
    //################################   WiFi settings Read   #####################################
#if 0
    for (int wsidaddress = 66; EEPROM.read(wsidaddress) != '\0' ; ++wsidaddress) {
        wsid += char(EEPROM.read(wsidaddress));
    }

    Serial.println("");
    Serial.println("");
    Serial.print("Wifi SSID: ");
    Serial.println(wsid);

    for (int passaddress = 88 ; passaddress < 103; ++passaddress) {
        wpass += char(EEPROM.read(passaddress));
    }
    Serial.print("Wifi PASSword: ");
    Serial.println(wpass);

    String W_mode = "";
    for (int modeaddress = 116 ; modeaddress < 122 ; ++modeaddress) {
        W_mode += char(EEPROM.read(modeaddress));
    }
    Serial.print("WI-FI_MODE: ");
    Serial.println(W_mode);

    if (wsid == NULL) {
        wsid = "Not Given";
        LOCAL_IP = "network not set";
    }

    //####################################### MODE CHECKING(DHCP-STATIC) AND WIFI BEGIN #######################################
    if (W_mode == "dhcp") {
        WiFi.begin(wsid.c_str(), wpass.c_str());
        delay(2000);
        if (testWifi()) {
            Serial.print(WiFi.status());
            Serial.println("YOU ARE CONNECTED");
            LOCAL_IP = WiFi.localIP().toString();
            Serial.println(LOCAL_IP);
        }
    }

    if (W_mode == "static") {
        for (int passaddress = 123 ; passaddress < 143; ++passaddress){
            static_ip += char(EEPROM.read(passaddress));
        }
        Serial.print("W-static_ip: ");
        Serial.println(static_ip);
        ipAdress(static_ip, ip1, ip2, ip3, ip4);
        String sb1, sb2, sb3, sb4;
        String sub_net = "";
        for (int passaddress = 143 ; passaddress < 160; ++passaddress) {
            sub_net += char(EEPROM.read(passaddress));
        }
        Serial.print("sub_net-: ");
        Serial.println(sub_net);
        delay(1000);

        ipAdress(sub_net, sb1, sb2, sb3, sb4);
        String g1, g2, g3, g4;
        String G_add = "";
        for (int passaddress = 161 ; passaddress < 180; ++passaddress) {
            G_add += char(EEPROM.read(passaddress));
        }
        Serial.print("G_add-: ");
        Serial.println(G_add);
        ipAdress(G_add, g1, g2, g3, g4);

        String p1, p2, p3, p4;
        String P_dns = "";
        for (int passaddress = 181 ; passaddress < 200; ++passaddress) {
            P_dns += char(EEPROM.read(passaddress));
        }
        Serial.print("Primary_dns-: ");
        Serial.println(P_dns);
        ipAdress(P_dns, p1, p2, p3, p4);

        String s1, s2, s3, s4;
        String S_dns = "";
        for (int passaddress = 201 ; passaddress < 216; ++passaddress) {
            S_dns += char(EEPROM.read(passaddress));
        }
        Serial.print("SECONDARY_dns-: ");
        Serial.println(S_dns);
        ipAdress(S_dns, s1, s2, s3, s4);

        IPAddress S_IP(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
        IPAddress gateway(g1.toInt(), g2.toInt(), g3.toInt(), g4.toInt());
        IPAddress subnet(sb1.toInt(), sb2.toInt(), sb3.toInt(), sb4.toInt());
        IPAddress primaryDNS(p1.toInt(), p2.toInt(), p3.toInt(), p4.toInt()); //optional
        IPAddress secondaryDNS(s1.toInt(), s2.toInt(), s3.toInt(), s4.toInt()); //optional

        if (!WiFi.config(S_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
            Serial.println("STA Failed to configure");
        }

        WiFi.begin(wsid.c_str(), wpass.c_str());
        delay(1000);

        if (testWifi()) {
            Serial.print(WiFi.status());
            Serial.println("YOU ARE CONNECTED");
            LOCAL_IP = WiFi.localIP().toString();
            Serial.println(LOCAL_IP);
        }
    }
#endif

    if (Service == NULL) {
        service_s = "Not Set";
    }
    else {
        service_s = Service + "(SET)";
    }
    server.begin();

    for(;;){ // infinite loop
        if (WiFi.status() == WL_CONNECTED) {

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
        delay(5000);
    }

}




