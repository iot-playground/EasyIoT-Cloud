/*
 Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community fourum on website do not contact author directly
 
 External libraries:
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <ESP8266WiFi.h>
#include "DHT.h"
#include "EIoTCloudRestApi.h"



// EasyIoT Cloud definitions - change EIOT_CLOUD_TEMP_INSTANCE_PARAM_ID and EIOT_CLOUD_HUM_INSTANCE_PARAM_ID
#define EIOT_CLOUD_TEMP_INSTANCE_PARAM_ID    "xxxx"
#define EIOT_CLOUD_HUM_INSTANCE_PARAM_ID     "xxxx"
#define REPORT_INTERVAL 60 // in sec


#define DHT22_PIN 2  // DHT22 pin


float oldTemp;
float oldHum;

DHT dht;
EIoTCloudRestApi eiotcloud;

void setup() {
  Serial.begin(115200);
  eiotcloud.begin();

  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)");

  dht.setup(DHT22_PIN); // 
  
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

  if (temp != oldTemp)
  {
    eiotcloud.sendParameter(EIOT_CLOUD_TEMP_INSTANCE_PARAM_ID, temp);
    oldTemp = temp;
  }

  if (hum != oldHum)
  {
    eiotcloud.sendParameter(EIOT_CLOUD_HUM_INSTANCE_PARAM_ID, hum);
    oldHum = hum;
  }

  int cnt = REPORT_INTERVAL;
  
  while(cnt--)
    delay(1000);
}
