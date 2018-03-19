/*//-------------------------------------------------------------------------------
 *  MQTT.cpp
 *
 *  Implementation file for MQTT Wrapper
 *
 *  Wrapper for Arduino written by Ingo Randolf during
 *	eTextiles Summercamp 2015.
 *
 *	This library is intended to be used with esp8266 modules.
 *
 *
 *  This class wraps a slightly modified version of mqtt
 *	for esp8266 written by Tuan PM.
 *	Original code: https://github.com/tuanpmt/esp_mqtt
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 //-------------------------------------------------------------------------------*/
#include "MQTT.h"

#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "mqtt/debug.h"


//------------------------------------------------------------------------------------
// mqtt internal callbacks
//------------------------------------------------------------------------------------
static void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttConnectedCb) {
		_this->onMqttConnectedCb();
	}
}

static void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttDisconnectedCb) {
		_this->onMqttDisconnectedCb();
	}
}

static void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this && _this->onMqttPublishedCb) {
		_this->onMqttPublishedCb();
	}
}

static void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
	if (_this) {
		
		_this->_onMqttDataCb(topic, topic_len, data, data_len);
	}
}

static void mqttTimeoutCb(uint32_t *args)
{
	printf("TIMEOUT....\n");
	
	MQTT_Client* client = (MQTT_Client*)args;
	
	MQTT* _this = (MQTT*)client->user_data;
	
//	if (_this && _this->onMqttTimeoutCb) {
//		_this->onMqttTimeoutCb();
//	}
}


//------------------------------------------------------------------------------------
// MQTT class implementation
//------------------------------------------------------------------------------------
MQTT::MQTT(const char* client_id, const char* host, uint32_t port) :
	onMqttConnectedCb(0)
	,onMqttDisconnectedCb(0)
	,onMqttPublishedCb(0)
	,onMqttDataCb(0)
	,onMqttDataRawCb(0)
{
	// init connections
	MQTT_InitConnection(&mqttClient, (uint8_t*)host, port, 0);

	// init client
	if ( !MQTT_InitClient(&mqttClient, (uint8_t*)client_id, (uint8_t*)"", (uint8_t*)"", 120, 1) ) {
		MQTT_INFO("Failed to initialize properly. Check MQTT version.\r\n");
	}

	// init LWT
	MQTT_InitLWT(&mqttClient, (uint8_t*)"/lwt", (uint8_t*)"offline", 0, 0);

	// set user data
	mqttClient.user_data = (void*)this;

	// setup callbacks
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
	
	MQTT_OnTimeout(&mqttClient, mqttTimeoutCb);
}


MQTT::~MQTT()
{
	MQTT_DeleteClient(&mqttClient);
}


/*
 */
void MQTT::setClientId(const char* client_id)
{
	MQTT_SetUserId(&mqttClient, client_id);
}

void MQTT::setUserPwd(const char* user, const char* pwd)
{
	MQTT_SetUserPwd(&mqttClient, user, pwd);
}


/*
 */
void MQTT::connect()
{
	MQTT_Connect(&mqttClient);
}

void MQTT::disconnect()
{
	MQTT_Disconnect(&mqttClient);
}

bool MQTT::isConnected()
{
	return (mqttClient.connState >= TCP_CONNECTED);
}

/*
 */
bool MQTT::publish(const char* topic, const char* buf, uint32_t buf_len, int qos, int retain)
{
	return MQTT_Publish(&mqttClient, topic, buf, buf_len, qos, retain);
}

bool MQTT::publish(String& topic, String& data, int qos, int retain)
{
	return publish(topic.c_str(), data.c_str(), data.length(), qos, retain);
}

bool MQTT::publish(String& topic, const char* buf, uint32_t buf_len, int qos, int retain)
{
	return publish(topic.c_str(), buf, buf_len, qos, retain);
}

bool MQTT::publish(const char* topic, String& data, int qos, int retain)
{
	return publish(topic, data.c_str(), data.length(), qos, retain);
}


/*
 */
bool MQTT::subscribe(const char* topic, uint8_t qos)
{
	return MQTT_Subscribe(&mqttClient, (char*)topic, qos);
}

bool MQTT::subscribe(const String& topic, uint8_t qos)
{
	return MQTT_Subscribe(&mqttClient, (char*)topic.c_str(), qos);
}



//-------------------------------------------------------------------------------
// set user callback functions
//-------------------------------------------------------------------------------
void MQTT::onConnected( void (*function)(void) )
{
	onMqttConnectedCb = function;
}

void MQTT::onDisconnected( void (*function)(void) )
{
	onMqttDisconnectedCb = function;
}

void MQTT::onPublished( void (*function)(void) )
{
	onMqttPublishedCb = function;
}

void MQTT::onData( void (*function)(String&, String&) )
{
	onMqttDataCb = function;
}

void MQTT::onData( void (*function)(const char*, uint32_t, const char*, uint32_t) )
{
	onMqttDataRawCb = function;
}


