#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif


#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <EEPROM.h>

#include <OneWire.h>
#include <DallasTemperature.h>





#define AP_SSID           "xxx"
#define AP_PASSWORD       "xxx"  

#define EIOTCLOUD_USERNAME "xxx"
#define EIOTCLOUD_PASSWORD "xxx"



#define TEMPSENDINTERVAL     30 // s

#define TEMP_HISTERESE  0.25

#define CONFIG_START 0
#define CONFIG_VERSION "v02"

#define AP_CONNECT_TIME 30 //s


#define EIOT_CLOUD_ADDRESS "cloud.iot-playground.com"

MQTT *pmyMqtt = NULL;

#define PARAM_SET_TEMP "Sensor.Parameter1"
#define PARAM_SWITCH   "Sensor.Parameter2"
#define PARAM_FAN  "Sensor.Parameter3"
#define PARAM_ACT_TEMP "Sensor.Parameter4"



#define ONE_WIRE_BUS_PIN  D2
#define RELAY_PIN         D1
#define CFG_BUTTON_PIN    D3
#define WIFI_STATUS_PIN   D0 // LED_BUILTIN
#define LED_PIN           D5 // 
#define MANUAL_BUTTON_PIN D6 // 



float actTemp; // actual temperature
bool fanOld;
bool fan;

float setTempNew;
bool switchState;

int milCnt;
int secCnt;


OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);


WiFiServer server(80);


struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  uint moduleId;  // module id
  bool state;     // state

  uint setTemp;  // set temp
  //uint fanMode;  // fanMode
  //uint acMode;  // ac mode
  
  char ssid[20];
  char pwd[20];
  char clUser[20];
  char clPwd[20];
} storage = {
  CONFIG_VERSION,
  0, // The default module 0
  0, // off
  22, // 22 C
  //2, // fan 2
  //0, // ac mode - cool
  AP_SSID,
  AP_PASSWORD,
  EIOTCLOUD_USERNAME,
  EIOTCLOUD_PASSWORD
};

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
  Serial.println("Setup mode");
  WiFi.mode(WIFI_AP);

  String clientName;
  clientName += "Device-";
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

  int i =0;
  while(inf_loop){
    while (!client){
      Serial.print(".");
      delay(100);
      client = server.available();

      if (i++ > 1 * 60 * 10 * 5) // 5 min
      {
        Serial.println("Exit AP loop");
        return;
      }
      Serial.println(i);
    }
    String ssid = "";
    String passwd = "";

    String clUser = "";
    String clPwd = "";
    bool moduleReset = false;
  
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n";
    s += "Content-Type: text/html\r\n\r\n";
    s += "<!DOCTYPE HTML>\r\n<html>\r\n";

    if (req.indexOf("&") != -1){
      int ptr1 = req.indexOf("ssid=", 0);
      int ptr2 = req.indexOf("password=", ptr1);
      int ptr3 = req.indexOf("cluser=", ptr2);
      int ptr4 = req.indexOf("clpwd=", ptr3);    
      int ptr5 = req.indexOf(" HTTP/",ptr4);

      int ptr6 = req.indexOf("modReset=", ptr4);    
         
      ssid = req.substring(ptr1+5, ptr2-1);
      passwd = req.substring(ptr2+9, ptr3-1);  
      clUser = req.substring(ptr3+7, ptr4-1);

      if (ptr6 != -1)
      {
        moduleReset = true;
        clPwd = req.substring(ptr4+6, ptr6-1);
      }
      else
        clPwd = req.substring(ptr4+6, ptr5);
      
      val = -1;
    }

    if (val == -1){
      strcpy(storage.ssid, ssid.c_str());
      strcpy(storage.pwd, passwd.c_str());

      strcpy(storage.clUser, clUser.c_str());
      strcpy(storage.clPwd, clPwd.c_str());

      if (moduleReset)
        storage.moduleId = 0;
        
      saveConfig();

      s += "Setting OK";
      s += "<br>"; // Go to the next line.

//      s += ssid + "<br>";
//      s += passwd + "<br>";
//      s += clUser + "<br>";
//      s += clPwd + "<br>";
      
      s += "Continue / reboot";
      inf_loop = false;
    }

    else{
      String content="";
      
content += "<!DOCTYPE html>";
content += "<html>";
content += "<body>";
content += "<style>";

content += "body {";
content += "zoom: 300%;";
content += "}";


content += "input[type=text], input[type=password], select {";
content += "width: 100%;";
content += "padding: 12px 20px;";
content += "margin: 8px 0;";
content += "display: inline-block;";
content += "border: 1px solid #ccc;";
content += "border-radius: 4px;";
content += "box-sizing: border-box;";
content += "}";
content += "input[type=submit] {";
content += "width: 100%;";
content += "background-color: #4CAF50;";
content += "color: white;";
content += "padding: 14px 20px;";
content += "margin: 8px 0;";
content += "border: none;";
content += "border-radius: 4px;";
content += "cursor: pointer;";
content += "}";
content += "input[type=submit]:hover {";
content += "background-color: #45a049;";
content += "}";
content += "div {";
content += "border-radius: 5px;";
content += "background-color: #f2f2f2;";
content += "padding: 20px;";
content += "}";
content += "</style>";
content += "<div>";
content += "<form method=get>";
content += "<b>AP</b><br>";
content += "SSID:<br>";
content += "<input type='text' name='ssid' maxlength='19' value='"+ String(storage.ssid) +"'>";
content += "<br>";
content += "PASSWORD:<br>";
content += "<input type='password' name='password' maxlength='19' value='"+ String(storage.pwd) +"'>";
  
content += "<br><br>";
content += "<b>Cloud</b><br>";
content += "Username:<br>";
content += "<input type='text' name='cluser' maxlength='19' value='"+ String(storage.clUser) +"'>";
content += "<br>";
content += "PASSWORD:<br>";
content += "<input type='password' name='clpwd' maxlength='19' value='"+ String(storage.clPwd) +"'>";
content += "<br><br>";
content += "<input type='checkbox' name='modReset' >Reset module Id<br><br>";
content += "<input type='submit' value='Submit'>";
content += "</form>"; 
content += "</div>";
content += "</body>";
content += "</html>";

      s += content;
    }
    
    s += "</html>\n";
    // Send the response to the client
    client.print(s);
    delay(1);
    client.stop();
  }
}


