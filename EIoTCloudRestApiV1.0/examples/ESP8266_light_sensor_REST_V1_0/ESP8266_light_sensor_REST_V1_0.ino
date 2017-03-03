/*
 Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community fourum on website do not contact author directly
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <ESP8266WiFi.h>
#include "EIoTCloudRestApiV1.0.h"
#include <EEPROM.h>

#define DEBUG_PROG 

#ifdef DEBUG_PROG
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINT(x)    Serial.print(x)
#else
  #define DEBUG_PRINTLN(x) 
  #define DEBUG_PRINT(x)
#endif

EIoTCloudRestApi eiotcloud;

// change those lines
#define AP_USERNAME "xxx"
#define AP_PASSWORD "xxx"

#define TOKEN "xxx"

#define REPORT_INTERVAL 1

#define CONFIG_START 0
#define CONFIG_VERSION "v01"


#define MIN_VALUE 25
#define MAX_VALUE 800

#define SENSOR_PIN A0    // select the input pin for the potentiometer


struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  uint moduleId;
} storage = {
  CONFIG_VERSION,
  // The default module 0 - invalid module
  0,
};

String moduleId = "";
String parameterId = "";

int sensorValue = 0; 


void setup() {
    Serial.begin(9600);
    DEBUG_PRINTLN("Start...");

    EEPROM.begin(512);
    loadConfig();

    eiotcloud.begin(AP_USERNAME, AP_PASSWORD);
    eiotcloud.SetToken(TOKEN);

    // if first time get new token and register new module
    // here hapend Plug and play logic to add module to Cloud
    if (storage.moduleId == 0)
    {
      

      // add new module and configure it
      moduleId = eiotcloud.ModuleNew();
      DEBUG_PRINT("ModuleId: ");
      DEBUG_PRINTLN(moduleId);
      storage.moduleId = moduleId.toInt();
      
      // stop if module ID is not valid
      if (storage.moduleId == 0)
      {
        DEBUG_PRINTLN("Check Account limits -> module limit.");
        while(true) delay(1);
      }
      // set module type
      bool modtyperet = eiotcloud.SetModulType(moduleId, "MT_GENERIC");
      DEBUG_PRINT("SetModulType: ");
      DEBUG_PRINTLN(modtyperet);
      
      // set module name
      bool modname = eiotcloud.SetModulName(moduleId, "LUX meter");
      DEBUG_PRINT("SetModulName: ");
      DEBUG_PRINTLN(modname);

      // add image settings parameter
      String parameterImgId = eiotcloud.NewModuleParameter(moduleId, "Settings.Icon1");
      DEBUG_PRINT("parameterImgId: ");
      DEBUG_PRINTLN(parameterImgId);

      // set module image
      bool valueRet1 = eiotcloud.SetParameterValue(parameterImgId, "radiation.png");
      DEBUG_PRINT("SetParameterValue: ");
      DEBUG_PRINTLN(valueRet1);
      
      // now add parameter to value
      parameterId = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter1");
      DEBUG_PRINT("ParameterId: ");
      DEBUG_PRINTLN(parameterId);

      //set parameter description
      bool valueRet2 = eiotcloud.SetParameterDescription(parameterId, "LUX");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      bool valueRet3 = eiotcloud.SetParameterUnit(parameterId, "%");
      DEBUG_PRINT("SetParameterUnit: ");
      DEBUG_PRINTLN(valueRet3);

      //Set parameter LogToDatabase
      bool valueRet4 = eiotcloud.SetParameterLogToDatabase(parameterId, true);
      DEBUG_PRINT("SetLogToDatabase: ");
      DEBUG_PRINTLN(valueRet4);

      //SetAvreageInterval
      bool valueRet5 = eiotcloud.SetParameterAverageInterval(parameterId, "10");
      DEBUG_PRINT("SetAvreageInterval: ");
      DEBUG_PRINTLN(valueRet5);

      // save configuration
      saveConfig();
    }

    // if something went wrong, wiat here
    if (storage.moduleId == 0)
      while(true) delay(1);

    // read module ID from storage
    moduleId = String(storage.moduleId);
    // read Sensor.Parameter1 ID from cloud
    parameterId = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter1");
    DEBUG_PRINT("parameterId: ");
    DEBUG_PRINTLN(parameterId);
}

void loop() {
  sensorValue = analogRead(SENSOR_PIN);
  float level = 100 - ((sensorValue - MIN_VALUE) * 100 / (MAX_VALUE - MIN_VALUE));
  
  sensorValue = level;
  DEBUG_PRINT("LUX = ");
  DEBUG_PRINTLN(sensorValue);

  bool valueRet = eiotcloud.SetParameterValue(parameterId, String(sensorValue));
  DEBUG_PRINT("SetParameterValue: ");
  DEBUG_PRINTLN(valueRet);

  delay(1000 * 10 * REPORT_INTERVAL);
}



void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}


void saveConfig() {
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));

  EEPROM.commit();
}
