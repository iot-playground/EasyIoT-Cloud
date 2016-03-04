/*
  Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community forum on website do not contact author directly
 
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 Changelog
 ----------------
 10.02.2016 - first version
 04.03.2016 - added SetParameterValues
 
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

//#define EIOT_CLOUD_ADDRESS     "192.168.88.110"
//#define EIOT_CLOUD_PORT        12345


class EIoTCloudRestApi 
{	
	const char* _ssid;
	const char* _password;
	String		_token;


	public:
		EIoTCloudRestApi();	
		void begin(const char* ssid, const char* password, String token);
		void begin(const char* ssid, const char* password);

		String TokenNew(String instance);
		bool TokenList(String instance, int *, String**);
		void SetToken(String);
		String GetToken();

		String ModuleNew();
		bool SetModulType(String id, String moduleType);
		bool SetModulName(String id, String name);
		String NewModuleParameter(String id);
		String NewModuleParameter(String id, String name);

		String GetModuleParameterByName(String id, String parameterName);



		bool SetParameterName(String parameterId, String name);
		String GetParameterName(String parameterId);
		bool SetParameterDescription(String parameterId, String description);
		String GetParameterDescription(String parameterId);
		bool SetParameterUnit(String parameterId, String unit);
		String GetParameterUnit(String parameterId);
		bool SetParameterUINotification(String parameterId, bool uiNotification);
		String GetParameterUINotification(String parameterId);
		bool SetParameterLogToDatabase(String parameterId, bool logToDatabase);
		String GetParameterLogToDatabase(String parameterId);
		bool SetParameterAverageInterval(String parameterId, String avgInterval);				
		String GetParameterAverageInterval(String parameterId);
		bool SetParameterChartSteps(String parameterId, bool chartSteps);
		String GetParameterChartSteps(String parameterId);
		bool SetParameterValue(String parameterId, String value);
		String GetParameterValue(String parameterId);

		bool SetParameterValues(String values);
		
private:
	String parseId(WiFiClient* client);
	bool parseResponse(WiFiClient* client);
	bool setParameterProperty(String parameterId, String property, String value);
	bool setParameterProperty(String parameterId, String property, bool value);

	String getParameterProperty(String parameterId, String property);



	String parseParameter(WiFiClient* client, String property);


#ifdef DEBUG
		void printDebug(const char *fmt, ...);
#endif		
protected:
	void wifiConnect();
	
};

#endif

