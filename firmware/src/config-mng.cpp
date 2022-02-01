#include "config-mng.h"
#include <Arduino.h>
#include <EEPROM.h>


#define CFG_VER 1

#define EEPROM_SIZE 1024

struct config cfg;
static int const cfgaddr = 0; 



static void setdefault( struct config* cfg ) { 
    memset( cfg, 0 , sizeof( struct config));
    strcpy(cfg->wifi.mode, "dhcp");
}

void config_setdefault( void ) { 
    setdefault( &cfg );
    EEPROM.put( cfgaddr, cfg );
    EEPROM.commit();
}

void config_savecfg( ) {
    
    // replace values in byte-array cache with modified data
    // no changes made to flash, all in local byte-array cache
    Serial.println("Saving config to EEPROM");
    
    EEPROM.put( cfgaddr, cfg );


    // actually write the content of byte-array cache to
    // hardware flash.  flash write occurs if and only if one or more byte
    // in byte-array cache has been changed, but if so, ALL 512 bytes are 
    // written to flash
    EEPROM.commit(); 

}

void config_load(  ) {
    
    if ( !EEPROM.begin(EEPROM_SIZE) ) {
        Serial.println("failed to initialise EEPROM"); 
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    
    int cfgversion=0;
    int eeAdress = cfgaddr;
    EEPROM.get( eeAdress, cfg );
    eeAdress += sizeof( struct config );
    EEPROM.get( eeAdress, cfgversion );

    if (eeAdress > EEPROM_SIZE) {
        Serial.println("ERROR > EEPROM insufficient size");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    if ( cfgversion != CFG_VER ) {
        setdefault( &cfg );
        eeAdress = 0;
        EEPROM.put( eeAdress, cfg );
        eeAdress += sizeof( struct config );
        cfgversion = CFG_VER;
        EEPROM.put( eeAdress, cfgversion );
        Serial.printf("LOAD DEFAULT\n");
    }

    EEPROM.commit(); 

} 