// internal callback, calling user CB
void MQTT::_onMqttDataCb(const char* topic, uint32_t topic_len, const char* buf, uint32_t buf_len)
{
	char* topicCpy = (char*)malloc(topic_len+1);
	memcpy(topicCpy, topic, topic_len);
	topicCpy[topic_len] = 0;
	// string it
	String topicStr(topicCpy);
	free(topicCpy);
	
	//EasyIoT Cloud
	if (topicStr ==  String("/NewModule"))
	{
		char* bufCpy = (char*)malloc(buf_len+1);
		memcpy(bufCpy, buf, buf_len);
		bufCpy[buf_len] = 0;
		// string it
		String bufStr(bufCpy);
			
		_moduleId = (uint16_t)bufStr.toInt();
		stepOk1 = true;
		free(bufCpy);
		return;
	}
	else if (isNewParameterCmd && (topicStr == String("/"+String(_moduleId)+"/"+_parameterName +"/NewParameter") ))
	{
		stepOk1 = true;
		return;
	}
	
	
	if (onMqttDataRawCb) {
		onMqttDataRawCb(topic, topic_len, buf, buf_len);
	}
	
	if (onMqttDataCb) {
		
		char* topicCpy = (char*)malloc(topic_len+1);
		memcpy(topicCpy, topic, topic_len);
		topicCpy[topic_len] = 0;
		// string it
		String topicStr(topicCpy);
		
		char* bufCpy = (char*)malloc(buf_len+1);
		memcpy(bufCpy, buf, buf_len);
		bufCpy[buf_len] = 0;
		// string it
		String bufStr(bufCpy);
		
		onMqttDataCb(topicStr, bufStr);
		
		free(topicCpy);
		free(bufCpy);
	}
}



void MQTT::waitStepOK()
{
	while(!stepOk1){
		if (onWaitOkCb)
			onWaitOkCb();
		delay(100);
	}
		
}

uint16_t MQTT::NewModule()
{
	_moduleId = 0;
	stepOk1 = false;
	subscribe("/NewModule");	
	waitStepOK();
	return _moduleId;
}

bool MQTT::SetModuleType(uint16_t moduleId, const char* moduleType)	
{
	String valueStr1("");
	valueStr1 = moduleType;
	_topic  = "/" + String(moduleId) + "/ModuleType";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}

bool MQTT::SetParameterIsCommand(uint16_t moduleId, const char* parameterName, bool isCommand)
{
	String valueStr1("");
	valueStr1 = isCommand?"true":"false";
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/IsCommand";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}

bool MQTT::SetParameterDescription(uint16_t moduleId, const char* parameterName, const char* description)
{
	String valueStr1("");
	valueStr1 = description;
	_topic  = "/" + String(moduleId) +"/" + parameterName + "/Description";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}


bool MQTT::SetParameterUnit(uint16_t moduleId, const char* parameterName, const char* unit)
{
	String valueStr1("");
	valueStr1 = unit;
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/Unit";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}


bool MQTT::SetParameterDBLogging(uint16_t moduleId, const char* parameterName, bool dbLogging)
{
	String valueStr1("");
	valueStr1 = dbLogging?"true":"false";
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/DBLogging";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}

bool MQTT::SetParameterChartSteps(uint16_t moduleId, const char* parameterName, bool chartSteps)
{
	String valueStr1("");
	valueStr1 = chartSteps?"true":"false";
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/ChartSteps";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}

bool MQTT::SetParameterUINotifications(uint16_t moduleId, const char* parameterName, bool uiNotifications)
{
	String valueStr1("");
	valueStr1 = uiNotifications?"true":"false";
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/UINotifications";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}


bool MQTT::SetParameterDbAvgInterval(uint16_t moduleId, const char* parameterName, uint dbAvgInterval)
{
	String valueStr1("");
	valueStr1 = String(dbAvgInterval);
	_topic  = "/" + String(moduleId) + "/" + parameterName + "/DbAvgInterval";
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}


bool MQTT::SetParameterValue(uint16_t moduleId, const char* parameterName, const char* value)
{
	String valueStr1("");
	valueStr1 = value;
	_topic  = "/" + String(moduleId) +"/" + parameterName;
    bool result = publish(_topic, valueStr1);
    delay(100);
	return  result;
}


/*
	add new parameter to EasyIoT Cloud
	param:
	-module id
	-parameter name
*/
void MQTT::NewModuleParameter(uint16_t moduleId, const char* parameterName)
{
	_parameterName = String(parameterName);
	_moduleId = moduleId;
	stepOk1 = false;
	isNewParameterCmd = true;
	subscribe("/"+String(moduleId)+ "/"+ parameterName + "/NewParameter");

	waitStepOK();
	_parameterName = String("");
	isNewParameterCmd = false;
}

void MQTT::onWaitOk( void (*function)(void) )
{
	onWaitOkCb = function;
}










