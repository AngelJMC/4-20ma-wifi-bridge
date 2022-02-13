#include "Arduino.h"
#include "FreeRTOS.h"
#include "webserver.h"
#include "mqtt_task.h"
#include "config-mng.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>

#define CONFIG_ASYNC_TCP_RUNNING_CORE 
void setup() {
 
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));
    config_load(  );


    //########################  reading config file ########################################
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    
    // Now set up two Tasks to run independently.
    xTaskCreate( webserver_task , "webserver-task",  1024*10  ,NULL  ,  3,  NULL );
    xTaskCreate( mqtt_task ,      "mqtt-task",       1024*2   ,NULL  ,  1,  NULL );

}


void loop() { 

}