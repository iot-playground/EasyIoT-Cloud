//#define DEBUG

#include <ESP8266WiFi.h>
#ifdef DEBUG
#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>
#else
#include <EasyTransfer.h>
#endif
#include <MQTT.h>


#define DATA_LEN 55

#define AP_CONNECT_TIME 30 //s

#ifdef DEBUG
SoftwareSerial mySerial(D2, D3); // RX, TX
SoftEasyTransfer ETin;
SoftEasyTransfer ETout;  
#else
EasyTransfer ETin;
EasyTransfer ETout;  
#endif


unsigned long startTime;

typedef enum {
  CMD_NONE,
  CMD_RESET,
  CMD_INIT_FINISHED,
  CMD_CONNECT_TO_AP,
  CMD_AP_CONNECTED,
  CMD_AP_CONN_TIMEOUT,
  CMD_SET_AP_NAME,
  CMD_SET_AP_PASSWORD,
  CMD_SET_CLOUD_HOST,
  CMD_SET_CLOUD_USER,
  CMD_SET_CLOUD_PASSWORD,
  CMD_SET_CLOUD_CONNECT,

  CMD_SET_CLOUD_CONNECTED,
  CMD_SET_CLOUD_DISCONNECTED,

  CMD_GET_NEW_MODULE,
  CMD_SET_MODULE_ID,
  CMD_SET_MODULE_TYPE,


  CMD_GET_NEW_PARAMETER,

  CMD_SET_PARAMETER_ISCOMMAND,
  CMD_SET_PARAMETER_DESCRIPTION,
  CMD_SET_PARAMETER_UNIT,
  CMD_SET_PARAMETER_DBLOGGING,
  CMD_SET_PARAMETER_CHARTSTEPS,
  CMD_SET_PARAMETER_UINOTIFICATIONS,
  CMD_SET_PARAMETER_DBAVGINTERVAL,

  CMD_SUBSCRIBE_PARAMETER,
  CMD_SET_PARAMETER_VALUE,
  
  CMD_PING,
  CMD_PONG,
} comad_type;



struct DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int8_t header;
  int8_t command;
  char data[DATA_LEN + 1 ];
} __attribute__((packed));


DATA_STRUCTURE OutgoingData;
DATA_STRUCTURE IncomingData;


char ap_ssid[30];
char ap_pass[30];

char cloud_host[30];
char cloud_user[20];
char cloud_pass[20];

char str1[DATA_LEN+1];
char str2[DATA_LEN+1];

uint16_t moduleId;
String topic;
String valueStr;


MQTT *myMqttPtr;

void onSTAGotIP(WiFiEventStationModeGotIP event) {
#ifdef DEBUG  
  Serial.printf("Got IP: %s\n", event.ip.toString().c_str());
#endif
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
#ifdef DEBUG
  Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
#endif
  OutgoingData.command = CMD_AP_CONN_TIMEOUT;
}


void setup() {

  static WiFiEventHandler e1, e2;


  // debug serial
#ifdef DEBUG  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Start ESP Cloud GW");
#endif
 
  // default host
  strcpy(cloud_host, "cloud.iot-playground.com");

  moduleId = 0;
  myMqttPtr = NULL;

  OutgoingData.header = 0x68;

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
  
  // init serial protocol
#ifdef DEBUG
  mySerial.begin(9600);
  delay(50);
  ETin.begin(details(IncomingData), &mySerial);
  ETout.begin(details(OutgoingData), &mySerial);
#else
  Serial.begin(9600);
  ETin.begin(details(IncomingData), &Serial);
  ETout.begin(details(OutgoingData), &Serial);
#endif  

  delay(100);
  OutgoingData.command = CMD_INIT_FINISHED;
  OutgoingData.header = 0x68;
  ETout.sendData();
  OutgoingData.command = CMD_NONE;    
#ifdef DEBUG
  Serial.println("SEND CMD_INIT_FINISHED");
#endif
}

