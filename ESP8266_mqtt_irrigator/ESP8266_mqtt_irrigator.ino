#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <EEPROM.h>


#define DEBUG


#define AP_SSID     "xxx"
#define AP_PASSWORD "xxx"  

#define EIOTCLOUD_USERNAME "xxx"
#define EIOTCLOUD_PASSWORD "xxx"

// create MQTT object
#define EIOT_CLOUD_ADDRESS "cloud.iot-playground.com"



#define PIN_PUMP         BUILTIN_LED //D0  // nodemcu built in LED
#define PIN_BUTTON       D3  // nodemcu flash button
#define PIN_HUM_ANALOG   A0  // humidity pin


#define MAX_ANALOG_VAL         956
#define MIN_ANALOG_VAL         250

#define IRRIGATION_TIME        10 // irrigation time in sec
#define IRRIGATION_PAUSE_TIME  300 // irrigation pause time in sec - only for auto irrigator


// irrigator state
typedef enum {
  s_idle             = 0,  // irrigation idle
  s_irrigation_start = 1,  // start irrigation
  s_irrigation       = 2,  // irrigate
  s_irrigation_stop  = 3,  // irrigation stop
} e_state;


#define CONFIG_START 0
#define CONFIG_VERSION "v01"

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  uint moduleId;  // module id
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
};






#define PARAM_HUMIDITY_TRESHOLD   "Sensor.Parameter1"
#define PARAM_MANUAL_AUTO_MODE    "Sensor.Parameter2"
#define PARAM_PUMP_ON             "Sensor.Parameter3"
#define PARAM_HUMIDITY            "Sensor.Parameter4"


#define MS_IN_SEC  1000 // 1S  


MQTT myMqtt("", EIOT_CLOUD_ADDRESS, 1883);

// intvariables
int state;
bool stepOk = false;
int soilHumidityThreshold;
bool autoMode;
String valueStr("");
String topic("");
boolean result;
int lastAnalogReading;
bool autoModeOld;
int soilHumidityThresholdOld;
unsigned long startTime;
int soilHum;
int irrigatorCounter;



void setup() {
  state = s_idle;
  pinMode(PIN_PUMP, OUTPUT); 
  pinMode(PIN_BUTTON, INPUT);

  autoMode = false;
  stepOk = false;
  soilHumidityThresholdOld = -1;
  startTime = millis();
  soilHum = -1;
  
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);  
  WiFi.begin(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(AP_SSID);
    
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  };

  Serial.println("WiFi connected");
  Serial.println("Connecting to MQTT server");  


  EEPROM.begin(512);
  loadConfig();


  //set client id
  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  //clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  myMqtt.setClientId((char*) clientName.c_str());

  Serial.print("MQTT client id:");
  Serial.println(clientName);

  // setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);
  
  //////Serial.println("connect mqtt...");
  myMqtt.setUserPwd(EIOTCLOUD_USERNAME, EIOTCLOUD_PASSWORD);  
  myMqtt.connect();

  delay(500);
  
  Serial.print("ModuleId: ");
  Serial.println(storage.moduleId);


  //create module if necessary 
  if (storage.moduleId == 0)
  {
    //create module
    Serial.println("create module: /NewModule");
    storage.moduleId = myMqtt.NewModule();

    if (storage.moduleId == 0)
    {
      Serial.println("Module NOT created. Check module limit");
      while(1)
        delay(100);
    }

   // set module type
    Serial.println("Set module type");    
    myMqtt.SetModuleType(storage.moduleId, "ZMT_IRRIGATOR");

    // create Sensor.Parameter1 - humidity treshold value
    Serial.println("new parameter: /"+String(storage.moduleId)+ "/" +PARAM_HUMIDITY_TRESHOLD);    
    myMqtt.NewModuleParameter(storage.moduleId, PARAM_HUMIDITY_TRESHOLD);

    // set IsCommand
    Serial.println("set isCommand: /"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY_TRESHOLD);    
    myMqtt.SetParameterIsCommand(storage.moduleId, PARAM_HUMIDITY_TRESHOLD, true);


    // create Sensor.Parameter2
    // Sensor.Parameter2 - manual/auto mode 0 - manual, 1 - auto mode
    Serial.println("new parameter: /"+String(storage.moduleId)+ "/" + PARAM_MANUAL_AUTO_MODE);    
    myMqtt.NewModuleParameter(storage.moduleId, PARAM_MANUAL_AUTO_MODE);

    // set IsCommand
    Serial.println("set isCommand: /"+String(storage.moduleId)+ "/" + PARAM_MANUAL_AUTO_MODE);    
    myMqtt.SetParameterIsCommand(storage.moduleId, PARAM_MANUAL_AUTO_MODE, true);

    
    // create Sensor.Parameter3
    // Sensor.Parameter3 - pump on/ pump off
    Serial.println("new parameter: /"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON);    
    myMqtt.NewModuleParameter(storage.moduleId, PARAM_PUMP_ON);


    // set IsCommand
    Serial.println("set isCommand: /"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON);    
    myMqtt.SetParameterIsCommand(storage.moduleId, PARAM_PUMP_ON, true);


    // create Sensor.Parameter4
    // Sensor.Parameter4 - current soil humidity
    Serial.println("new parameter: /"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY);    
    myMqtt.NewModuleParameter(storage.moduleId, PARAM_HUMIDITY);


    // set Description
    Serial.println("set description: /"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY);    
    myMqtt.SetParameterDescription(storage.moduleId, PARAM_HUMIDITY, "Soil moist.");

    // set Unit
    Serial.println("set Unit: /"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY);    
    myMqtt.SetParameterUnit(storage.moduleId, PARAM_HUMIDITY, "%");

    // set dbLogging
    Serial.println("set Unit: /"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY);    
    myMqtt.SetParameterDBLogging(storage.moduleId, PARAM_HUMIDITY, true);

    // save new module id
    saveConfig();
  }

  subscribe();
  
  lastAnalogReading = analogRead(PIN_HUM_ANALOG); 

  autoModeOld = !autoMode;
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG        
    Serial.print(".");