void setupMode()
{
    Serial.println("Enter SETUP mode.");

    digitalWrite(WIFI_STATUS_PIN, HIGH);  // Turn the LED off by making the voltage HIGH
    AP_Setup();
    AP_Loop();
    ESP.reset();
 
}

void checkConfigButton()
{
  if (digitalRead(CFG_BUTTON_PIN) == 0)
    setupMode();  
}



void myConnectedCb() {
#ifdef DEBUG
  Serial.println("Connected to MQTT server");
#endif
  if (storage.moduleId != 0)
  {
    pmyMqtt->subscribe("/" + String(storage.moduleId) + "/Sensor.Parameter1");
    pmyMqtt->subscribe("/" + String(storage.moduleId) + "/Sensor.Parameter2");
  }
}

void myDisconnectedCb() {
  DPRINTLN("disconnected. try to reconnect...");
  delay(500);
  pmyMqtt->connect();
}


void myPublishedCb() {
  DPRINTLN("published.");
}

void myTimeoutCb() {
  DPRINTLN("MQTT timeout.");
}

void myDataCb(String& topic, String& data) {  
  DPRINT("Received topic:");
  DPRINTLN(topic);
  DPRINT(": ");
  DPRINTLN(data);

  if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter1"))
  {
    setTempNew =  data.toFloat();
    DPRINTLN("New set temp received: " + String(setTempNew));
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter2"))
  {
    switchState = (data == String("1"))? true: false;
    DPRINTLN("switch state received: " + String(switchState));
  }
}

void setupMQTT()
{
  pmyMqtt = new MQTT("", EIOT_CLOUD_ADDRESS, 1883);

  String clientName;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  pmyMqtt->setClientId((char*) clientName.c_str());

  DPRINT("MQTT client id:");
  DPRINTLN(clientName);
  // setup callbacks
  pmyMqtt->onConnected(myConnectedCb);
  pmyMqtt->onDisconnected(myDisconnectedCb);
  pmyMqtt->onPublished(myPublishedCb);
  pmyMqtt->onData(myDataCb);

  pmyMqtt->onWaitOk(checkConfigButton);

  DPRINTLN("connect mqtt...");
  
  pmyMqtt->setUserPwd(storage.clUser, storage.clPwd);  
  pmyMqtt->connect();

  delay(500);
}

void subscribeMQTT()
{
  DPRINTLN("Suscribe: /"+String(storage.moduleId)+ "/Sensor.Parameter1"); 
  pmyMqtt->subscribe("/"+String(storage.moduleId)+ "/Sensor.Parameter1");
  DPRINTLN("Suscribe: /"+String(storage.moduleId)+ "/Sensor.Parameter2"); 
  pmyMqtt->subscribe("/" + String(storage.moduleId) + "/Sensor.Parameter2");
}


void setup() {
#ifdef DEBUG    //Macros are usually in all capital letters.
  Serial.begin(115200);  
  delay(1000);
#endif
  DPRINTLN("");

  DPRINTLN("D0: " + String(D0));
  DPRINTLN("D1: " + String(D1));
  DPRINTLN("D2: " + String(D2));
  DPRINTLN("D3: " + String(D3));
  DPRINTLN("D4: " + String(D4));
  DPRINTLN("D5: " + String(D5));
  DPRINTLN("D6: " + String(D6));
  DPRINTLN("D7: " + String(D7));

  DS18B20.begin();
  
  pinMode(RELAY_PIN, OUTPUT);     // relay pin
  digitalWrite(RELAY_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);     // relay pin
  digitalWrite(LED_PIN, LOW);

  // config pin
  pinMode(CFG_BUTTON_PIN, INPUT);
  digitalWrite(CFG_BUTTON_PIN, HIGH);

  pinMode(WIFI_STATUS_PIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(WIFI_STATUS_PIN, HIGH);  // Turn the LED off by making the voltage HIGH

  pinMode(MANUAL_BUTTON_PIN, INPUT_PULLUP);
  //digitalWrite(MANUAL_BUTTON_PIN, HIGH);

  EEPROM.begin(512);
  loadConfig();  

  setTempNew = storage.setTemp;
  switchState = storage.state;


  // WIFI
  WiFi.setAutoConnect ( true );                                             // Autoconnect to last known Wifi on startup
  WiFi.setAutoReconnect ( true );

  WiFi.mode(WIFI_STA);
  WiFi.begin(storage.ssid, storage.pwd);


  int i = 0;

  DPRINT("Connecting to AP");
  while (WiFi.status() != WL_CONNECTED && i++ < (AP_CONNECT_TIME*2) ) {
    delay(500);
    DPRINT(".");
  }

  DPRINTLN("");
  if (!(i < (AP_CONNECT_TIME*2)))
    setupMode();


  DPRINTLN("WiFi connected");
  DPRINT("IP address: ");
  DPRINTLN(WiFi.localIP());

  DPRINT("ModuleId: ");
  DPRINTLN(storage.moduleId);

  DPRINTLN("Connecting to MQTT server");  
  setupMQTT();

  if (storage.moduleId == 0)
  {
    //create module
    DPRINTLN("create new module");
    storage.moduleId = pmyMqtt->NewModule();

    // create parameter
    DPRINTLN("create  parameter: "+String(PARAM_SET_TEMP));    
    pmyMqtt->NewModuleParameter(storage.moduleId, PARAM_SET_TEMP);

    // set IsCommand    
    DPRINTLN("set IsCommand parameter: "+String(PARAM_SET_TEMP));    
    pmyMqtt->SetParameterIsCommand(storage.moduleId, PARAM_SET_TEMP, true); 

    // set Description
    DPRINTLN("set description: "+String(PARAM_SET_TEMP));    
    pmyMqtt->SetParameterDescription(storage.moduleId, PARAM_SET_TEMP, "Set.temp.");

    // set Unit
    DPRINTLN("set Unit: "+String(PARAM_SET_TEMP));    
    pmyMqtt->SetParameterUnit(storage.moduleId, PARAM_SET_TEMP, "°C");


    // create parameter
    DPRINTLN("create  parameter: " + String(PARAM_SWITCH));    
    pmyMqtt->NewModuleParameter(storage.moduleId, PARAM_SWITCH);

    // set IsCommand    
    DPRINTLN("set IsCommand parameter: " + String(PARAM_SWITCH));    
    pmyMqtt->SetParameterIsCommand(storage.moduleId, PARAM_SWITCH, true); 


    // create parameter
    DPRINTLN("create  parameter: " + String(PARAM_FAN));    
    pmyMqtt->NewModuleParameter(storage.moduleId, PARAM_FAN);

    // set Description
    DPRINTLN("set description: "+String(PARAM_FAN));    
    pmyMqtt->SetParameterDescription(storage.moduleId, PARAM_FAN, "Fan");

    // set ParameterDBLogging    
//    DPRINTLN("set ParameterDBLogging parameter: " + String(PARAM_FAN));    
//    pmyMqtt->SetParameterDBLogging(storage.moduleId, PARAM_FAN, true); 

    // set ParameterChartSteps  
    DPRINTLN("set ParameterChartSteps parameter: " + String(PARAM_FAN));    
    pmyMqtt->SetParameterChartSteps(storage.moduleId, PARAM_FAN, true); 



    // create parameter
    DPRINTLN("create  parameter: "+String(PARAM_ACT_TEMP));    
    pmyMqtt->NewModuleParameter(storage.moduleId, PARAM_ACT_TEMP);

    // set Description
    DPRINTLN("set description: "+String(PARAM_ACT_TEMP));    
    pmyMqtt->SetParameterDescription(storage.moduleId, PARAM_ACT_TEMP, "Act.temp.");

    // set Unit
    DPRINTLN("set Unit: "+String(PARAM_ACT_TEMP));    
    pmyMqtt->SetParameterUnit(storage.moduleId, PARAM_ACT_TEMP, "°C");


    // set ParameterDBLogging    
    DPRINTLN("set ParameterDBLogging parameter: " + String(PARAM_ACT_TEMP));    
    pmyMqtt->SetParameterDBLogging(storage.moduleId, PARAM_ACT_TEMP, true); 




    DPRINTLN("create  parameter: Settings.ShowAdditionalParameters");    
    pmyMqtt->NewModuleParameter(storage.moduleId, "Settings.ShowAdditionalParameters");
    pmyMqtt->SetParameterValue(storage.moduleId, "Settings.ShowAdditionalParameters", "1");

    DPRINTLN("create  parameter: Settings.SliderMinValue");    
    pmyMqtt->NewModuleParameter(storage.moduleId, "Settings.SliderMinValue");
    pmyMqtt->SetParameterValue(storage.moduleId, "Settings.SliderMinValue", "25");

    DPRINTLN("create  parameter: Settings.SliderMaxValue");    
    pmyMqtt->NewModuleParameter(storage.moduleId, "Settings.SliderMaxValue");
    pmyMqtt->SetParameterValue(storage.moduleId, "Settings.SliderMaxValue", "35");


    // set module type        
    DPRINTLN("Set module type");    
    pmyMqtt->SetModuleType(storage.moduleId, "MT_DIMMER");

    // save new module id
    saveConfig();      
  }

  do {
    DS18B20.requestTemperatures(); 
    actTemp = DS18B20.getTempCByIndex(0);
  } while (actTemp == 85.0 || actTemp == (-127.0));  

  DPRINTLN("Temperature: " + String(actTemp));    
  pmyMqtt->SetParameterValue(storage.moduleId, PARAM_ACT_TEMP, String(actTemp).c_str());


  pmyMqtt->SetParameterValue(storage.moduleId, PARAM_FAN , String(fanOld).c_str());
  pmyMqtt->SetParameterValue(storage.moduleId, PARAM_SWITCH, String(switchState).c_str());
  pmyMqtt->SetParameterValue(storage.moduleId, PARAM_SET_TEMP, String(setTempNew).c_str());
}


void readTemp()
{
  float readTemp;
  do {
    DS18B20.requestTemperatures(); 
    readTemp = DS18B20.getTempCByIndex(0);
  } while (readTemp == 85.0 || readTemp == (-127.0));

  actTemp += (readTemp - actTemp) / 100;
}


void setRelay()
{
  if (fan)
    digitalWrite(RELAY_PIN, HIGH); 
  else
    digitalWrite(RELAY_PIN, LOW); 
}


// the loop function runs over and over again forever
void loop() {

// WIFI check

  if (WiFi.status() == WL_CONNECTED  ) 
  //if (pmyMqtt->isConnected()  ) 
    digitalWrite(WIFI_STATUS_PIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  else
    digitalWrite(WIFI_STATUS_PIN, HIGH);  // Turn the LED off by making the voltage HIGH

  checkConfigButton();


 if (WiFi.status() != WL_CONNECTED) { // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
    DPRINTLN("AP not connected");
    pmyMqtt->onConnected(NULL);
    pmyMqtt->onDisconnected(NULL);
    pmyMqtt->onPublished(NULL);
    
    pmyMqtt->disconnect();
    delay(1000);
    delete pmyMqtt;
    delay(1000);

    WiFi.begin(storage.ssid, storage.pwd);
  
    while (WiFi.status() != WL_CONNECTED) {
      int i =0;
      DPRINTLN("Connecting to AP");
      while (WiFi.status() != WL_CONNECTED && i++ < (AP_CONNECT_TIME*2) ) {
        delay(500);
        DPRINT(".");
      }
    }
     
     setupMQTT();
     delay(2000);
     subscribeMQTT();      
     delay(2000);
  }

  if (!pmyMqtt->isConnected())
  {
    DPRINTLN("Re-connect MQTT");
    pmyMqtt->disconnect();
    delete pmyMqtt;
    pmyMqtt = NULL;

    setupMQTT();
    subscribeMQTT();      
  }


  // COUNTER
  if (milCnt++ >= 100)
  {
    secCnt++;

    readTemp();    
  }


  if (secCnt >= TEMPSENDINTERVAL)
  {
    digitalWrite(WIFI_STATUS_PIN, HIGH); 
    delay(50);
    secCnt = 0;
    DPRINTLN("Temperature: " + String(actTemp));    
    pmyMqtt->SetParameterValue(storage.moduleId, PARAM_ACT_TEMP, String(actTemp).c_str());
  }


  // if manual button pressed
  if (digitalRead(MANUAL_BUTTON_PIN) == 0)
  {
    DPRINTLN("manual btn pressed");
    while(digitalRead(MANUAL_BUTTON_PIN) == 0){
      delay(50);
      //DPRINT(".");
    }
    //DPRINTLN("manual btn pressed OK");
    switchState =! storage.state;
  }

  if (switchState != storage.state)
  {
    digitalWrite(WIFI_STATUS_PIN, HIGH); 
    delay(50);

    DPRINTLN("publish " + String(PARAM_SWITCH));
    pmyMqtt->SetParameterValue(storage.moduleId, PARAM_SWITCH, String(switchState).c_str());

    storage.state = switchState;
    saveConfig();
  }

  if (setTempNew != storage.setTemp)
  {       
    digitalWrite(WIFI_STATUS_PIN, HIGH); 
    delay(50);

    DPRINTLN("publish " + String(PARAM_SET_TEMP));
    pmyMqtt->SetParameterValue(storage.moduleId, PARAM_SET_TEMP, String(setTempNew).c_str());

    storage.setTemp = setTempNew;
    saveConfig();      
  }

  if (switchState)
  {
    digitalWrite(LED_PIN, HIGH);
    if (actTemp <= (setTempNew - TEMP_HISTERESE) && fan)
      fan = false;
    else if (actTemp >= (setTempNew + TEMP_HISTERESE) && !fan)
      fan = true;

    if (fan != fanOld)
    {
      DPRINTLN("Temperature: " + String(actTemp));    
      pmyMqtt->SetParameterValue(storage.moduleId, PARAM_ACT_TEMP, String(actTemp).c_str());

      fanOld = fan;
      pmyMqtt->SetParameterValue(storage.moduleId, PARAM_FAN , String(fan).c_str());  
      DPRINTLN("fan " + String(fan));      
    }
      
    setRelay();
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    fan = false;    

    if (fan != fanOld)
    {
      fanOld = fan;
      pmyMqtt->SetParameterValue(storage.moduleId, PARAM_FAN , String(fan).c_str());  
      DPRINTLN("fan " + String(fan));
    }
    setRelay();    
  }

  delay(10); 
}
