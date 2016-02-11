#include <ESP8266WiFi.h>
#include "EIoTCloudRestApiV1.0.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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
#define AP_USERNAME "xxxx"
#define AP_PASSWORD "xxxx"
#define INSTANCE_ID "xxxx"



#define REPORT_INTERVAL 1

#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);


#define CONFIG_START 0
#define CONFIG_VERSION "v01"

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

String moduleId = "";
String parameterId = "";
float tempOld = 0;

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
      bool modname = eiotcloud.SetModulName(moduleId, "Room temperature");
      DEBUG_PRINT("SetModulName: ");
      DEBUG_PRINTLN(modname);

      // add image settings parameter
      String parameterImgId = eiotcloud.NewModuleParameter(moduleId, "Settings.Icon1");
      DEBUG_PRINT("parameterImgId: ");
      DEBUG_PRINTLN(parameterImgId);

      // set module image
      bool valueRet1 = eiotcloud.SetParameterValue(parameterImgId, "temperature.png");
      DEBUG_PRINT("SetParameterValue: ");
      DEBUG_PRINTLN(valueRet1);
      
      // now add parameter to display temperature
      parameterId = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter1");
      DEBUG_PRINT("ParameterId: ");
      DEBUG_PRINTLN(parameterId);

      //set parameter description
      bool valueRet2 = eiotcloud.SetParameterDescription(parameterId, "Temperature");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      // see http://meyerweb.com/eric/tools/dencoder/ how to encode °C 
      bool valueRet3 = eiotcloud.SetParameterUnit(parameterId, "%C2%B0C");
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
      delay(1);

    // read module ID from storage
    moduleId = String(storage.moduleId);
    // read token ID from storage
    eiotcloud.SetToken(storage.token);    
    // read Sensor.Parameter1 ID from cloud
    parameterId = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter1");
    DEBUG_PRINT("parameterId: ");
    DEBUG_PRINTLN(parameterId);
}


void loop() {
  float temp;

  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINTLN(temp);
  } while (temp == 85.0 || temp == (-127.0));



  if (tempOld != temp)
  {
    // send temperature
    bool valueRet = eiotcloud.SetParameterValue(parameterId, String(temp));
    DEBUG_PRINT("SetParameterValue: ");
    DEBUG_PRINTLN(valueRet);
    tempOld = temp;
  }
  delay(1000 * 60 * REPORT_INTERVAL);
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
