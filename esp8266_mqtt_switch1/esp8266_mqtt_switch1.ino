#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <EEPROM.h>

#define DEBUG

#define AP_SSID     "Geek"
#define AP_PASSWORD "G33k"  


#define EIOTCLOUD_USERNAME "username"
#define EIOTCLOUD_PASSWORD "passwd"


// create MQTT object
#define EIOT_CLOUD_ADDRESS "cloud.iot-playground.com"
MQTT myMqtt("", EIOT_CLOUD_ADDRESS, 1883);

#define CONFIG_START 0
#define CONFIG_VERSION "v01"

#define AP_CONNECT_TIME 10 //s

char ap_ssid[16];
char ap_pass[16];

WiFiServer server(80);


struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  uint moduleId;  // module id
  bool state;     // state
  char ssid[20];
  char pwd[20];
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
  0, // off
  AP_SSID,
  AP_PASSWORD
};

bool stepOk = false;
int buttonState;

boolean result;
String topic("");
String valueStr("");

boolean switchState;
const int buttonPin = 0;  
const int outPin = 2;  
int lastButtonState = LOW; 
int cnt = 0;

void setup() 
{  
#ifdef DEBUG
  Serial.begin(115200);
#endif
  delay(1000);

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  
  EEPROM.begin(512);

  switchState = false;  

  loadConfig();
  
  pinMode(BUILTIN_LED, OUTPUT); 
  digitalWrite(BUILTIN_LED, !switchState); 
#ifndef DEBUG
  pinMode(outPin, OUTPUT); 
  digitalWrite(outPin, switchState);
#endif  


#ifdef DEBUG
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(AP_SSID);
#endif  
  WiFi.begin(storage.ssid, storage.pwd);

  int i = 0;
  
  while (WiFi.status() != WL_CONNECTED && i++ < (AP_CONNECT_TIME*2) ) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }

  if (!(i < (AP_CONNECT_TIME*2)))
  {
    AP_Setup();
    AP_Loop();
    ESP.reset();
  }

#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to MQTT server");  
#endif
  //set client id
  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  myMqtt.setClientId((char*) clientName.c_str());

#ifdef DEBUG
  Serial.print("MQTT client id:");
  Serial.println(clientName);
#endif
  // setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);
  
  //////Serial.println("connect mqtt...");
  myMqtt.setUserPwd(EIOTCLOUD_USERNAME, EIOTCLOUD_PASSWORD);  
  myMqtt.connect();

  delay(500);

#ifdef DEBUG
  Serial.print("ModuleId: ");
  Serial.println(storage.moduleId);
#endif

  //create module if necessary 
  if (storage.moduleId == 0)
  {
    //create module
#ifdef DEBUG
    Serial.println("create module: /NewModule");
#endif
    myMqtt.subscribe("/NewModule");
    waitOk();
      
    // create Sensor.Parameter1
#ifdef DEBUG    
    Serial.println("/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter");    
#endif
    myMqtt.subscribe("/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter");
    waitOk();

    // set IsCommand
#ifdef DEBUG    
    Serial.println("/"+String(storage.moduleId)+ "/Sensor.Parameter1/IsCommand");    
#endif
    valueStr = "true";
    topic  = "/"+String(storage.moduleId)+ "/Sensor.Parameter1/IsCommand";
    result = myMqtt.publish(topic, valueStr);
    delay(100);

    // set module type
#ifdef DEBUG        
    Serial.println("Set module type");    
#endif
    valueStr = "MT_DIGITAL_OUTPUT";
    topic  = "/" + String(storage.moduleId) + "/ModuleType";
    result = myMqtt.publish(topic, valueStr);
    delay(100);

    // save new module id
    saveConfig();
  }

  //switchState = storage.state;
  //storage.state = !storage.state;

  storage.state = switchState;

  Serial.println("Suscribe: /"+String(storage.moduleId)+ "/Sensor.Parameter1"); 
  myMqtt.subscribe("/"+String(storage.moduleId)+ "/Sensor.Parameter1");
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG        
    Serial.print(".");
