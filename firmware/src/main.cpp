#include "Arduino.h"
#include "FreeRTOS.h"
#include "webserver.h"


void setup() {
 
    Serial.begin(115200);
    
    // Now set up two Tasks to run independently.
    xTaskCreate( webserver_task , "webserver-task",  512*10  ,NULL  ,  1,  NULL );

    //vTaskStartScheduler();
}


void loop() { 

}