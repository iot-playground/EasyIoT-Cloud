/*
  Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community forum on website do not contact author directly
 
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 Changelog
 ----------------
 15.10.2015 - first version

 */
#ifndef EIoTCloudRestApi_h
#define EIoTCloudRestApi_h

#include "EIoTCloudRestApiConfig.h"
#include <ESP8266WiFi.h>

#ifdef DEBUG
#define debug(x,...) printDebug(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#endif


#define EIOT_CLOUD_ADDRESS     "cloud.iot-playground.com"
#define EIOT_CLOUD_PORT        40404


class EIoTCloudRestApi 
{
	public:
		EIoTCloudRestApi();	
		void begin();

		void sendParameter(const char * instaceParamId, bool value);
		void sendParameter(const char * instaceParamId, int value);
		void sendParameter(const char * instaceParamId, float value);
		void sendParameter(const char * instaceParamId, String value);
		void sendParameter(const char * instaceParamId, const char * value);

#ifdef DEBUG
		void printDebug(const char *fmt, ...);
#endif		
protected:
	void wifiConnect();

};

#endif

