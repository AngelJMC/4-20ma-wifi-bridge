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
    Serial.printf("Access point SSID: %s\n", cfg.ssid);
    Serial.printf("Access point PASSWORD: %s\n", cfg.pass);
    Serial.printf("Access point ADDRESS: %s\n", cfg.apaddr);
    
    //##############################   ACESS POINT begin on given credential #################################
    if ( (cfg.ssid[0] == 0 ) || (cfg.pass[0] == 0) || cfg.apaddr[0] == 0 ) {
        Serial.println("\n###  FIRST TIME SSID PASSWORD AND AP ADDRESS SET  ### ");
        StaticJsonDocument<500> doc;
        auto error = deserializeJson(doc, abc);
        JsonObject root = doc.as<JsonObject>();
        if (error) {
            Serial.println("parseObject() failed");
            return;
        }
        strcpy( cfg.ssid, root["AP_name"] );
        strcpy( cfg.pass, root["AP_pass"] );
        strcpy(cfg.apaddr, root["AP_IP"]);
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