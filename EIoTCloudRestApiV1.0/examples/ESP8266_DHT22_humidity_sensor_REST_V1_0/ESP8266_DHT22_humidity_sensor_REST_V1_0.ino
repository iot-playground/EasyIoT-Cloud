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
#include "DHT.h"

//#define DEBUG_PROG 

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
#define INSTANCE_ID "xxx"


#define CONFIG_START 0
#define CONFIG_VERSION "v01"

#define REPORT_INTERVAL 60 // in sec

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  char token[41];
  uint moduleId;
  //bool tokenOk; // valid token  
} storage = {
  CONFIG_VERSION,
  // token
  "1234567890123456789012345678901234567890",
  // The default module 0 - invalid module
  0,
  //0 // not valid
};

float oldTemp;
float oldHum;

DHT dht;

String moduleId = "";
String parameterId1 = "";
String parameterId2 = "";


void setup() {
    Serial.begin(115200);
    DEBUG_PRINTLN("Start...");

    EEPROM.begin(512);
    loadConfig();

    eiotcloud.begin(AP_USERNAME, AP_PASSWORD);

    // if first time get new token and register new module
    // here hapend Plug and play logic to add module to Cloud
    if (storage.moduleId == 0)
    {
      // get new token - alternarive is to manually create token and store it in EEPROM
      String token = eiotcloud.TokenNew(INSTANCE_ID);
      DEBUG_PRINT("Token: ");
      DEBUG_PRINTLN(token);
      eiotcloud.SetToken(token);

      // remember token
      token.toCharArray(storage.token, 41);

      // add new module and configure it
      moduleId = eiotcloud.ModuleNew();
      DEBUG_PRINT("ModuleId: ");
      DEBUG_PRINTLN(moduleId);
      storage.moduleId = moduleId.toInt();

      // set module type
      bool modtyperet = eiotcloud.SetModulType(moduleId, "MT_GENERIC");
      DEBUG_PRINT("SetModulType: ");
      DEBUG_PRINTLN(modtyperet);
      
      // set module name
      bool modname = eiotcloud.SetModulName(moduleId, "Humidity sensor");
      DEBUG_PRINT("SetModulName: ");
      DEBUG_PRINTLN(modname);

      // add image settings parameter
      String parameterImgId = eiotcloud.NewModuleParameter(moduleId, "Settings.Icon1");
      DEBUG_PRINT("parameterImgId: ");
      DEBUG_PRINTLN(parameterImgId);

      // set module image
      bool valueRet1 = eiotcloud.SetParameterValue(parameterImgId, "humidity.png");
      DEBUG_PRINT("SetParameterValue: ");
      DEBUG_PRINTLN(valueRet1);
      
      // now add parameter to display temperature
      parameterId1 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter1");
      DEBUG_PRINT("parameterId1: ");
      DEBUG_PRINTLN(parameterId1);

      //set parameter description
      bool valueRet2 = eiotcloud.SetParameterDescription(parameterId1, "Temperature");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      // see http://meyerweb.com/eric/tools/dencoder/ how to encode °C 
      bool valueRet3 = eiotcloud.SetParameterUnit(parameterId1, "%C2%B0C");
      DEBUG_PRINT("SetParameterUnit: ");
      DEBUG_PRINTLN(valueRet3);

      //Set parameter LogToDatabase
      bool valueRet4 = eiotcloud.SetParameterLogToDatabase(parameterId1, true);
      DEBUG_PRINT("SetLogToDatabase: ");
      DEBUG_PRINTLN(valueRet4);

      //SetAvreageInterval
      bool valueRet5 = eiotcloud.SetParameterAverageInterval(parameterId1, "10");
      DEBUG_PRINT("SetAvreageInterval: ");
      DEBUG_PRINTLN(valueRet5);


      // now add parameter to display humidity
      parameterId2 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter2");
      DEBUG_PRINT("parameterId2: ");
      DEBUG_PRINTLN(parameterId2);

      //set parameter description
      bool valueRet6 = eiotcloud.SetParameterDescription(parameterId2, "Humidity");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      bool valueRet7 = eiotcloud.SetParameterUnit(parameterId2, "%");
      DEBUG_PRINT("SetParameterUnit: ");
      DEBUG_PRINTLN(valueRet7);

      //Set parameter LogToDatabase
      bool valueRet8 = eiotcloud.SetParameterLogToDatabase(parameterId2, true);
      DEBUG_PRINT("SetLogToDatabase: ");
      DEBUG_PRINTLN(valueRet8);

      //SetAvreageInterval
      bool valueRet9 = eiotcloud.SetParameterAverageInterval(parameterId2, "10");
      DEBUG_PRINT("SetAvreageInterval: ");
      DEBUG_PRINTLN(valueRet9);

      // save configuration
      saveConfig();
    }

    // if something went wrong, wiat here
    if (storage.moduleId == 0)
      delay(1);

    // read module ID from storage
    moduleId = String(storage.moduleId);
    // read token ID from storage
    eiotcloud.SetToken(storage.token);    
    // read Sensor.Parameter1 ID from cloud
    parameterId1 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter1");
    DEBUG_PRINT("parameterId1: ");
    DEBUG_PRINTLN(parameterId1);

    parameterId2 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter2");
    DEBUG_PRINT("parameterId2: ");
    DEBUG_PRINTLN(parameterId2);


    Serial.println();
    Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)");

    dht.setup(2); // data pin 2
  
    oldTemp = -1;
    oldHum = -1;
}


void loop() {
  delay(dht.getMinimumSamplingPeriod());
  
  float hum = dht.getHumidity();
  float temp = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(hum, 1);
  Serial.print("\t\t");
  Serial.print(temp, 1);
  Serial.print("\t\t");
  Serial.println(dht.toFahrenheit(temp), 1);

  if (temp != oldTemp || hum != oldHum)
  {
    //sendTeperature(temp);
    eiotcloud.SetParameterValues("[{\"Id\": \""+parameterId1+"\", \"Value\": \""+String(temp)+"\" },{\"Id\": \""+parameterId2+"\", \"Value\": \""+String(hum)+"\" }]");
    oldTemp = temp;
    oldHum = hum;
  }

  int cnt = REPORT_INTERVAL;
  
  while(cnt--)
    delay(1000);
  
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