#endif
  }


  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    if (reading == LOW)
    {
      switchState = !switchState;            
#ifdef DEBUG
      Serial.println("button pressed");
#endif
    }

#ifdef DEBUG
      Serial.println("set state: ");
      Serial.println(valueStr);
#endif

    lastButtonState = reading;
  }
  
  digitalWrite(BUILTIN_LED, !switchState); 
#ifndef DEBUG  
  digitalWrite(outPin, switchState); 
#endif
  if (switchState != storage.state)
  {
    storage.state = switchState;
    // save button state
    saveConfig();

    valueStr = String(switchState);
    topic  = "/"+String(storage.moduleId)+ "/Sensor.Parameter1";
    result = myMqtt.publish(topic, valueStr, 0, 1);    
#ifdef DEBUG
      Serial.print("Publish ");
      Serial.print(topic);
      Serial.print(" ");
      Serial.println(valueStr);
#endif  
    delay(200);
  }  
}

void waitOk()
{
  while(!stepOk)
    delay(100);
 
  stepOk = false;
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


/*
 * 
 */
void myConnectedCb() {
#ifdef DEBUG
  Serial.println("connected to MQTT server");
#endif
  if (storage.moduleId != 0)
    myMqtt.subscribe("/" + String(storage.moduleId) + "/Sensor.Parameter1");
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
  Serial.print("Received topic:");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);
#endif
  if (topic ==  String("/NewModule"))
  {
    storage.moduleId = data.toInt();
    stepOk = true;
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter"))
  {
    stepOk = true;
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter1"))
  {
    switchState = (data == String("1"))? true: false;
#ifdef DEBUG      
    Serial.print("switch state received: ");
    Serial.println(switchState);
#endif
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



void AP_Setup(void){
  Serial.println("setting mode");
  WiFi.mode(WIFI_AP);

  String clientName;
  clientName += "Thing-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  
  Serial.println("starting ap");
  WiFi.softAP((char*) clientName.c_str(), "");
  Serial.println("running server");
  server.begin();
}

void AP_Loop(void){

  bool  inf_loop = true;
  int  val = 0;
  WiFiClient client;

  Serial.println("AP loop");

  while(inf_loop){
    while (!client){
      Serial.print(".");
      delay(100);
      client = server.available();
    }
    String ssid;
    String passwd;
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n";
    s += "Content-Type: text/html\r\n\r\n";
    s += "<!DOCTYPE HTML>\r\n<html>\r\n";

    if (req.indexOf("&") != -1){
      int ptr1 = req.indexOf("ssid=", 0);
      int ptr2 = req.indexOf("&", ptr1);
      int ptr3 = req.indexOf(" HTTP/",ptr2);
      ssid = req.substring(ptr1+5, ptr2);
      passwd = req.substring(ptr2+10, ptr3);    
      val = -1;
    }

    if (val == -1){
      strcpy(storage.ssid, ssid.c_str());
      strcpy(storage.pwd, passwd.c_str());
      
      saveConfig();
      //storeAPinfo(ssid, passwd);
      s += "Setting OK";
      s += "<br>"; // Go to the next line.
      s += "Continue / reboot";
      inf_loop = false;
    }

    else{
      String content="";
      // output the value of each analog input pin
      content += "<form method=get>";
      content += "<label>SSID</label><br>";
      content += "<input  type='text' name='ssid' maxlength='19' size='15' value='"+ String(storage.ssid) +"'><br>";
      content += "<label>Password</label><br>";
      content += "<input  type='password' name='password' maxlength='19' size='15' value='"+ String(storage.pwd) +"'><br><br>";
      content += "<input  type='submit' value='Submit' >";
      content += "</form>";
      s += content;
    }
    
    s += "</html>\n";
    // Send the response to the client
    client.print(s);
    delay(1);
    client.stop();
  }
}
