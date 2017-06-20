/*
  V1.0 - first version

  Created by Igor Jarc <admin@iot-playground.com>
  See http://iot-playground.com for details

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/
#include <UTFT.h>
#include <URTouch.h>
#include <avr/eeprom.h>
#include <TimeLib.h>
#include <Time.h>
#include "DHT.h"
#include <DS1302RTC.h>
#include <Timezone.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#include <EasyTransfer.h>


#include <SevenSegNumFontPlus.h>
#include <SevenSeg_XXXL_Num.h>
#include <arial_normal.h>
#include <ArialNumFontPlus.h>


#define DATA_LEN 55

#define DELAY_ADD_PARAM 0

char str1[DATA_LEN + 1];
char str2[DATA_LEN + 1];


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




EasyTransfer ETin;
EasyTransfer ETout;


struct DATA_STRUCTURE {
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int8_t header;
  int8_t command;
  char data[DATA_LEN + 1 ];
} __attribute__((packed));


DATA_STRUCTURE OutgoingData;
DATA_STRUCTURE IncomingData;


// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Arduino Uno / 2009:
// -------------------
// Standard Arduino Uno/2009 shield            : <display model>,A5,A4,A3,A2
// DisplayModule Arduino Uno TFT shield        : <display model>,A5,A4,A3,A2
//
// Arduino Mega:
// -------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Mega      : <display model>,38,39,40,41
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ITDB32S, 38, 39, 40, 41);

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
URTouch  myTouch( 6, 5, 4, 3, 2);


// Set pins:  CE, IO,CLK
DS1302RTC RTC(11, 10, 9);


#define DHT22_PIN         8
#define ESP_RESET_PIN     12
#define RELAY_PIN         51

#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relaysu

#define HEATER_MODE_OFF   0
#define HEATER_MODE_AUTO  1
#define HEATER_MODE_LOLO  2
#define HEATER_MODE_LO    3
#define HEATER_MODE_HI    4
#define HEATER_MODE_HIHI  5

#define TEMPERATURE_READ_INTERVAL  3  // in s
#define MILS_IN_SEC  1000



#define SCREEN_MAIN           0
#define SCREEN_SETTINGS       1
#define SCREEN_SET_DATETIME   2
#define SCREEN_TEMPWERATURE   3
#define SCREEN_SCHEDULE       4
#define SCREEN_WIFI_SETTINGS  5

int screen = SCREEN_MAIN;

#define TEMP_STEP       0.5
#define TEPM_HISTERESIS 0.4
#define MAX_SCHEDULES   10

int _rec = 0;


#define DAYS_ALL        0
#define DAYS_WEEK       1
#define DAYS_WEEKEND    2
#define DAYS_MO         3
#define DAYS_TU         4
#define DAYS_WE         5
#define DAYS_TH         6
#define DAYS_FR         7
#define DAYS_SA         8
#define DAYS_SU         9

#define CONFIG_REVISION 12345L
#define EEPROM_CONFIG_OFFSET 2


#define PARAM_HUM           "Sensor.Parameter1"  // room humidity
#define PARAM_TEMP          "Sensor.Parameter2"  // room temperature
#define PARAM_BARO          "Sensor.Parameter3"  // room pressure
#define PARAM_TEMP1         "Sensor.Parameter4"  // temperature 1
#define PARAM_TEMP2         "Sensor.Parameter5"  // temperature 2
#define PARAM_HUM1          "Sensor.Parameter6"  // humidity 1
#define PARAM_MODE          "Sensor.Parameter7"  // mode 0-off,1-auto, 2-lolo, 3-lo, 4-hi, 5-hihi
#define PARAM_TEMP_LOLO     "Sensor.Parameter8"  // set temperature LOLO
#define PARAM_TEMP_LO       "Sensor.Parameter9"  // set temperature LO
#define PARAM_TEMP_HI       "Sensor.Parameter10" // set temperature HI
#define PARAM_TEMP_HIHI     "Sensor.Parameter11" // set temperature HIHI
#define PARAM_HEATING       "Sensor.Parameter12" // DI heating 
#define PARAM_FORECAST      "Sensor.Parameter13" // forecast 
#define PARAM_TEMP_TARGET   "Sensor.Parameter14" // target temperature
#define PARAM_ICON          "Settings.Icon1"     // icon

char *days[DAYS_SU + 1] = {
  "All      ", "Weekdays ", "Weekend  ", "Monday   ", "Tuesday  ", "Wednesday", "Thursday ", "Friday   ", "Saturday ", "Sunday   "
};


char Dbuf1[60];  // all the rest for building messages with sprintf
char Dbuf[60];

typedef struct s_shedule {
  boolean enabled;
  unsigned char days;
  unsigned char hourStart;
  unsigned char minuteStart;
  unsigned char hourStop;
  unsigned char minuteStop;
  unsigned char temp;
};

typedef struct s_config {
  long  config_revision;
  int heaterMode;
  float temp_lolo;
  float temp_lo;
  float temp_hi;
  float temp_hihi;
  uint16_t  moduleId;
  char  ap_ssid[30];
  char  ap_password[30];
  char  host[30];
  char  cloud_username[30];
  char  cloud_password[30];
  s_shedule schedules[MAX_SCHEDULES];
}
config;


config configuration;

// change if you are not in CET
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {
  "CEST", Last, Sun, Mar, 2, 120
};     //Central European Summer Time
TimeChangeRule CET = {
  "CET ", Last, Sun, Oct, 3, 60
};       //Central European Standard Time
Timezone tzc(CEST, CET);


unsigned char tmpHour;
unsigned char tmpMinute;
unsigned char tmpDay;
unsigned char tmpMonth;
unsigned int tmpYear;

DHT dht;

int cnt;

boolean clock;
float temperature = 22;
float humidity = 50;


float temperature1 = 22;
float humidity1 = 50;

float temperature2 = 22;


// forecast
#define FORECAST_CALC_INTERVAL     60 // 60s 
int forecastInterval;
int minuteCount = 0;
double pressureSamples[9][6];
double pressureAvg[9];
//bool firstRound = true;
double dP_dt;
const char *weather[] = {
  "stable      ", "sunny       ", "cloudy      ", "unstable    ", "thunderstorm", "unknown     "
};
double pressureF;
SFE_BMP180 bmp180;
#define ALTITUDE 301.0 // Altitude of my home
int forecast = 5;
boolean heaterStatus;


#define TMPERATURE_NO_TARGET     -9999
float temperatureTarget = TMPERATURE_NO_TARGET;
boolean heating;

unsigned long startTime;
unsigned long startTime1;


typedef enum {
  COMM_START,
  COMM_WAIT_RESET,
  COMM_SET_AP,
  COMM_AP_CONNECTING,
  COMM_AP_CONNECTED,
  COMM_CLOUD_CONNECTING,
  COMM_CLOUD_CONNECTED,
  COMM_NEW_MODULE_ID,
  COMM_MODULE_CONFIGURATION,
  COMM_SUBSCRIBE,
  COMM_PUBLISH,
  COMM_IDLE,
  COMM_WAIT_PONG,
} comm_state_type;

comm_state_type comm_state;


#define DEFAULT_AP_SSID         "XXXX"
#define DEFAULT_AP_PASSWORD     "XXXX"
#define DEFAULT_CLOUD_HOST      "cloud.iot-playground.com"
#define DEFAULT_CLOUD_USERNAME  "XXXX"
#define DEFAULT_CLOUD_PASSWORD  "XXXX"

void setup()
{
  comm_state = COMM_START;

  pinMode(RELAY_PIN, OUTPUT);

  pinMode(ESP_RESET_PIN, OUTPUT);


  Serial1.begin(9600);

  ETin.begin(details(IncomingData), &Serial1);
  ETout.begin(details(OutgoingData), &Serial1);

  hwReset();

  Serial.begin(115200);

  eeprom_read_block((void*)&configuration, (void*)EEPROM_CONFIG_OFFSET, sizeof(configuration));

  if (configuration.config_revision != CONFIG_REVISION)
  { // default values
    configuration.config_revision = CONFIG_REVISION;
    configuration.heaterMode = HEATER_MODE_OFF;
    configuration.temp_lolo = 19;
    configuration.temp_lo = 20;
    configuration.temp_hi = 21;
    configuration.temp_hihi = 22;

    configuration.moduleId = 0;
    strcpy(configuration.ap_ssid, DEFAULT_AP_SSID);
    strcpy(configuration.ap_password, DEFAULT_AP_PASSWORD);
    strcpy(configuration.host, DEFAULT_CLOUD_HOST);
    strcpy(configuration.cloud_username, DEFAULT_CLOUD_USERNAME);
    strcpy(configuration.cloud_password, DEFAULT_CLOUD_PASSWORD);

    for (int i = 0; i < MAX_SCHEDULES; i++)
    {
      configuration.schedules[i].enabled = false;
      configuration.schedules[i].days = DAYS_ALL;
      configuration.schedules[i].hourStart = 0;
      configuration.schedules[i].minuteStart = 0;
      configuration.schedules[i].hourStop = 0;
      configuration.schedules[i].minuteStop = 0;
      configuration.schedules[i].temp = HEATER_MODE_HI;
    }

    SaveConfiguration();
  }

  processComm();

  //BMP
  bmp180.begin();

  // DHT22
  dht.setup(DHT22_PIN); // data pin 8

  myGLCD.InitLCD();
  myGLCD.clrScr();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  setSyncProvider(RTC.get);

  DrawScreenMain();
  startTime = millis();
  cnt = TEMPERATURE_READ_INTERVAL + 1;

  forecastInterval = FORECAST_CALC_INTERVAL + 1;
}



void hwReset()
{
  digitalWrite(ESP_RESET_PIN, LOW);
  delay(500);              // wait
  digitalWrite(ESP_RESET_PIN, HIGH);
}


int waitConnCounter = 0;

void CopyValuesToOutgoing(char *out, const char* in1, const char* in2)
{
  strcpy(out, in1);
  int len = strlen(out) + 1;
  char* str1 = out;
  str1 += len;
  strcpy(str1, in2);
}

void parseStrings(const char* in, char* out1, char* out2)
{
  strcpy(out1, in);
  int len = strlen(out1) + 1;

  char* in1;
  in1 = (char *)in;
  in1 = in1 + len;

  if (len > 1)
    strcpy(out2, in1);
  else
    out2[0] = 0;
}

void recData()
{
  if (ETin.receiveData())
  {
    if (IncomingData.header == 0x68)
    {
      switch (IncomingData.command)
      {
        case CMD_NONE:
          break;
        case CMD_INIT_FINISHED:
          comm_state =  COMM_SET_AP;
          Serial.println("REC CMD_INIT_FINISHED");
          break;
        case CMD_AP_CONNECTED:
          comm_state = COMM_AP_CONNECTED;
          Serial.println("REC CMD_AP_CONNECTED");
          break;
        case CMD_AP_CONN_TIMEOUT:
          Serial.println("REC CMD_AP_CONN_TIMEOUT");
          comm_state = COMM_START;
          break;
        case CMD_SET_CLOUD_CONNECTED:
          Serial.println("REC CMD_SET_CLOUD_CONNECTED");
          comm_state = COMM_CLOUD_CONNECTED;
          break;
        case CMD_SET_CLOUD_DISCONNECTED:
          Serial.println("REC CMD_SET_CLOUD_DISCONNECTED");
          break;
        case CMD_SET_MODULE_ID:
          Serial.print("REC CMD_SET_MODULE_ID ");
          configuration.moduleId = (IncomingData.data[1] << 8) + (uint8_t)IncomingData.data[0];
          Serial.println(configuration.moduleId);
          comm_state = COMM_MODULE_CONFIGURATION;
          break;
        case CMD_SET_PARAMETER_VALUE:
          Serial.print("REC CMD_SET_PARAMETER_VALUE ");
          parseStrings((char *)IncomingData.data, str1, str2);

          if (strcmp(PARAM_MODE, str1) == 0 )
          {
            Serial.print(PARAM_MODE);
            Serial.println(" ");
            Serial.println(str2);
            configuration.heaterMode = atoi(str2);
            UpdateSetTemp();
            DrawModeButton(true);
            UpdateHeaterStatus();
            SendModeStatus();
            SaveConfiguration();
          }
          else if (strcmp(PARAM_TEMP1, str1) == 0 )
          {
            temperature1 = atof(str2);
            DrawTemp1(false);
          }
          else if (strcmp(PARAM_TEMP2, str1) == 0 )
          {
            temperature2 = atof(str2);
            DrawTemp2(false);
          }
          else if (strcmp(PARAM_HUM1, str1) == 0 )
          {
            humidity1 = atof(str2);
            DrawTemp1(false);
          }
          break;
        case CMD_PONG:
          comm_state = COMM_IDLE;
          Serial.print("REC CMD_PONG ");
          Serial.println(millis());
          ResetTimer1();
          break;
      }
    }
    else
      Serial.println("invalid header");
  }

}

void recDelay(int i)
{
  while (i-- > 0)
  {
    recData();
    delay(1);
  }
}

void processComm()
{
  sendData();

  recData();

  switch (comm_state)
  {
    case COMM_START:
      OutgoingData.command = CMD_RESET;
      waitConnCounter = 0;
      sendData();
      comm_state = COMM_WAIT_RESET;
      Serial.println("SEND CMD_RESET");
      hwReset();
      break;
    case COMM_WAIT_RESET:
      if (waitConnCounter++ > 50)
      {
        waitConnCounter = 0;
        comm_state = COMM_START;
        Serial.println("CMD_RESET timeout");
      }
      else
        delay(100);
      break;
    case COMM_SET_AP:
      strcpy(OutgoingData.data, configuration.ap_ssid);
      Serial.println("Send AP name");
      OutgoingData.command = CMD_SET_AP_NAME;
      sendData();

      strcpy(OutgoingData.data, configuration.ap_password);
      Serial.println("Send AP pwd");
      OutgoingData.command = CMD_SET_AP_PASSWORD;
      sendData();

      Serial.println("connect to AP");
      OutgoingData.command = CMD_CONNECT_TO_AP;
      sendData();
      waitConnCounter = 0;
      comm_state = COMM_AP_CONNECTING;
      break;
    case COMM_AP_CONNECTING:
      if (waitConnCounter++ > 400)
      {
        waitConnCounter = 0;
        comm_state = COMM_START;
        Serial.println("CMD_CONNECT_TO_AP timeout");
      }
      else
        delay(100);
      break;
    case COMM_AP_CONNECTED:
      strcpy(OutgoingData.data, configuration.host);
      Serial.println("Send Cloud host");
      OutgoingData.command = CMD_SET_CLOUD_HOST;
      sendData();

      strcpy(OutgoingData.data, configuration.cloud_username);
      Serial.println("Send Cloud user");
      OutgoingData.command = CMD_SET_CLOUD_USER;
      sendData();

      strcpy(OutgoingData.data, configuration.cloud_password);
      Serial.println("Send Cloud pwd");
      OutgoingData.command = CMD_SET_CLOUD_PASSWORD;
      sendData();

      Serial.println("MQTT connect");
      OutgoingData.command = CMD_SET_CLOUD_CONNECT;
      sendData();
      comm_state = COMM_CLOUD_CONNECTING;
      waitConnCounter = 0;
    case COMM_CLOUD_CONNECTING:
      if (waitConnCounter++ > 500)
        comm_state = COMM_START;
      break;
    case COMM_CLOUD_CONNECTED:
      if (configuration.moduleId == 0)
      {
        OutgoingData.command = CMD_GET_NEW_MODULE;
        sendData();
        comm_state = COMM_NEW_MODULE_ID;
        waitConnCounter = 0;
      }
      else
      {
        OutgoingData.data[0] = configuration.moduleId & 0xFF;
        OutgoingData.data[1] = (configuration.moduleId >> 8) & 0xFF;
        OutgoingData.command = CMD_SET_MODULE_ID;
        sendData();
        comm_state = COMM_SUBSCRIBE;
        Serial.println("SEND CMD_SET_MODULE_ID");
      }
      break;
    case COMM_NEW_MODULE_ID:
      if (waitConnCounter++ > 1500)
        comm_state = COMM_CLOUD_CONNECTED;
      break;
    case COMM_MODULE_CONFIGURATION:
      // konfiguriraj modul - dodaj parametre in nastavitve
      if (configuration.moduleId != 0)
      {
        strcpy(OutgoingData.data, "MT_GENERIC");
        Serial.println("Send module type");
        OutgoingData.command = CMD_SET_MODULE_TYPE;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_ICON);
        strcpy(OutgoingData.data, PARAM_ICON);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_ICON, "dimmer.png");
        Serial.println("set parameter value");
        OutgoingData.command = CMD_SET_PARAMETER_VALUE;
        sendData();
        delay(DELAY_ADD_PARAM);

        ///////
        Serial.print("add new parameter ");
        Serial.println(PARAM_HUM);
        strcpy(OutgoingData.data, PARAM_HUM);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, "Room hum.");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, "%");
        Serial.println("set parameter Unit");
        OutgoingData.command = CMD_SET_PARAMETER_UNIT;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, "5");
        Serial.println("set parameter DBAVGINTERVAL");
        OutgoingData.command = CMD_SET_PARAMETER_DBAVGINTERVAL;
        sendData();
        delay(DELAY_ADD_PARAM);

        /////
        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP);
        strcpy(OutgoingData.data, PARAM_TEMP);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, "Room temp.");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, "°C");
        Serial.println("set parameter Unit");
        OutgoingData.command = CMD_SET_PARAMETER_UNIT;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, "5");
        Serial.println("set parameter DBAVGINTERVAL");
        OutgoingData.command = CMD_SET_PARAMETER_DBAVGINTERVAL;
        sendData();
        delay(DELAY_ADD_PARAM);

        //////
        Serial.print("add new parameter ");
        Serial.println(PARAM_BARO);
        strcpy(OutgoingData.data, PARAM_BARO);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, "Pressure");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, "mb");
        Serial.println("set parameter Unit");
        OutgoingData.command = CMD_SET_PARAMETER_UNIT;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, "5");
        Serial.println("set parameter DBAVGINTERVAL");
        OutgoingData.command = CMD_SET_PARAMETER_DBAVGINTERVAL;
        sendData();
        delay(DELAY_ADD_PARAM);

        //////////
        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP1);
        strcpy(OutgoingData.data, PARAM_TEMP1);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP2);
        strcpy(OutgoingData.data, PARAM_TEMP2);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_HUM1);
        strcpy(OutgoingData.data, PARAM_HUM1);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        ///////////
        Serial.print("add new parameter ");
        Serial.println(PARAM_MODE);
        strcpy(OutgoingData.data, PARAM_MODE);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_MODE, "Mode");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_MODE, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_MODE, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_ISCOMMAND;
        sendData();
        delay(DELAY_ADD_PARAM);

        //////
        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP_LOLO);
        strcpy(OutgoingData.data, PARAM_TEMP_LOLO);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP_LO);
        strcpy(OutgoingData.data, PARAM_TEMP_LO);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP_HI);
        strcpy(OutgoingData.data, PARAM_TEMP_HI);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP_HIHI);
        strcpy(OutgoingData.data, PARAM_TEMP_HIHI);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        ////////////
        Serial.print("add new parameter ");
        Serial.println(PARAM_HEATING);
        strcpy(OutgoingData.data, PARAM_HEATING);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HEATING, "Heating");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_HEATING, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        /////////////
        Serial.print("add new parameter ");
        Serial.println(PARAM_FORECAST);
        strcpy(OutgoingData.data, PARAM_FORECAST);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        Serial.print("add new parameter ");
        Serial.println(PARAM_TEMP_TARGET);
        strcpy(OutgoingData.data, PARAM_TEMP_TARGET);
        OutgoingData.command = CMD_GET_NEW_PARAMETER;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, "Target temp.");
        Serial.println("set parameter description");
        OutgoingData.command = CMD_SET_PARAMETER_DESCRIPTION;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, "°C");
        Serial.println("set parameter Unit");
        OutgoingData.command = CMD_SET_PARAMETER_UNIT;
        sendData();
        delay(DELAY_ADD_PARAM);

        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, "true");
        Serial.println("set parameter DBLogging");
        OutgoingData.command = CMD_SET_PARAMETER_DBLOGGING;
        sendData();
        delay(DELAY_ADD_PARAM);

        SaveConfiguration();
        comm_state = COMM_SUBSCRIBE;
      }
      else
        Serial.println("Error adding new module to cloud");
      break;
    case COMM_SUBSCRIBE:
      strcpy(OutgoingData.data, PARAM_MODE);
      Serial.println("subscribe parameter PARAM_MODE");
      OutgoingData.command = CMD_SUBSCRIBE_PARAMETER;
      sendData();

      strcpy(OutgoingData.data, PARAM_TEMP1);
      Serial.println("subscribe parameter PARAM_TEMP1");
      OutgoingData.command = CMD_SUBSCRIBE_PARAMETER;
      sendData();

      strcpy(OutgoingData.data, PARAM_TEMP2);
      Serial.println("subscribe parameter PARAM_TEMP2");
      OutgoingData.command = CMD_SUBSCRIBE_PARAMETER;
      sendData();

      strcpy(OutgoingData.data, PARAM_HUM1);
      Serial.println("subscribe parameter PARAM_HUM1");
      OutgoingData.command = CMD_SUBSCRIBE_PARAMETER;
      sendData();

      comm_state = COMM_PUBLISH;
      break;
    case COMM_PUBLISH:

      // humidity
      CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, String((int)humidity).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      // temp
      CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, String(temperature).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      // pressure
      CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, String((int)pressureF).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      // mode
      CopyValuesToOutgoing(OutgoingData.data, PARAM_MODE, String(configuration.heaterMode).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LOLO, String(configuration.temp_lolo).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LO, String(configuration.temp_lo).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HI, String(configuration.temp_hi).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HIHI, String(configuration.temp_hihi).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_HEATING, heaterStatus ? "1" : "0");
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      CopyValuesToOutgoing(OutgoingData.data, PARAM_FORECAST, weather[forecast]);
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();

      if ((configuration.heaterMode == HEATER_MODE_OFF) || (temperatureTarget == TMPERATURE_NO_TARGET))
        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, String(temperature).c_str());
      else
        CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, String(temperatureTarget).c_str());
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();
      
      ResetTimer1();
      comm_state = COMM_IDLE;
      break;
    case COMM_IDLE:
      if (IsTimeout1(30000))
      {
        OutgoingData.command = CMD_PING;
        sendData();
        Serial.print("SEND PING ");
        Serial.println(now());
        comm_state = COMM_WAIT_PONG;
        ResetTimer1();
      }
      else
        delay(1);
      break;
    case COMM_WAIT_PONG:
      if (IsTimeout1(1000))
      {
        comm_state = COMM_START;
        Serial.print("PING timeout ");
        Serial.println(millis());
      }
      else
        delay(1);
      break;
  }
  sendData();
}

void sendData()
{
  if (OutgoingData.command != CMD_NONE)
  {
    OutgoingData.header = 0x68;
    ETout.sendData();
    OutgoingData.command = CMD_NONE;
    delay(250);
    Serial.print("Send data ");
    OutgoingData.data[DATA_LEN] = 0;
    Serial.println(OutgoingData.data);
  }
}


void loop()
{
  processComm();

  // handle buttons
  if (screen == SCREEN_MAIN)
  {
    ScreenMainButtonHandler();
  }
  else if (screen == SCREEN_SETTINGS)
  {
    ScreenSettingsButtonHandler();
  }
  else if (screen == SCREEN_TEMPWERATURE)
  {
    ScreenTemperatureButtonHandler();
  }
  else if (screen == SCREEN_SCHEDULE)
  {
    ScreenScheduleButtonHandler();
  }
  else if (screen == SCREEN_SET_DATETIME)
  {
    ScreenDatetimeeButtonHandler();
  }
  else if (screen == SCREEN_WIFI_SETTINGS)
  {
    ScreenWiFiSettingsButtonHandler();
  }

  if (IsTimeout())
  {
    cnt++ ;
    forecastInterval++;
    startTime = millis();
    DrawTime(10, 110, false);
  }

  if (forecastInterval > FORECAST_CALC_INTERVAL)
  {
    forecastInterval = 0;

    ReadPressure();
    forecast = calculateForecast(pressureF);

    static int lastSendPresInt;
    int pres = round(pressureF * 10);

    if (pres != lastSendPresInt)
    {
      lastSendPresInt = pres;
      if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
      {
        CopyValuesToOutgoing(OutgoingData.data, PARAM_BARO, String((int)pressureF).c_str());
        OutgoingData.command = CMD_SET_PARAMETER_VALUE;
        sendData();
      }
    }

    static int lastSendForeInt = -1;

    if (forecast != lastSendForeInt)
    {
      lastSendForeInt = forecast;
      if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
      {
        CopyValuesToOutgoing(OutgoingData.data, PARAM_FORECAST, weather[forecast]);
        OutgoingData.command = CMD_SET_PARAMETER_VALUE;
        sendData();
      }
    }

    DrawPressure(false);
    DrawForecast(false);
  }

  if (cnt > TEMPERATURE_READ_INTERVAL)
  {
    cnt = 0;

    float humidityReading = dht.getHumidity();
    float tempReading = dht.getTemperature();

    if (!isnan(humidityReading))
    {
      if (humidityReading <= 100)
        humidity = humidityReading;
    }

    static int lastSendHumInt;
    int hum = round(humidity);

    if (hum != lastSendHumInt)
    {
      lastSendHumInt = hum;
      if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
      {
        CopyValuesToOutgoing(OutgoingData.data, PARAM_HUM, String((int)humidity).c_str());
        OutgoingData.command = CMD_SET_PARAMETER_VALUE;
        sendData();
      }
    }

    if (isnan(tempReading))
    {
      Serial.println("Failed reading temperatur from DHT");
    }
    else
    {
      if (abs(temperature - tempReading) > 10)
        temperature += tempReading / 16.0 - temperature / 16.0;
      else
        temperature += tempReading / 4.0 - temperature / 4.0;

      static int lastSendTempInt;
      int temp = round(temperature * 10);

      if (temp != lastSendTempInt)
      {
        lastSendTempInt = temp;
        if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
        {
          CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP, String(temperature).c_str());
          OutgoingData.command = CMD_SET_PARAMETER_VALUE;
          sendData();

          if ((configuration.heaterMode == HEATER_MODE_OFF) || (temperatureTarget == TMPERATURE_NO_TARGET))
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, String(temperature).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            Serial.println("send PARAM_TEMP_TARGET 1");
          }
        }
      }

      DrawCurrentTemp(10, 30, temperature, humidity);

      UpdateSetTemp();
      DrawModeButton(220, 10, false);
      UpdateHeaterStatus();
    }
  }
}


void ReadPressure()
{
  //pressureF++;
  char status;
  double T, P, p0, a;

  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = bmp180.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = bmp180.getTemperature(T);
    if (status != 0)
    {
      status = bmp180.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = bmp180.getPressure(P, T);
        if (status != 0)
        {
          pressureF = bmp180.sealevel(P, ALTITUDE); // we're at 1655 meters (Boulder, CO)
        }
      }
    }
  }
}

void SendModeStatus()
{
  if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
  {
    CopyValuesToOutgoing(OutgoingData.data, PARAM_MODE, String(configuration.heaterMode).c_str());
    OutgoingData.command = CMD_SET_PARAMETER_VALUE;
    sendData();
  }
}

int calculateForecast(double pressure) {
  //From 0 to 5 min.
  if (minuteCount <= 5) {
    pressureSamples[0][minuteCount] = pressure;
  }
  //From 30 to 35 min.
  else if ((minuteCount >= 30) && (minuteCount <= 35)) {
    pressureSamples[1][minuteCount - 30] = pressure;
  }
  //From 60 to 65 min.
  else if ((minuteCount >= 60) && (minuteCount <= 65)) {
    pressureSamples[2][minuteCount - 60] = pressure;
  }
  //From 90 to 95 min.
  else if ((minuteCount >= 90) && (minuteCount <= 95)) {
    pressureSamples[3][minuteCount - 90] = pressure;
  }
  //From 120 to 125 min.
  else if ((minuteCount >= 120) && (minuteCount <= 125)) {
    pressureSamples[4][minuteCount - 120] = pressure;
  }
  //From 150 to 155 min.
  else if ((minuteCount >= 150) && (minuteCount <= 155)) {
    pressureSamples[5][minuteCount - 150] = pressure;
  }
  //From 180 to 185 min.
  else if ((minuteCount >= 180) && (minuteCount <= 185)) {
    pressureSamples[6][minuteCount - 180] = pressure;
  }
  //From 210 to 215 min.
  else if ((minuteCount >= 210) && (minuteCount <= 215)) {
    pressureSamples[7][minuteCount - 210] = pressure;
  }
  //From 240 to 245 min.
  else if ((minuteCount >= 240) && (minuteCount <= 245)) {
    pressureSamples[8][minuteCount - 240] = pressure;
  }


  if (minuteCount == 5) {
    // Avg pressure in first 5 min, value averaged from 0 to 5 min.
    pressureAvg[0] = ((pressureSamples[0][0] + pressureSamples[0][1]
                       + pressureSamples[0][2] + pressureSamples[0][3]
                       + pressureSamples[0][4] + pressureSamples[0][5]) / 6);
  }
  else if (minuteCount == 35) {
    // Avg pressure in 30 min, value averaged from 0 to 5 min.
    pressureAvg[1] = ((pressureSamples[1][0] + pressureSamples[1][1]
                       + pressureSamples[1][2] + pressureSamples[1][3]
                       + pressureSamples[1][4] + pressureSamples[1][5]) / 6);
    float change = (pressureAvg[1] - pressureAvg[0]);
    dP_dt = change / 5;
  }
  else if (minuteCount == 65) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[2] = ((pressureSamples[2][0] + pressureSamples[2][1]
                       + pressureSamples[2][2] + pressureSamples[2][3]
                       + pressureSamples[2][4] + pressureSamples[2][5]) / 6);
    float change = (pressureAvg[2] - pressureAvg[0]);
    dP_dt = change / 10;
  }
  else if (minuteCount == 95) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[3] = ((pressureSamples[3][0] + pressureSamples[3][1]
                       + pressureSamples[3][2] + pressureSamples[3][3]
                       + pressureSamples[3][4] + pressureSamples[3][5]) / 6);
    float change = (pressureAvg[3] - pressureAvg[0]);
    dP_dt = change / 15;
  }
  else if (minuteCount == 125) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[4] = ((pressureSamples[4][0] + pressureSamples[4][1]
                       + pressureSamples[4][2] + pressureSamples[4][3]
                       + pressureSamples[4][4] + pressureSamples[4][5]) / 6);
    float change = (pressureAvg[4] - pressureAvg[0]);
    dP_dt = change / 20;
  }
  else if (minuteCount == 155) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[5] = ((pressureSamples[5][0] + pressureSamples[5][1]
                       + pressureSamples[5][2] + pressureSamples[5][3]
                       + pressureSamples[5][4] + pressureSamples[5][5]) / 6);
    float change = (pressureAvg[5] - pressureAvg[0]);
    dP_dt = change / 25;
  }
  else if (minuteCount == 185) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[6] = ((pressureSamples[6][0] + pressureSamples[6][1]
                       + pressureSamples[6][2] + pressureSamples[6][3]
                       + pressureSamples[6][4] + pressureSamples[6][5]) / 6);
    float change = (pressureAvg[6] - pressureAvg[0]);
    dP_dt = change / 30;
  }
  else if (minuteCount == 215) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[7] = ((pressureSamples[7][0] + pressureSamples[7][1]
                       + pressureSamples[7][2] + pressureSamples[7][3]
                       + pressureSamples[7][4] + pressureSamples[7][5]) / 6);
    float change = (pressureAvg[7] - pressureAvg[0]);
    dP_dt = change / 35;
  }
  else if (minuteCount == 245) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[8] = ((pressureSamples[8][0] + pressureSamples[8][1]
                       + pressureSamples[8][2] + pressureSamples[8][3]
                       + pressureSamples[8][4] + pressureSamples[8][5]) / 6);
    float change = (pressureAvg[8] - pressureAvg[0]);
    dP_dt = change / 40; // note this is for t = 4 hour

    minuteCount -= 30;
    pressureAvg[0] = pressureAvg[1];
    pressureAvg[1] = pressureAvg[2];
    pressureAvg[2] = pressureAvg[3];
    pressureAvg[3] = pressureAvg[4];
    pressureAvg[4] = pressureAvg[5];
    pressureAvg[5] = pressureAvg[6];
    pressureAvg[6] = pressureAvg[7];
    pressureAvg[7] = pressureAvg[8];
  }

  minuteCount++;

  // http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf table 4

  if (minuteCount < 36) //if time is less than 35 min
    return 5; // Unknown, more time needed
  else if (dP_dt < (-0.25))
    return 4; // Quickly falling LP, Thunderstorm, not stable
  else if (dP_dt > 0.25)
    return 3; // Quickly rising HP, not stable weather
  else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
    return 2; // Slowly falling Low Pressure System, stable rainy weather
  else if ((dP_dt > 0.05) && (dP_dt < 0.25))
    return 1; // Slowly rising HP stable good weather
  else if ((dP_dt > (-0.05)) && (dP_dt < 0.05))
    return 0; // Stable weather
  else
    return 5; // Unknown
}


void ResetTimer1()
{
  startTime1 = millis();
}

boolean IsTimeout1(long timeout)
{
  unsigned long now = millis();
  if (startTime1 <= now)
  {
    if ( (unsigned long)(now - startTime1 )  < timeout )
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime1 - now) < timeout )
      return false;
  }

  return true;
}


boolean IsTimeout()
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < MILS_IN_SEC )
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < MILS_IN_SEC )
      return false;
  }

  return true;
}

void SaveConfiguration()
{
  config tmpconfig;
  eeprom_read_block((void*)&tmpconfig, (void*)EEPROM_CONFIG_OFFSET, sizeof(tmpconfig));

  byte *b1;
  byte *b2;

  b1 = (byte *)&tmpconfig;
  b2 = (byte *)&configuration;

  // store in EEPROM only if changed
  for (int i = 0; i < sizeof(configuration); i++)
  {
    if (*b1 != *b2)
    {
      eeprom_write_block((const void*)&configuration, (void*)EEPROM_CONFIG_OFFSET, sizeof(configuration));
      break;
    }
    b1++;
    b2++;
  }
}

void WaitTouch()
{
  delay(300);
  while (myTouch.dataAvailable())
    myTouch.read();
}


// Draw a red frame while a button is touched
void DrawButtonPressed(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}

time_t timelocal(time_t utctime) {
  time_t rt;
  rt = tzc.toLocal(utctime);
  //if(year(rt) < 2036) {
  return (rt);
  //    }
  //    else {
  //  return(RTC.get());
  //    }
}


void UpdateHeaterStatus()
{
  static boolean oldHeaterStatus;

  if (temperature < (temperatureTarget - TEPM_HISTERESIS / 2))
    heaterStatus = true;
  else if (temperature > (temperatureTarget + TEPM_HISTERESIS / 2))
    heaterStatus = false;
  else
    heaterStatus = oldHeaterStatus;


  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setFont(arial_normal);

    if (heaterStatus)
      myGLCD.print("ON ", 160, 10);
    else
      myGLCD.print("OFF", 160, 10);
  }

  Serial.print("Update heater status ");
  Serial.println(heaterStatus);
  // set heater status
  digitalWrite(RELAY_PIN, heaterStatus ? RELAY_ON : RELAY_OFF);

  if (oldHeaterStatus != heaterStatus)
  {
    oldHeaterStatus = heaterStatus;
    if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
    {
      CopyValuesToOutgoing(OutgoingData.data, PARAM_HEATING, heaterStatus ? "1" : "0");
      OutgoingData.command = CMD_SET_PARAMETER_VALUE;
      sendData();
      delay(500);
    }
  }
}


void UpdateSetTemp()
{
  if (configuration.heaterMode == HEATER_MODE_OFF)
  {
    temperatureTarget = TMPERATURE_NO_TARGET;
  }
  else if (configuration.heaterMode == HEATER_MODE_AUTO)
  {
    temperatureTarget = TMPERATURE_NO_TARGET;

    time_t t = timelocal(now());

    for (int i = 0; i < MAX_SCHEDULES; i++)
    {
      if (configuration.schedules[i].enabled)
      {
        int wd = weekday(t);
        boolean found = false;

        if ((configuration.schedules[i].days == DAYS_ALL) ||
            (configuration.schedules[i].days == DAYS_WEEK) && (wd >= 2 && wd <= 6) ||
            (configuration.schedules[i].days == DAYS_WEEKEND) && (wd == 1 || wd == 7) ||
            (configuration.schedules[i].days == DAYS_MO) && (wd == 2) ||
            (configuration.schedules[i].days == DAYS_TU) && (wd == 3) ||
            (configuration.schedules[i].days == DAYS_WE) && (wd == 4) ||
            (configuration.schedules[i].days == DAYS_TH) && (wd == 5) ||
            (configuration.schedules[i].days == DAYS_FR) && (wd == 6) ||
            (configuration.schedules[i].days == DAYS_SA) && (wd == 7) ||
            (configuration.schedules[i].days == DAYS_SU) && (wd == 1))
        {
          if ((configuration.schedules[i].hourStart == configuration.schedules[i].hourStop) && (configuration.schedules[i].minuteStart == configuration.schedules[i].minuteStop))
            continue;
          else if ((configuration.schedules[i].hourStart < configuration.schedules[i].hourStop) || (configuration.schedules[i].hourStart == configuration.schedules[i].hourStop) && (configuration.schedules[i].minuteStart < configuration.schedules[i].minuteStop))
          {
            if ((configuration.schedules[i].hourStart < hour(t) || ((configuration.schedules[i].hourStart == hour(t)) && (configuration.schedules[i].minuteStart <= minute(t)))) &&
                (((hour(t) < configuration.schedules[i].hourStop) || ((hour(t) == configuration.schedules[i].hourStop) && (minute(t) <= configuration.schedules[i].minuteStop)))))
            {
              found = true;
            }
          }
          else
          {
            if ((hour(t) > configuration.schedules[i].hourStart || hour(t) == configuration.schedules[i].hourStart && minute(t) >= configuration.schedules[i].minuteStart) ||
                ( hour(t) < configuration.schedules[i].hourStop || hour(t) == configuration.schedules[i].hourStop && minute(t) <= configuration.schedules[i].minuteStop))
            {
              found = true;
            }
          }

          if (found)
          {
            float settemp = configuration.temp_lolo;

            if (configuration.schedules[i].temp == HEATER_MODE_LOLO)
              settemp = configuration.temp_lolo;
            else if (configuration.schedules[i].temp == HEATER_MODE_LO)
              settemp = configuration.temp_lo;
            else if (configuration.schedules[i].temp == HEATER_MODE_HI)
              settemp = configuration.temp_hi;
            else if (configuration.schedules[i].temp == HEATER_MODE_HIHI)
              settemp = configuration.temp_hihi;

            if (settemp > temperatureTarget)
              temperatureTarget = settemp;
          }
        }
      }
    }
  }
  else if (configuration.heaterMode == HEATER_MODE_LOLO)
  {
    temperatureTarget = configuration.temp_lolo;
  }
  else if (configuration.heaterMode == HEATER_MODE_LO)
  {
    temperatureTarget = configuration.temp_lo;
  }
  else if (configuration.heaterMode == HEATER_MODE_HI)
  {
    temperatureTarget = configuration.temp_hi;
  }
  else if (configuration.heaterMode == HEATER_MODE_HIHI)
  {
    temperatureTarget = configuration.temp_hihi;
  }
}

/////////////////////////////////////
// begin Main screen
/////////////////////////////////////
void DrawScreenMain()
{
  screen = SCREEN_MAIN;

  myGLCD.clrScr();

  DrawHeaterStatus(10, 10);
  DrawCurrentTemp(10, 30, temperature, humidity);

  UpdateSetTemp();
  DrawModeButton(220, 10, true);
  UpdateHeaterStatus();
  DrawSettingsButton(220, 70);
  DrawTime(10, 110, true);


  DrawTemp1(true);
  DrawTemp2(true);
  DrawPressure(true);
  DrawForecast(true);
}

void DrawTemp1(boolean forceRedraw)
{
  static int lastHumidity1;
  static float lastTemperature1;
  int humidityI = round(humidity1);
  if ((humidityI != lastHumidity1) || (temperature1 != lastTemperature1) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastHumidity1 = humidityI;
      lastTemperature1 = temperature1;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      char str_temp[6];
      dtostrf(temperature1, 5, 1, str_temp);
      strcpy_P(Dbuf1, PSTR("Bedroom:%s'C %d%%"));
      sprintf(Dbuf, Dbuf1, str_temp, humidityI);

      myGLCD.print(Dbuf, 10, 180);
    }
  }
}



void DrawTemp2(boolean forceRedraw)
{
  static float lastTemperature2;
  if ((temperature2 != lastTemperature2) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastTemperature2 = temperature2;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      char str_temp[6];
      dtostrf(temperature2, 5, 1, str_temp);
      strcpy_P(Dbuf1, PSTR("Outdoor:%s'C"));
      sprintf(Dbuf, Dbuf1, str_temp);

      myGLCD.print(Dbuf, 10, 200);
    }
  }
}

void DrawPressure(boolean forceRedraw)
{
  static int lastPressure;
  int pressuseI = round(pressureF);
  if ((pressuseI != lastPressure) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastPressure = pressuseI;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      strcpy_P(Dbuf1, PSTR("%dmb"));
      sprintf(Dbuf, Dbuf1, pressuseI);

      myGLCD.print(Dbuf, 10, 220);
    }
  }
}


void DrawForecast(boolean forceRedraw)
{
  static int lastForecast;
  if ((lastForecast != forecast) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastForecast = forecast;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      myGLCD.print(weather[forecast], 122, 220);
    }
  }
}

void DrawModeButton(boolean forceDraw)
{
  DrawModeButton(220, 10, forceDraw);
}
void DrawModeButton(int x, int y, boolean forceDraw)
{
  if (screen == SCREEN_MAIN)
  {
    static float oldtemperatureTarget;

    if ((oldtemperatureTarget != temperatureTarget) || forceDraw)
    {

      if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
      {
        if ((configuration.heaterMode == HEATER_MODE_OFF) || (temperatureTarget == TMPERATURE_NO_TARGET))
          CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, String(temperature).c_str());
        else
          CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_TARGET, String(temperatureTarget).c_str());
        OutgoingData.command = CMD_SET_PARAMETER_VALUE;
        sendData();
        //recDelay(800);
      }

      oldtemperatureTarget = temperatureTarget;

      myGLCD.setBackColor(0, 255, 0);
      myGLCD.setColor(0, 255, 0);
      myGLCD.fillRoundRect (x, y, x + 90, y + 45);

      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect  (x, y, x + 90, y + 45);
      myGLCD.setColor(0, 0, 0);
      myGLCD.setFont(arial_normal);
      if (configuration.heaterMode == HEATER_MODE_OFF)
      {
        myGLCD.print("OFF", x + 5, y + 5);
        myGLCD.setColor(0, 255, 0);
        myGLCD.fillRect  (x + 1, y + 25, x + 80, y + 20);
      }
      else if (configuration.heaterMode == HEATER_MODE_AUTO)
      {
        myGLCD.print("AUTO", x + 5, y + 5);
        if (temperatureTarget != TMPERATURE_NO_TARGET)
          myGLCD.printNumF(temperatureTarget, 1, x + 1, y + 25, 46, 5);
      }
      else if (configuration.heaterMode == HEATER_MODE_LOLO)
      {
        myGLCD.print("LOLO", x + 5, y + 5);
        myGLCD.printNumF(temperatureTarget, 1, x + 1, y + 25, 46, 5);
      }
      else if (configuration.heaterMode == HEATER_MODE_LO)
      {
        myGLCD.print("LOW", x + 5, y + 5);
        myGLCD.printNumF(temperatureTarget, 1, x + 1, y + 25, 46, 5);
      }
      else if (configuration.heaterMode == HEATER_MODE_HI)
      {
        myGLCD.print("HIGH", x + 5, y + 5);
        myGLCD.printNumF(temperatureTarget, 1, x + 1, y + 25, 46, 5);
      }
      else if (configuration.heaterMode == HEATER_MODE_HIHI)
      {
        myGLCD.print("HIHI", x + 5, y + 5);
        myGLCD.printNumF(temperatureTarget, 1, x + 1, y + 25, 46, 5);
      }
    }
  }
}

void DrawSettingsButton(int x, int y)
{
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (x, y, x + 90, y + 45);

  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect  (x, y, x + 90, y + 45);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setFont(arial_normal);
  myGLCD.print("Sett", x + 5, y + 15);
}


void DrawHeaterStatus(int x, int y)
{
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);

  myGLCD.setFont(arial_normal);
  myGLCD.print("Indoor", x, y);

  //myGLCD.print("OFF", x+150, y);
}
void DrawCurrentTemp(int x, int y, float temp, float hum)
{
  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(arial_normal);

    int tmp1 = trunc(temp);
    int tmp2 = trunc(temp * 10 - tmp1 * 10);
    float tmp3 = temp * 100 - tmp1 * 100  - tmp2 * 10;

    if (tmp3 > 5)
    {
      tmp2++;

      if (tmp2 > 9)
      {
        tmp1++;
        tmp2 = 0;
      }
    }

    myGLCD.setFont(ArialNumFontPlus);
    myGLCD.printNumI(tmp1, x + 20, y);
    myGLCD.printNumI(tmp2, x + 100, y);
    //myGLCD.setFont(arial_normal);
    //myGLCD.print("o", x+130, y);
    myGLCD.fillCircle(x + 137, y + 4, 4);
    y += 36;
    //myGLCD.print("o", x+85, y);
    myGLCD.fillCircle(x + 92, y + 7, 4);

    myGLCD.setFont(arial_normal);
    y += 24;
    myGLCD.print("Hum.:", x, y);
    tmp1 = round(hum);
    myGLCD.printNumI(tmp1, x + 100, y);
    myGLCD.print("%", x + 140, y);
  }
}

void DrawTime(int x, int y, boolean force)
{
  static time_t tLast;
  //time_t utc = now();
  //time_t t = tzc.toLocal(t);
  time_t t = timelocal(now());




  if (minute(t) != minute(tLast))
  {
    UpdateSetTemp();
    DrawModeButton(220, 10, false);
    UpdateHeaterStatus();
  }
  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(SevenSegNumFontPlus);

    clock = !clock;
    if (clock)
      myGLCD.print(":", x + 2 * 32, y);
    else
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect (x + 2 * 32, y, x + 3 * 32 - 1, y + 50);
    }

    if ((minute(t) != minute(tLast)) || force)
    {
      tLast = t;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);

      strcpy_P(Dbuf1, PSTR("%02d"));
      sprintf(Dbuf, Dbuf1, hour(t));

      myGLCD.print(Dbuf, x, y);
      sprintf(Dbuf, Dbuf1, minute(t));
      myGLCD.print(Dbuf, x + 3 * 32, y);

      y += 50;

      myGLCD.setFont(arial_normal);

      strcpy_P(Dbuf1, PSTR("%s %02d.%02d.%04d"));
      sprintf(Dbuf, Dbuf1, dayShortStr(weekday(t)), day(t), month(t), year(t));

      myGLCD.print(Dbuf, x, y);
      //myGLCD.print("MO 18.2.2015", x, y);
    }
  }
}


void ScreenMainButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    Serial.println("touch");

    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();

    int xpos = 220;
    int ypos = 10;

    if ((x > xpos) && (x < (xpos + 90)) && (y > 10) && (y < (ypos + 45)))
    {
      configuration.heaterMode += 1;
      configuration.heaterMode = (configuration.heaterMode > HEATER_MODE_HIHI) ? HEATER_MODE_OFF : configuration.heaterMode;
      SaveConfiguration();

      DrawButtonPressed(xpos, ypos, xpos + 90, ypos + 45);
      UpdateSetTemp();
      DrawModeButton(xpos, ypos, true);
      UpdateHeaterStatus();
      SendModeStatus();
      WaitTouch();
    }
    else
    {
      xpos = 220;
      ypos = 70;

      if ((x > xpos) && (x < (xpos + 90)) && (y > 10) && (y < (ypos + 45)))
      {
        DrawButtonPressed(xpos, ypos, xpos + 90, ypos + 45);
        WaitTouch();
        DrawScreenSettings();
      }
    }
  }
}

/////////////////////////////////////
// end Main screen
/////////////////////////////////////


/////////////////////////////////////
// begin temperature screen
/////////////////////////////////////
void DrawScreenTemperature()
{
  screen = SCREEN_TEMPWERATURE;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);

  int x = 20;
  int y = 10;

  int s = 50;

  myGLCD.print("LOLO:", x, y);

  myGLCD.print("LOW:",  x, y + s);

  myGLCD.print("HIGH:", x, y  + s * 2);

  myGLCD.print("HIHI:", x, y  + s * 3);

  DrawTeemperatureTemp();

  int t = 45;
  int u = 250;
  int k;

  for (k = 0; k < 4; k++)
  {
    u = 250;
    myGLCD.setBackColor(0, 255, 0);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (u, y + k * t, u + t, (k + 1)*t);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (u, y + k * t, u + t, (k + 1)*t);
    myGLCD.setColor(0, 0, 0);
    myGLCD.print("+", u + 15, y + k * t + 10);

    u = 200;
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (u, y + k * t, u + t, (k + 1)*t);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (u, y + k * t, u + t, (k + 1)*t);
    myGLCD.setColor(0, 0, 0);
    myGLCD.print("-", u + 15, y + k * t + 10);
  }

  u = 200;
  k = 4;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (u, y + k * t, u + t * 2, (k + 1)*t);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (u, y + k * t, u + t * 2, (k + 1)*t);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", u + 15, y + k * t + 10);
}

void DrawTeemperatureTemp()
{
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);
  int x = 20;
  int y = 10;
  int s = 50;

  DrawScreenTemperatureTemp(x + 75, y, configuration.temp_lolo);
  DrawScreenTemperatureTemp(x + 75, y + s, configuration.temp_lo);
  DrawScreenTemperatureTemp(x + 75, y  + s * 2, configuration.temp_hi);
  DrawScreenTemperatureTemp(x + 75, y  + s * 3, configuration.temp_hihi);
}

void DrawScreenTemperatureTemp(int x, int y, float temp)
{
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setFont(arial_normal);

  myGLCD.printNumF(temp, 1, x, y, 46, 5); // Print Floating Number   1 decimal places, x=20, y=170, 46 = ".", 8 characters, 48= "0" (filler)
}

// sceen temperature button handler
void ScreenTemperatureButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();

    // exit button

    int xpos = 20;
    int ypos = 10;

    int s = 50;
    int t = 45;
    int u = 250;
    int k;

    u = 200;
    k = 4;

    // back button
    if ((x > u) && (x < (u + t * 2)) && (y > (ypos + k * t)) && (y < ((k + 1)*t)))
    {
      DrawButtonPressed(u, ypos + k * t, u + t * 2, (k + 1)*t);
      WaitTouch();
      SaveConfiguration();
      DrawScreenSettings();
    }

    // increase decrease temp button handler
    for (k = 0; k < 4; k++)
    {
      // increase
      u = 250;
      if ((x > u) && (x < (u + t)) && (y > (ypos + k * t)) && (y < ((k + 1)*t)))
      {
        if (k == 0)
        {
          configuration.temp_lolo += TEMP_STEP;
          configuration.temp_lolo = (configuration.temp_lolo > configuration.temp_lo) ? configuration.temp_lo : configuration.temp_lolo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LOLO, String(configuration.temp_lolo).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 1)
        {
          configuration.temp_lo += TEMP_STEP;
          configuration.temp_lo = (configuration.temp_lo > configuration.temp_hi) ? configuration.temp_hi : configuration.temp_lo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempLo.set(configuration.temp_lo, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LO, String(configuration.temp_lo).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 2)
        {
          configuration.temp_hi += TEMP_STEP;
          configuration.temp_hi = (configuration.temp_hi > configuration.temp_hihi) ? configuration.temp_hihi : configuration.temp_hi;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempHi.set(configuration.temp_hi, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HI, String(configuration.temp_hi).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 3)
        {
          configuration.temp_hihi += TEMP_STEP;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HIHI, String(configuration.temp_hihi).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }

        DrawTeemperatureTemp();

        DrawButtonPressed(u, ypos + k * t, u + t, (k + 1)*t);
        WaitTouch();
      }

      // decrease
      u = 200;
      if ((x > u) && (x < (u + t)) && (y > (ypos + k * t)) && (y < ((k + 1)*t)))
      {
        if (k == 0)
        {
          configuration.temp_lolo -= TEMP_STEP;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LOLO, String(configuration.temp_lolo).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 1)
        {
          configuration.temp_lo -= TEMP_STEP;
          configuration.temp_lo = (configuration.temp_lo < configuration.temp_lolo) ? configuration.temp_lolo : configuration.temp_lo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempLo.set(configuration.temp_lo, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_LO, String(configuration.temp_lo).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 2)
        {
          configuration.temp_hi -= TEMP_STEP;
          configuration.temp_hi = (configuration.temp_hi < configuration.temp_lo) ? configuration.temp_lo : configuration.temp_hi;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempHi.set(configuration.temp_hi, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HI, String(configuration.temp_hi).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }
        else if (k == 3)
        {
          configuration.temp_hihi -= TEMP_STEP;
          configuration.temp_hihi = (configuration.temp_hihi < configuration.temp_hi) ? configuration.temp_hi : configuration.temp_hihi;
          UpdateSetTemp();
          UpdateHeaterStatus();
          // esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
          if (comm_state == COMM_IDLE || comm_state == COMM_WAIT_PONG)
          {
            CopyValuesToOutgoing(OutgoingData.data, PARAM_TEMP_HIHI, String(configuration.temp_hihi).c_str());
            OutgoingData.command = CMD_SET_PARAMETER_VALUE;
            sendData();
            delay(300);
          }
        }

        DrawTeemperatureTemp();

        DrawButtonPressed(u, ypos + k * t, u + t, (k + 1)*t);
        WaitTouch();
      }
    }

  }
}
/////////////////////////////////////
// end temperature screen
/////////////////////////////////////


/////////////////////////////////////
// begin settings screen
/////////////////////////////////////

void DrawScreenSettings()
{
  screen = SCREEN_SETTINGS;
  _rec = 0;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);

  int xpos = 20;
  int ypos = 15;
  int s = 35;
  int xsize = 280;


  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("WiFi/Cloud", xpos + 20, ypos + 10);

  ypos += s + 10;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Date/Time", xpos + 20, ypos + 10);

  ypos += s + 10;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Temperature", xpos + 20, ypos + 10);

  ypos += s + 10;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Schedule", xpos + 20, ypos + 10);

  ypos += s + 10;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", xpos + 20, ypos + 10);
}


void ScreenSettingsButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();

    int xpos = 20;
    int ypos = 15;
    int ysize = 35;
    int xsize = 280;

    if ((x > xpos) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      //DrawScreenDatetime();
      DrawScreenWiFiSettings();
    }

    ypos += ysize + 10;
    // date/time
    if ((x > xpos) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      DrawScreenDatetime();
    }

    ypos += ysize + 10;
    // temperature
    if ((x > xpos) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      DrawScreenTemperature();
    }

    ypos += ysize + 10;
    // schedule
    if ((x > xpos) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      DrawScreenSchedule();
    }

    ypos += ysize + 10;
    // back
    if ((x > xpos) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      DrawScreenMain();
    }
  }
}

/////////////////////////////////////
// end settings screen
/////////////////////////////////////






/////////////////////////////////////
// begin wifi settings screen
/////////////////////////////////////

void drawWiFiTxt(char * txt, int xpos, int ypos)
{
  ypos = ypos + 3;
  char tmp[DATA_LEN + 1];
  int len;
  int xsize = 285;

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect (xpos, ypos + 15 + 1, xpos + xsize, ypos + 2 * 15);


  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  strcpy(tmp, txt);
  while ((len = strlen(tmp)) > 18)
  {
    char* tmp1;
    tmp1 = tmp;
    while (*tmp1 != 0)
    {
      *tmp1 = *(tmp1 + 1);
      tmp1++;
    }
  }
  myGLCD.print(tmp, xpos, ypos + 15);
}

void DrawScreenWiFiSettings()
{
  char tmp[DATA_LEN + 1];
  int len;

  screen = SCREEN_WIFI_SETTINGS;
  _rec = 0;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);

  int xpos = 20;
  int ypos = 15;
  int s = 35;
  int xsize = 280;
  int btnsize = 15;

  // btn
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("D", xpos + xsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("E", xpos + xsize - 2 * btnsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("+", xpos + xsize - 3 * btnsize - 2 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + xsize - 4 * btnsize - 3 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);


  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("AP SSID", xpos, ypos);

  drawWiFiTxt(configuration.ap_ssid, xpos, ypos);

  ypos += s + 3;

  // btn
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("D", xpos + xsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("E", xpos + xsize - 2 * btnsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("+", xpos + xsize - 3 * btnsize - 2 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + xsize - 4 * btnsize - 3 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 0);
  //myGLCD.fillRoundRect (xpos, ypos, xpos+100, ypos + s-20);
  myGLCD.print("AP pass", xpos, ypos);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  //myGLCD.print("danesjelepdan", xpos, ypos+15);
  drawWiFiTxt(configuration.ap_password, xpos, ypos);

  ypos += s + 3;

  // btn
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("D", xpos + xsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("E", xpos + xsize - 2 * btnsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("+", xpos + xsize - 3 * btnsize - 2 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + xsize - 4 * btnsize - 3 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);


  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 0);
  //myGLCD.fillRoundRect (xpos, ypos, xpos+100, ypos + s-20);
  myGLCD.print("Cloud host", xpos, ypos);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  //myGLCD.print("danesjelepdan", xpos, ypos+15);
  drawWiFiTxt(configuration.host, xpos, ypos);

  ypos += s + 3;

  // btn
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("D", xpos + xsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("E", xpos + xsize - 2 * btnsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("+", xpos + xsize - 3 * btnsize - 2 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + xsize - 4 * btnsize - 3 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Cloud user", xpos, ypos);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  //myGLCD.print("danesjelepdan", xpos, ypos+15);
  drawWiFiTxt(configuration.cloud_username, xpos, ypos);

  ypos += s + 3;

  // btn
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("D", xpos + xsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - btnsize, ypos, xpos + xsize - 0 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("E", xpos + xsize - 2 * btnsize - btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 2 * btnsize - btnsize, ypos, xpos + xsize - 1 * btnsize - btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("+", xpos + xsize - 3 * btnsize - 2 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 3 * btnsize - 2 * btnsize, ypos, xpos + xsize - 2 * btnsize - 2 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + xsize - 4 * btnsize - 3 * btnsize, ypos);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - 4 * btnsize - 3 * btnsize, ypos, xpos + xsize - 3 * btnsize - 3 * btnsize, ypos + btnsize);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Cloud pass", xpos, ypos);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  //myGLCD.print("danesjelepdan", xpos, ypos+15);
  drawWiFiTxt(configuration.cloud_password, xpos, ypos);


  ypos += s + 3;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + 130, ypos + btnsize * 2);
  myGLCD.setColor(0, 0, 0);
  String btntxt = "Mod:" + String(configuration.moduleId);
  myGLCD.print(btntxt.c_str(), xpos + 10, ypos + 7);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + 130, ypos + btnsize * 2);

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + 130 + 20, ypos, xpos + xsize, ypos + btnsize * 2);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", xpos + 130 + 20 + 10, ypos + 7);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + 130 + 20, ypos, xpos + xsize, ypos + btnsize * 2);


}


void ScreenWiFiSettingsButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();

    int xpos = 20;
    int ypos = 15;
    int ysize = 35;
    int xsize = 280;
    int btnsize = 15;
    int len;

    // SSID D
    int xbtnpos = xpos + xsize - btnsize;
    int ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.ap_ssid);
      if (len > 1)
      {
        configuration.ap_ssid[len - 1] = 0;
        drawWiFiTxt(configuration.ap_ssid, xpos, ypos);
      }
    }

    // SSID E
    xbtnpos = xpos + xsize - 2 * btnsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.ap_ssid);
      if (len < 30)
      {
        configuration.ap_ssid[len] = 'A';
        configuration.ap_ssid[len + 1] = 0;
        //DrawScreenWiFiSettings();
        drawWiFiTxt(configuration.ap_ssid, xpos, ypos);
      }
    }

    // SSID +
    xbtnpos = xpos + xsize - 3 * btnsize - 2 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.ap_ssid);
      if (++configuration.ap_ssid[len - 1] > 126)
        configuration.ap_ssid[len - 1] = 32;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.ap_ssid, xpos, ypos);
    }
    // SSID -
    xbtnpos = xpos + xsize - 4 * btnsize - 3 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.ap_ssid);
      if (--configuration.ap_ssid[len - 1] < 32)
        configuration.ap_ssid[len - 1] = 126;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.ap_ssid, xpos, ypos);
    }

    // AP password
    ypos += ysize + 3;

    // AP password D
    xbtnpos = xpos + xsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.ap_password);
      if (len > 1)
      {
        configuration.ap_password[len - 1] = 0;
        drawWiFiTxt(configuration.ap_password, xpos, ypos);
      }
    }

    // AP password E
    xbtnpos = xpos + xsize - 2 * btnsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.ap_password);
      if (len < 30)
      {
        configuration.ap_password[len] = 'A';
        configuration.ap_password[len + 1] = 0;
        //DrawScreenWiFiSettings();
        drawWiFiTxt(configuration.ap_password, xpos, ypos);
      }
    }

    // AP password +
    xbtnpos = xpos + xsize - 3 * btnsize - 2 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.ap_password);
      if (++configuration.ap_password[len - 1] > 126)
        configuration.ap_password[len - 1] = 32;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.ap_password, xpos, ypos);
    }
    // AP password -
    xbtnpos = xpos + xsize - 4 * btnsize - 3 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.ap_password);
      if (--configuration.ap_password[len - 1] < 32)
        configuration.ap_password[len - 1] = 126;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.ap_password, xpos, ypos);
    }

    // host
    ypos += ysize + 3;

    // host D
    xbtnpos = xpos + xsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.host);
      if (len > 1)
      {
        configuration.host[len - 1] = 0;
        drawWiFiTxt(configuration.host, xpos, ypos);
      }
    }

    // host E
    xbtnpos = xpos + xsize - 2 * btnsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.host);
      if (len < 30)
      {
        configuration.host[len] = 'A';
        configuration.host[len + 1] = 0;
        //DrawScreenWiFiSettings();
        drawWiFiTxt(configuration.host, xpos, ypos);
      }
    }

    // host +
    xbtnpos = xpos + xsize - 3 * btnsize - 2 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.host);
      if (++configuration.host[len - 1] > 126)
        configuration.host[len - 1] = 32;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.host, xpos, ypos);
    }
    // host -
    xbtnpos = xpos + xsize - 4 * btnsize - 3 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.host);
      if (--configuration.host[len - 1] < 32)
        configuration.host[len - 1] = 126;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.host, xpos, ypos);
    }



    // cloud_username
    ypos += ysize + 3;

    // host D
    xbtnpos = xpos + xsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.cloud_username);
      if (len > 1)
      {
        configuration.cloud_username[len - 1] = 0;
        drawWiFiTxt(configuration.cloud_username, xpos, ypos);
      }
    }

    // cloud_username E
    xbtnpos = xpos + xsize - 2 * btnsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.cloud_username);
      if (len < 30)
      {
        configuration.cloud_username[len] = 'A';
        configuration.cloud_username[len + 1] = 0;
        //DrawScreenWiFiSettings();
        drawWiFiTxt(configuration.cloud_username, xpos, ypos);
      }
    }

    // cloud_username +
    xbtnpos = xpos + xsize - 3 * btnsize - 2 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.cloud_username);
      if (++configuration.cloud_username[len - 1] > 126)
        configuration.cloud_username[len - 1] = 32;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.cloud_username, xpos, ypos);
    }
    // cloud_username -
    xbtnpos = xpos + xsize - 4 * btnsize - 3 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.cloud_username);
      if (--configuration.cloud_username[len - 1] < 32)
        configuration.cloud_username[len - 1] = 126;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.cloud_username, xpos, ypos);
    }


    // cloud_password
    ypos += ysize + 3;

    // host D
    xbtnpos = xpos + xsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.cloud_password);
      if (len > 1)
      {
        configuration.cloud_password[len - 1] = 0;
        drawWiFiTxt(configuration.cloud_password, xpos, ypos);
      }
    }

    // cloud_password E
    xbtnpos = xpos + xsize - 2 * btnsize - btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.cloud_password);
      if (len < 30)
      {
        configuration.cloud_password[len] = 'A';
        configuration.cloud_password[len + 1] = 0;
        //DrawScreenWiFiSettings();
        drawWiFiTxt(configuration.cloud_password, xpos, ypos);
      }
    }

    // cloud_password +
    xbtnpos = xpos + xsize - 3 * btnsize - 2 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();
      //DrawScreenDatetime();

      len = strlen(configuration.cloud_password);
      if (++configuration.cloud_password[len - 1] > 126)
        configuration.cloud_password[len - 1] = 32;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.cloud_password, xpos, ypos);
    }
    // cloud_password -
    xbtnpos = xpos + xsize - 4 * btnsize - 3 * btnsize;
    ybtnpos = ypos;

    if ((x > xbtnpos) && (x < (xbtnpos + btnsize)) && (y > ybtnpos) && (y < (ybtnpos + btnsize)))
    {
      DrawButtonPressed(xbtnpos, ybtnpos, xbtnpos + btnsize, ybtnpos + btnsize);
      WaitTouch();

      len = strlen(configuration.cloud_password);
      if (--configuration.cloud_password[len - 1] < 32)
        configuration.cloud_password[len - 1] = 126;
      //DrawScreenWiFiSettings();
      drawWiFiTxt(configuration.cloud_password, xpos, ypos);
    }

    ypos += ysize + 3;

    if ((x > xpos) && (x < (xpos + 130)) && (y > ypos) && (y < (ypos + btnsize * 2)))
    {
      configuration.moduleId = 0;
      DrawScreenWiFiSettings();
    }

    if ((x > (xpos + 130 + 20)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + btnsize * 2)))
    {
      comm_state = COMM_START;
      SaveConfiguration();

      DrawScreenSettings();
    }
  }
}

/////////////////////////////////////
// end wifi settings screen
/////////////////////////////////////





/////////////////////////////////////
// begin schedule screen
/////////////////////////////////////

void DrawScreenSchedule()
{
  drawScreenSchedule();
  drawScreenSchedule(_rec);
}

void drawScreenSchedule(int rec)
{
  myGLCD.setFont(arial_normal);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  int xpos = 0;
  int ypos = 15;
  int ysize = 30;
  int xsize = 280;

  if (configuration.schedules[_rec].enabled == true)
    myGLCD.print("On ", xpos + 120, ypos + 5);
  else
    myGLCD.print("Off", xpos + 120, ypos + 5);

  ypos += ysize + 5;

  myGLCD.print(days[configuration.schedules[_rec].days], xpos + 120, ypos + 5);

  ypos += ysize + 5;
  strcpy_P(Dbuf1, PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, configuration.schedules[_rec].hourStart, configuration.schedules[_rec].minuteStart);
  myGLCD.print(Dbuf, xpos + 120, ypos + 5);

  ypos += ysize + 5;
  //strcpy_P(Dbuf1,PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, configuration.schedules[_rec].hourStop, configuration.schedules[_rec].minuteStop);
  myGLCD.print(Dbuf, xpos + 120, ypos + 5);


  ypos += ysize + 5;
  if (configuration.schedules[_rec].temp == HEATER_MODE_LOLO)
    myGLCD.print("LOLO", xpos + 120, ypos + 5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_LO)
    myGLCD.print("LOW ", xpos + 120, ypos + 5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_HI)
    myGLCD.print("HIGH ", xpos + 120, ypos + 5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_HIHI)
    myGLCD.print("HIHI ", xpos + 120, ypos + 5);


  ypos += ysize + 5;
  myGLCD.printNumI((_rec + 1), xpos + 120, ypos + 5);
}

void drawScreenSchedule()
{
  screen = SCREEN_SCHEDULE;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);


  int xpos = 20;
  int ypos = 15;
  int ysize = 30;
  int xsize = 280;


  // enabled
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Active:", xpos - 10, ypos + 5);


  ypos += ysize + 5;

  // days
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Days:", xpos - 10, ypos + 5);

  ypos += ysize + 5;

  // satrt
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("M", xpos + xsize - ysize + 7, ypos + 7);
  myGLCD.print("H", xpos + xsize - ysize * 2 - 5 + 7, ypos + 7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Start:", xpos - 10, ypos + 5);

  ypos += ysize + 5;

  // stop
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("M", xpos + xsize - ysize + 7, ypos + 7);
  myGLCD.print("H", xpos + xsize - ysize * 2 - 5 + 7, ypos + 7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Stop:", xpos - 10, ypos + 5);

  ypos += ysize + 5;

  // temp
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Temp:", xpos - 10, ypos + 5);


  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print(">", xpos + xsize - ysize + 7, ypos + 7);
  myGLCD.print("<", xpos + xsize - ysize * 2 - 5 + 7, ypos + 7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Rec:", xpos - 10, ypos + 5);
}


void ScreenScheduleButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();


    int xpos = 20;
    int ypos = 15;
    int ysize = 30;
    int xsize = 280;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].enabled = !configuration.schedules[_rec].enabled;
      drawScreenSchedule(_rec);
      UpdateSetTemp();
      UpdateHeaterStatus();
    }


    ypos += ysize + 5;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].days = (++configuration.schedules[_rec].days > DAYS_SU) ? DAYS_ALL : configuration.schedules[_rec].days;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }


    ypos += ysize + 5;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].minuteStart = (++configuration.schedules[_rec].minuteStart > 59) ? 0 : configuration.schedules[_rec].minuteStart;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }

    if ((x > (xpos + xsize - ysize * 2 - 5)) && (x < (xpos + xsize - ysize - 5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].hourStart = (++configuration.schedules[_rec].hourStart > 23) ? 0 : configuration.schedules[_rec].hourStart;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }

    ypos += ysize + 5;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].minuteStop = (++configuration.schedules[_rec].minuteStop > 59) ? 0 : configuration.schedules[_rec].minuteStop;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }

    if ((x > (xpos + xsize - ysize * 2 - 5)) && (x < (xpos + xsize - ysize - 5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].hourStop = (++configuration.schedules[_rec].hourStop > 23) ? 0 : configuration.schedules[_rec].hourStop;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }


    ypos += ysize + 5;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].temp = (++configuration.schedules[_rec].temp > HEATER_MODE_HIHI) ? HEATER_MODE_LOLO : configuration.schedules[_rec].temp;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();
    }


    ypos += ysize + 5;

    if ((x > (xpos + xsize - ysize)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();

      if (++_rec >= MAX_SCHEDULES)
      {
        SaveConfiguration();
        DrawScreenSettings();
      }
      else
        drawScreenSchedule(_rec);
    }

    if ((x > (xpos + xsize - ysize * 2 - 5)) && (x < (xpos + xsize - ysize - 5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize - ysize * 2 - 5, ypos, xpos + xsize - ysize - 5, ypos + ysize);
      WaitTouch();
      if (--_rec < 0)
      {
        SaveConfiguration();
        DrawScreenSettings();
      }
      else
        drawScreenSchedule(_rec);
    }


  }
}

/////////////////////////////////////
// end schedule screen
/////////////////////////////////////


/////////////////////////////////////
// begin datetime screen
/////////////////////////////////////


void DrawScreenDatetime()
{
  time_t t = timelocal(now());

  screen = SCREEN_SET_DATETIME;

  tmpHour = hour(t);
  tmpMinute = minute(t);

  tmpMonth = month(t);
  tmpDay = day(t);
  tmpYear = year(t);


  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);


  int xpos = 20;
  int ypos = 15;
  int ysize = 30;
  int xsize = 30;

  // time
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Time:", xpos, ypos + 5);

  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + 7, ypos + 7);
  myGLCD.print("+", xpos + xsize + 10 + 7, ypos + 7);
  myGLCD.print("-", xpos + xsize * 2 + 40 + 7, ypos + 7);
  myGLCD.print("+", xpos + xsize * 3 + 50 + 7, ypos + 7);

  ypos += ysize + 5;
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Date:", xpos, ypos + 5);

  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 4 + 80, ypos, xpos + xsize * 5 + 80, ypos + ysize);
  myGLCD.fillRoundRect (xpos + xsize * 5 + 90, ypos, xpos + xsize * 6 + 90, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 4 + 80, ypos, xpos + xsize * 5 + 80, ypos + ysize);
  myGLCD.drawRoundRect (xpos + xsize * 5 + 90, ypos, xpos + xsize * 6 + 90, ypos + ysize);

  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos + 7, ypos + 7);
  myGLCD.print("+", xpos + xsize + 10 + 7, ypos + 7);
  myGLCD.print("-", xpos + xsize * 2 + 40 + 7, ypos + 7);
  myGLCD.print("+", xpos + xsize * 3 + 50 + 7, ypos + 7);
  myGLCD.print("-", xpos + xsize * 4 + 80 + 7, ypos + 7);
  myGLCD.print("+", xpos + xsize * 5 + 90 + 7, ypos + 7);


  ypos += ysize + 40;
  xsize = 280;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos + xsize, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", xpos + 120, ypos + 7);

  DisplayDatetime();
}

void SetTime()
{
  time_t t1;
  time_t utc;
  tmElements_t tm;

  tm.Year = CalendarYrToTm(tmpYear);

  tm.Month = tmpMonth;
  tm.Day = tmpDay;
  tm.Hour = tmpHour;
  tm.Minute = tmpMinute;
  tm.Second = 0;
  t1 = makeTime(tm);

  utc = tzc.toUTC(t1);


  //use the time_t value to ensure correct weekday is set
  if (RTC.set(utc) == 0) { // Success
    setTime(utc);
  }

  time_t t = timelocal(now());

  tmpHour = hour(t);
  tmpMinute = minute(t);

  tmpMonth = month(t);
  tmpDay = day(t);
  tmpYear = year(t);
}

void ScreenDatetimeeButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x = myTouch.getX();
    int y = myTouch.getY();


    int xpos = 20;
    int ypos = 15;
    int ysize = 30;
    int xsize = 30;

    ypos += ysize + 5;

    // hour -
    if ((x > (xpos)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      tmpHour = (--tmpHour == 0xFF) ? 23 : tmpHour;
      DisplayDatetime();
    }

    // hour +
    if ((x > (xpos + xsize + 10)) && (x < (xpos + xsize * 2 + 10)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
      WaitTouch();
      tmpHour = (++tmpHour > 23) ? 0 : tmpHour;
      DisplayDatetime();
    }

    // minute -
    if ((x > (xpos + xsize * 2 + 40)) && (x < (xpos + xsize * 3 + 40)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
      WaitTouch();
      tmpMinute = (--tmpMinute == 0xFF) ? 59 : tmpMinute;
      DisplayDatetime();
    }

    // minute +
    if ((x > (xpos + xsize * 3 + 50)) && (x < (xpos + xsize * 4 + 50)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
      WaitTouch();
      tmpMinute = (++tmpMinute > 59) ? 0 : tmpMinute;
      DisplayDatetime();
    }

    ypos += ysize + 5;
    ypos += ysize + 5;


    // day -
    if ((x > (xpos)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      tmpDay = (--tmpDay < 1) ? 31 : tmpDay;
      DisplayDatetime();
    }

    // day +
    if ((x > (xpos + xsize + 10)) && (x < (xpos + xsize * 2 + 10)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize + 10, ypos, xpos + xsize * 2 + 10, ypos + ysize);
      WaitTouch();
      tmpDay = (++tmpDay > 31) ? 1 : tmpDay;
      DisplayDatetime();
    }


    // month -
    if ((x > (xpos + xsize * 2 + 40)) && (x < (xpos + xsize * 3 + 40)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 2 + 40, ypos, xpos + xsize * 3 + 40, ypos + ysize);
      WaitTouch();
      tmpMonth = (--tmpMonth < 1) ? 12 : tmpMonth;
      DisplayDatetime();
    }

    // month +
    if ((x > (xpos + xsize * 3 + 50)) && (x < (xpos + xsize * 4 + 50)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 3 + 50, ypos, xpos + xsize * 4 + 50, ypos + ysize);
      WaitTouch();
      tmpMonth = (++tmpMonth > 12) ? 1 : tmpMonth;
      DisplayDatetime();
    }

    // year -
    if ((x > (xpos + xsize * 4 + 80)) && (x < (xpos + xsize * 5 + 80)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 4 + 80, ypos, xpos + xsize * 5 + 80, ypos + ysize);
      WaitTouch();
      tmpYear = (--tmpYear < 2000) ? 2014 : tmpYear;
      DisplayDatetime();
    }

    // year +
    if ((x > (xpos + xsize * 5 + 90)) && (x < (xpos + xsize * 6 + 90)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos + xsize * 5 + 90, ypos, xpos + xsize * 6 + 90, ypos + ysize);
      WaitTouch();
      ++tmpYear;
      //tmpYear = (++tmpYear > 12)?1:tmpYear;
      DisplayDatetime();
    }

    ypos = 190;
    xsize = 280;

    if ((x > (xpos)) && (x < (xpos + xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos + xsize, ypos + ysize);
      WaitTouch();
      DrawScreenSettings();
    }
  }
}

void DisplayDatetime()
{
  SetTime();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  strcpy_P(Dbuf1, PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, tmpHour, tmpMinute);
  myGLCD.print(Dbuf, 110, 20);

  strcpy_P(Dbuf1, PSTR("%02d.%02d.%04d"));
  sprintf(Dbuf, Dbuf1, tmpDay, tmpMonth, tmpYear);
  myGLCD.print(Dbuf, 110, 90);
}

/////////////////////////////////////
// end datetime screen
/////////////////////////////////////