#endif
  }

  int in = digitalRead(PIN_BUTTON);

  //Serial.println(in);
  if (in == 0)
  {
    while(digitalRead(PIN_BUTTON) == 0)
      delay(100);

    if (state == s_idle || state == s_irrigation_start)
      state = s_irrigation_start;
    else
      state = s_irrigation_stop;
  }
  


  // post auto mode changes
  if (autoModeOld != autoMode)
  {
    autoModeOld = autoMode;

    if (autoMode)    
      valueStr = String("1");
    else
      valueStr = String("0");
    
    topic  = "/"+String(storage.moduleId)+ "/" + PARAM_MANUAL_AUTO_MODE;
    result = myMqtt.publish(topic, valueStr, 0, 1);

    Serial.print("Publish topic: ");
    Serial.print(topic);
    Serial.print(" value: ");
    Serial.println(valueStr);  
  }

  // post treshold changes
  if (soilHumidityThreshold != soilHumidityThresholdOld)
  {
    soilHumidityThresholdOld = soilHumidityThreshold;
    valueStr = String(soilHumidityThreshold);
    
    topic  = "/"+String(storage.moduleId)+ "/"+ PARAM_HUMIDITY_TRESHOLD;
    result = myMqtt.publish(topic, valueStr, 0, 1);

    Serial.print("Publish topic: ");
    Serial.print(topic);
    Serial.print(" value: ");
    Serial.println(valueStr);  
  }
  
  if (IsTimeout())
  {
    startTime = millis();
    // process every second
    int aireading = analogRead(PIN_HUM_ANALOG);

    Serial.print("Analog value: ");
    Serial.print(aireading);
    Serial.print(" ");
    // filter s
    lastAnalogReading += (aireading - lastAnalogReading) / 10;  
    Serial.print(lastAnalogReading); 
   
   // calculate soil humidity in % 
   int newSoilHum = map(lastAnalogReading, MIN_ANALOG_VAL, MAX_ANALOG_VAL, 100, 0);  
   Serial.print(", Soil hum %:");
   Serial.println(newSoilHum); 
        
   // limit to 0-100%
   if (newSoilHum < 0)
      newSoilHum = 0;

    if (newSoilHum > 100)
      newSoilHum = 100;
 
   // report soil humidity if changed
   if (soilHum != newSoilHum)
   {
     soilHum = newSoilHum;
     //esp.send(msgHum.set(soilHum)); 
     
     valueStr = String(soilHum);
     topic  = "/"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY;
     result = myMqtt.publish(topic, valueStr, 0, 1);

     Serial.print("Publish topic: ");
     Serial.print(topic);
     Serial.print(" value: ");
     Serial.println(valueStr);  
   }
   
   
   // irrigator state machine
   switch(state)
   {
     case s_idle:     
       if (irrigatorCounter <= IRRIGATION_PAUSE_TIME)
         irrigatorCounter++;
       
       if (irrigatorCounter >= IRRIGATION_PAUSE_TIME && autoMode)
       {
         if (soilHum <= soilHumidityThreshold)
           state = s_irrigation_start;       
       }         
       break;
     case s_irrigation_start:
       irrigatorCounter = 0;
       digitalWrite(PIN_PUMP, HIGH);
       //esp.send(msgMotorPump.set((uint8_t)1));       
       valueStr = String(1);
       topic  = "/"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON;
       result = myMqtt.publish(topic, valueStr, 0, 1);    

       Serial.print("Publish topic: ");
       Serial.print(topic);
       Serial.print(" value: ");
       Serial.println(valueStr);  
 
       state = s_irrigation;
       break;
     case s_irrigation:
       if (irrigatorCounter++ > IRRIGATION_TIME)
         state = s_irrigation_stop;
       break;
     case s_irrigation_stop:
       irrigatorCounter = 0;
       state = s_idle;
       //esp.send(msgMotorPump.set((uint8_t)0));
       valueStr = String(0);
       topic  = "/"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON;
       result = myMqtt.publish(topic, valueStr, 0, 1);    

       digitalWrite(PIN_PUMP, LOW);
       break;
   }
  }
  
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


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void waitOk()
{
  while(!stepOk)
    delay(100);
 
  stepOk = false;
}

