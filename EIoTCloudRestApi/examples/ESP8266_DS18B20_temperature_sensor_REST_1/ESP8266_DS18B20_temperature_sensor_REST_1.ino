/*
 Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community fourum on website do not contact author directly
 
 Code based on https://github.com/DennisSc/easyIoT-ESPduino/blob/master/sketches/ds18b20.ino
 
 External libraries:
 - https://github.com/milesburton/Arduino-Temperature-Control-Library
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "EIoTCloudRestApi.h"


// EasyIoT Cloud definitions - change EIOT_CLOUD_INSTANCE_PARAM_ID
#define EIOT_CLOUD_INSTANCE_PARAM_ID    "xxxxx"
#define REPORT_INTERVAL 60 // in sec



#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
EIoTCloudRestApi eiotcloud;


float oldTemp;


void setup() {
  Serial.begin(115200);

  eiotcloud.begin();
    
  oldTemp = -1;
}

void loop() {
  float temp;
  
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(temp);
  } while (temp == 85.0 || temp == (-127.0));
  
  if (temp != oldTemp)
  {
    eiotcloud.sendParameter(EIOT_CLOUD_INSTANCE_PARAM_ID, temp);
    oldTemp = temp;
  }
  
  int cnt = REPORT_INTERVAL;
  
  while(cnt--)
    delay(1000);
}
