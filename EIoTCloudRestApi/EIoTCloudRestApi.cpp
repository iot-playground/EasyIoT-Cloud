 /*
  Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community forum on website do not contact author directly
 
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "EIoTCloudRestApi.h"
#include <Arduino.h>



EIoTCloudRestApi::EIoTCloudRestApi() {
}


void EIoTCloudRestApi::begin() {
	wifiConnect();
}

void EIoTCloudRestApi::sendParameter(const char * instaceParamId, bool value)
{	
	if (value)
		sendParameter(instaceParamId, "1");
	else
		sendParameter(instaceParamId, "0");
}

void EIoTCloudRestApi::sendParameter(const char * instaceParamId, int value)
{	
	sendParameter(instaceParamId, String(value));
}

void EIoTCloudRestApi::sendParameter(const char * instaceParamId, float value)
{	
	sendParameter(instaceParamId, String(value));
}

void EIoTCloudRestApi::sendParameter(const char * instaceParamId, const char * value)
{
	sendParameter(instaceParamId, String(value));
}

void EIoTCloudRestApi::sendParameter(const char * instaceParamId, String value)
{
  WiFiClient client;
   
  while(!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT)) {
    debug("connection failed");
	wifiConnect(); 
  }

  String url = "";
  // URL: /RestApi/SetParameter/[instance id]/[parameter id]/[value]
  url += "/RestApi/SetParameter/" + String(instaceParamId) + "/" + value; // generate EasIoT cloud update parameter URL

  debug("POST data to URL: ");
#ifdef DEBUG
  char buff[300];
  url.toCharArray(buff, 300);
  debug(buff);
#endif
  
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" + 
               "Connection: close\r\n" + 
               "Content-Length: 0\r\n" + 
               "\r\n");

  delay(100);
  while(client.available()){
#ifdef DEBUG
  String line = client.readStringUntil('\r');
  line.toCharArray(buff, 300);
  debug(buff);
#endif
  }
}



void EIoTCloudRestApi::wifiConnect()
{
  debug("Connecting to AP");
  WiFi.begin(AP_SSID, AP_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  debug("");
  debug("WiFi connected");  
}



#ifdef DEBUG
void EIoTCloudRestApi::printDebug(const char *fmt, ...) {
	char buff[300];

	va_list args;
	va_start(args, fmt);
	va_end(args);
		
	vsnprintf_P(buff, 299, fmt, args);
	
	va_end(args);
	Serial.print(buff);
	Serial.flush();
}
#endif