boolean IsTimeout()
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < MS_IN_SEC ) 
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < MS_IN_SEC ) 
      return false;
  }

  return true;
}


void subscribe()
{
  if (storage.moduleId != 0)
  {
    // Sensor.Parameter1 - humidity treshold value
    myMqtt.subscribe("/"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY_TRESHOLD);
  
    // Sensor.Parameter2 - manual/auto mode 0 - manual, 1 - auto mode
    myMqtt.subscribe("/"+String(storage.moduleId)+ "/" + PARAM_MANUAL_AUTO_MODE);
  
    // Sensor.Parameter3 - pump on/ pump off
    myMqtt.subscribe("/"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON);
  }
}


void myConnectedCb() {
#ifdef DEBUG
  Serial.println("connected to MQTT server");
#endif
  subscribe();
}

void myDisconnectedCb() {
#ifdef DEBUG
  Serial.println("disconnected. try to reconnect...");
#endif
  delay(500);
  myMqtt.connect();
}

void myPublishedCb() {
#ifdef DEBUG  
  Serial.println("published.");
#endif
}

void myDataCb(String& topic, String& data) {  
#ifdef DEBUG  
  Serial.print("Receive topic: ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);
#endif
  if (topic == String("/"+String(storage.moduleId)+ "/" + PARAM_HUMIDITY_TRESHOLD))
  {
    soilHumidityThreshold = data.toInt();
    Serial.println("soilHumidityThreshold");
    Serial.println(data);
  }

  else if (topic == String("/"+String(storage.moduleId)+ "/" + PARAM_MANUAL_AUTO_MODE))
  {
    autoMode = (data == String("1"));
    Serial.println("Auto mode");
    Serial.println(data);
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/" + PARAM_PUMP_ON))
  {
    //switchState = (data == String("1"))? true: false;
    if (data == String("1"))
      state = s_irrigation_start;
    else
      state = s_irrigation_stop;
    Serial.println("Pump");
    Serial.println(data);
  }
}
