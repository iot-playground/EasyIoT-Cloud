/*
 Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community forum on website do not contact author directly
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <ESP8266WiFi.h>
#include "EIoTCloudRestApi.h"

// EasyIoT Cloud definitions - change EIOT_CLOUD_INSTANCE_PARAM_ID
#define EIOT_CLOUD_INSTANCE_PARAM_ID    "xxxx"

#define INPUT_PIN        2

EIoTCloudRestApi eiotcloud;
bool oldInputState;

void setup() {
  Serial.begin(115200);
  eiotcloud.begin();

  pinMode(INPUT_PIN, INPUT_PULLUP);
    
  oldInputState = !digitalRead(INPUT_PIN);
}

void loop() {
  int inputState = digitalRead(INPUT_PIN);;  
  
  if (inputState != oldInputState)
  {
    eiotcloud.sendParameter(EIOT_CLOUD_INSTANCE_PARAM_ID, (inputState == 1));
    oldInputState = inputState;
  }
}