void loop() {
  int interval;
  
  if (OutgoingData.command != CMD_NONE)
  {
    OutgoingData.header = 0x68;
    ETout.sendData();
    OutgoingData.command = CMD_NONE;    
    delay(100);
#ifdef DEBUG
    Serial.println("Send data");
#endif
  }

  if (IsTimeout(100))
  {
    ResetTimer();
    if(ETin.receiveData())
    {
      int i = 0;
      String clientName = "";
      
      if (IncomingData.header == 0x68)
      {
        IncomingData.data[DATA_LEN] = 0;
        switch(IncomingData.command)
        {        
          case CMD_NONE:
            break;
          case CMD_RESET:
  #ifdef DEBUG          
            Serial.print("REC CMD_RESET");
  #endif          
            delay(50);
            ESP.reset();          
            break;
          case CMD_SET_AP_NAME:
            strcpy(ap_ssid, (char *)IncomingData.data);
  #ifdef DEBUG
            Serial.print("REC CMD_SET_AP_NAME ");
            Serial.println(ap_ssid);
  #endif
            break;
          case CMD_SET_AP_PASSWORD:
            strcpy(ap_pass, (char *)IncomingData.data);
  #ifdef DEBUG          
            Serial.print("CMD_SET_AP_PASSWORD ");
            Serial.println(ap_pass);
  #endif
            break;
          case CMD_CONNECT_TO_AP:
            WiFi.mode(WIFI_STA);
            WiFi.begin(ap_ssid, ap_pass);
  
            i = 0;
  #ifdef DEBUG          
            Serial.println("Connecting to AP");
  #endif          
            while (WiFi.status() != WL_CONNECTED && i++ < (AP_CONNECT_TIME*2) ) {
              delay(500);
  #ifdef DEBUG
              Serial.print(".");
  #endif           
            }
  
            if (WiFi.status() == WL_CONNECTED)
            {
              OutgoingData.command = CMD_AP_CONNECTED;
  #ifdef DEBUG            
              Serial.println("Connected");
  #endif            
            }
            else
            {
              OutgoingData.command = CMD_AP_CONN_TIMEOUT;
  #ifdef DEBUG            
              Serial.println("Timeout");
  #endif            
            }
            break;
          case CMD_SET_CLOUD_HOST:
            strcpy(cloud_host, (char *)IncomingData.data);
  #ifdef DEBUG          
            Serial.print("REC CMD_SET_CLOUD_HOST ");
            Serial.println(cloud_host);
  #endif          
            break;
          case CMD_SET_CLOUD_USER:
            strcpy(cloud_user, (char *)IncomingData.data);
  #ifdef DEBUG          
            Serial.print("REC CMD_SET_CLOUD_USER ");
            Serial.println(cloud_user);
  #endif          
            break;
          case CMD_SET_CLOUD_PASSWORD:
            strcpy(cloud_pass, (char *)IncomingData.data);
  #ifdef DEBUG          
            Serial.print("REC CMD_SET_CLOUD_PASSWORD ");
            Serial.println(cloud_pass);
  #endif          
            break;
         case CMD_GET_NEW_MODULE:
  #ifdef DEBUG          
            Serial.print("REC CMD_GET_NEW_MODULE ");
  #endif          
            moduleId = myMqttPtr->NewModule();     
  #ifdef DEBUG          
            Serial.println(moduleId);     
  #endif          
            OutgoingData.command = CMD_SET_MODULE_ID;
  
            OutgoingData.data[0] = moduleId & 0xFF;
            OutgoingData.data[1] = (moduleId >> 8) & 0xFF;
            break;
        case CMD_SET_MODULE_ID:
  #ifdef DEBUG          
            Serial.print("REC CMD_SET_MODULE_ID ");
  #endif
            moduleId = (IncomingData.data[1] << 8) + (uint8_t)IncomingData.data[0];   
  #ifdef DEBUG          
            Serial.println(moduleId);
  #endif          
            break;          
         case CMD_SET_MODULE_TYPE:
  #ifdef DEBUG       
            Serial.print("CMD_SET_MODULE_TYPE ");
  #endif          
            myMqttPtr->SetModuleType(moduleId, IncomingData.data);     
  #ifdef DEBUG          
            Serial.println(IncomingData.data);     
  #endif          
            break; 
         case CMD_GET_NEW_PARAMETER:
  #ifdef DEBUG          
            Serial.print("REC CMD_GET_NEW_PARAMETER ");          
            Serial.println((char *)IncomingData.data);
  #endif          
            myMqttPtr->NewModuleParameter(moduleId, (char *)IncomingData.data);                         
            break;   
        case CMD_SET_PARAMETER_ISCOMMAND:
  #ifdef DEBUG      
            Serial.print("REC CMD_SET_PARAMETER_ISCOMMAND");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            if (strcmp("true", str2) == 0 || strcmp("True", str2) || strcmp("TRUE", str2))
              myMqttPtr->SetParameterIsCommand(moduleId, str1, true);
            else
              myMqttPtr->SetParameterIsCommand(moduleId, str1, false);
            break;
         case CMD_SET_PARAMETER_UNIT:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_UNIT ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            myMqttPtr->SetParameterUnit(moduleId, str1, str2);
            break;
         case CMD_SET_PARAMETER_DESCRIPTION:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_DESCRIPTION ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            myMqttPtr->SetParameterDescription(moduleId, str1, str2);
            break;
         case CMD_SET_PARAMETER_DBLOGGING:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_DBLOGGING ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            if (strcmp("true", str2) == 0 || strcmp("True", str2) || strcmp("TRUE", str2))
              myMqttPtr->SetParameterDBLogging(moduleId, str1, true);
            else
              myMqttPtr->SetParameterDBLogging(moduleId, str1, false);
            break;
         case CMD_SET_PARAMETER_CHARTSTEPS:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_CHARTSTEPS ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            if (strcmp("true", str2) == 0 || strcmp("True", str2) || strcmp("TRUE", str2))
              myMqttPtr->SetParameterChartSteps(moduleId, str1, true);
            else
              myMqttPtr->SetParameterChartSteps(moduleId, str1, false);
            break;
         case CMD_SET_PARAMETER_UINOTIFICATIONS:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_UINOTIFICATIONS ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            if (strcmp("true", str2) == 0 || strcmp("True", str2) || strcmp("TRUE", str2))
              myMqttPtr->SetParameterUINotifications(moduleId, str1, true);
            else
              myMqttPtr->SetParameterUINotifications(moduleId, str1, false);
            break;
         case CMD_SET_PARAMETER_DBAVGINTERVAL:
  #ifdef DEBUG       
            Serial.print("REC CMD_SET_PARAMETER_DBAVGINTERVAL ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            interval = atoi(str2);
            myMqttPtr->SetParameterDbAvgInterval(moduleId, str1, interval);
            break;
          case CMD_SUBSCRIBE_PARAMETER:
  #ifdef DEBUG        
            Serial.print("CMD_SUBSCRIBE_PARAMETER ");
  #endif          
            myMqttPtr->subscribe("/"+String(moduleId)+ "/" +IncomingData.data);          
  #ifdef DEBUG          
            Serial.println(IncomingData.data);             
  #endif          
            break;          
          case CMD_SET_PARAMETER_VALUE:
  #ifdef DEBUG        
            Serial.print("REC CMD_SET_PARAMETER_VALUE ");
  #endif          
            parseStrings((char *)IncomingData.data, str1, str2);
            topic  = "/"+String(moduleId)+ "/" + str1;
  #ifdef DEBUG          
            Serial.print(topic);      
            Serial.print(" ");
            Serial.println(str2);      
  #endif
            valueStr = str2;
            myMqttPtr->publish(topic, valueStr, 0, 1);  
            delay(100);             
            break;  
          case CMD_SET_CLOUD_CONNECT:        
  #ifdef DEBUG        
            Serial.println("REC CMD_SET_CLOUD_CONNECT");
  #endif          
            myMqttPtr = new MQTT("", cloud_host, 1883);
  
            clientName = "";
            uint8_t mac[6];
            WiFi.macAddress(mac);
            clientName += macToStr(mac);
            clientName += "-";
            clientName += String(micros() & 0xff, 16);
            myMqttPtr->setClientId((char*) clientName.c_str());
  
            myMqttPtr->onConnected(myConnectedCb);
            myMqttPtr->onDisconnected(myDisconnectedCb);
            myMqttPtr->onPublished(myPublishedCb);
            myMqttPtr->onData(myDataCb);
  #ifdef DEBUG  
            Serial.println("connect mqtt...");
  #endif          
            myMqttPtr->setUserPwd(cloud_user, cloud_pass);  
            myMqttPtr->connect();
            delay(20);
            break;
          case   CMD_PING:
            OutgoingData.command = CMD_PONG;          
            ETout.sendData();
            OutgoingData.command = CMD_NONE;
  #ifdef DEBUG
            Serial.println("PING PONG");    
  #endif          
            break;
        }
      } 
    }
  }
}

void parseStrings(const char* in, char* out1, char* out2)
{
  strcpy(out1, in);  
  int len = strlen(out1) + 1;

  char* in1;
  in1 = (char *)in;
  in1 = in1 + len;
  
  if (len>1)
    strcpy(out2, in1);  
  else  
    out2[0] = 0;
}

void myConnectedCb() {
#ifdef DEBUG
  Serial.println("connected to MQTT server");
#endif
  OutgoingData.command = CMD_SET_CLOUD_CONNECTED;
}

void myDisconnectedCb() {
#ifdef DEBUG
  Serial.println("disconnected...");
#endif
  OutgoingData.command = CMD_SET_CLOUD_DISCONNECTED;
}

void myPublishedCb() {
#ifdef DEBUG  
  Serial.println("published.");
#endif
}

void CopyValuesToOutgoing(char *out, const char* in1, const char* in2)
{
    strcpy(out, in1);
    int len = strlen(out) + 1;
    char* str1 = out;
    str1 += len;
    strcpy(str1, in2);
}

void myDataCb(String& topic, String& data) {  
#ifdef DEBUG  
  Serial.print("Received topic:");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);
#endif
  char* pch;
  char tmp[DATA_LEN];
  strcpy(tmp, topic.c_str());
  pch=strchr(tmp,'/');

  if (pch != NULL)
  {
    pch=strchr(pch+1,'/');

    if (pch != NULL)
    {
      pch++;
      CopyValuesToOutgoing(OutgoingData.data, pch, data.c_str()); 
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
#ifdef DEBUG      
      Serial.print("parameter received: ");
      Serial.println(OutgoingData.data);
#endif      
    }
  } 
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

void ResetTimer()
{
  startTime = millis();
}

boolean IsTimeout(long timeout)
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < timeout )
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < timeout )
      return false;
  }
  return true;
}
