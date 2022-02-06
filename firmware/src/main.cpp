#include "Arduino.h"
#include "FreeRTOS.h"
#include "webserver.h"
#include "mqtt_task.h"
#include "config-mng.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>


static const char* file = "/config.json";   //Enter your file name

static void loadConfig( void ) {
    String abc;
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
        Serial.print(abc);
    }


    //@@@@@@@@@@@@@@@@@@@@@@@@@@@  EEPROM read FOR SSID-PASSWORD- ACCESS POINT IP @@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct ap_config* ap = &cfg.ap;
    Serial.printf("Access point SSID: %s\n", ap->ssid);
    Serial.printf("Access point PASSWORD: %s\n", ap->pass);
    Serial.printf("Access point ADDRESS: %s\n", ap->addr);
    
    //##############################   ACESS POINT begin on given credential #################################
    if ( (ap->ssid[0] == 0 ) || (ap->pass[0] == 0) || ap->addr[0] == 0 ) {
        Serial.println("\n###  FIRST TIME SSID PASSWORD AND AP ADDRESS SET  ### ");
        StaticJsonDocument<500> doc;
        auto error = deserializeJson(doc, abc);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }
        strcpy( ap->ssid, root["AP_name"] );
        strcpy( ap->pass, root["AP_pass"] );
        strcpy( ap->addr, root["AP_IP"]);
        config_savecfg( );
    }
}


void setup() {
 
    Serial.begin(115200);

    vTaskDelay(pdMS_TO_TICKS(2000));
    config_load(  );

    //########################  reading config file ########################################
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    loadConfig();
    
    // Now set up two Tasks to run independently.
    xTaskCreate( webserver_task , "webserver-task",  1024*10  ,NULL  ,  1,  NULL );
    xTaskCreate( mqtt_task ,      "mqtt-task",       1024*2   ,NULL  ,  1,  NULL );
    //vTaskStartScheduler();
}


void loop() { 

}