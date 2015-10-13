/******************************************************************
  DHT Temperature & Humidity Sensor library for Arduino.

  Features:
  - Support for DHT11 and DHT22/AM2302/RHT03
  - Auto detect sensor model
  - Very low memory footprint
  - Very small code

  http://www.github.com/markruys/arduino-DHT

  Written by Mark Ruys, mark@paracas.nl.

  Update: Igor Jarc
  This library is ported to Arduino ESP8266 IDE.
  http://iot-playground.com/2-uncategorised/70-esp8266-wifi-dht22-humidity-sensor-easyiot-cloud-rest-api
  https://github.com/iot-playground/EasyIoT-Cloud

  BSD license, check license.txt for more information.
  All text above must be included in any redistribution.


  Datasheets:
  - http://www.micro4you.com/files/sensor/DHT11.pdf
  - http://www.adafruit.com/datasheets/DHT22.pdf
  - http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Weather/RHT03.pdf
  - http://meteobox.tk/files/AM2302.pdf

  Changelog:
   2013-06-10: Initial version
   2013-06-12: Refactored code
   2013-07-01: Add a resetTimer method
   2015-10-13: Ported to Arduino ESP8266 IDE Igor Jarc http://iot-playground.com
 ******************************************************************/

#include "DHT.h"

void DHT::setup(uint8_t pin, DHT_MODEL_t model)
{
  DHT::pin = pin;
  DHT::model = model;
  DHT::resetTimer(); // Make sure we do read the sensor in the next readSensor()

  if ( model == AUTO_DETECT) {
    DHT::model = DHT22;
    readSensor();
    if ( error == ERROR_TIMEOUT ) {
      DHT::model = DHT11;
      // Warning: in case we auto detect a DHT11, you should wait at least 1000 msec
      // before your first read request. Otherwise you will get a time out error.
    }
  }
}

void DHT::resetTimer()
{
  DHT::lastReadTime = millis() - 3000;
}

float DHT::getHumidity()
{
  readSensor();
  return humidity;
}

float DHT::getTemperature()
{
  readSensor();
  return temperature;
}

#ifndef OPTIMIZE_SRAM_SIZE

const char* DHT::getStatusString()
{
  switch ( error ) {
    case DHT::ERROR_TIMEOUT:
      return "TIMEOUT";

    case DHT::ERROR_CHECKSUM:
      return "CHECKSUM";

    default:
      return "OK";
  }
}

#else

// At the expense of 26 bytes of extra PROGMEM, we save 11 bytes of
// SRAM by using the following method:

prog_char P_OK[]       PROGMEM = "OK";
prog_char P_TIMEOUT[]  PROGMEM = "TIMEOUT";
prog_char P_CHECKSUM[] PROGMEM = "CHECKSUM";

const char *DHT::getStatusString() {
  prog_char *c;
  switch ( error ) {
    case DHT::ERROR_CHECKSUM:
      c = P_CHECKSUM; break;

    case DHT::ERROR_TIMEOUT:
      c = P_TIMEOUT; break;

    default:
      c = P_OK; break;
  }

  static char buffer[9];
  strcpy_P(buffer, c);

  return buffer;
}

#endif

uint32_t DHT::detectState(bool state)
{
	uint32_t count = 0;
	while(digitalRead(pin) == state)
	{
		if(count++ > 80000)
			return 0;
	}
	return count;
}

void DHT::readSensor()
{
  // Make sure we don't poll the sensor too often
  // - Max sample rate DHT11 is 1 Hz   (duty cicle 1000 ms)
  // - Max sample rate DHT22 is 0.5 Hz (duty cicle 2000 ms)
  unsigned long startTime = millis();
  if ( (unsigned long)(startTime - lastReadTime) < (model == DHT11 ? 999L : 1999L) ) {
    return;
  }
  lastReadTime = startTime;

  temperature = NAN;
  humidity = NAN;
  
  digitalWrite(pin, LOW); // Send start signal
  pinMode(pin, OUTPUT);
  if ( model == DHT11 ) {
    delay(18);
  }
  else {
    // This will fail for a DHT11 - that's how we can detect such a device
    delayMicroseconds(800);
  }
	
  noInterrupts();  
	
  digitalWrite(pin, HIGH); // Switch bus to receive data
  delayMicroseconds(40); 
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(10);		
  
  if (detectState(LOW) == 0)
  {
	interrupts();  
	error = ERROR_TIMEOUT;
	Serial.println("t1");
	return;
  }

  if (detectState(HIGH) == 0)
  {
	interrupts();
	error = ERROR_TIMEOUT;
	Serial.println("t2");
	return;
  }
  
  uint32_t cnt[80];
  
  for (int i=0; i<80; i+=2) {
      cnt[i]   = detectState(LOW);
      cnt[i+1] = detectState(HIGH);
  }
  
  interrupts();
  
  
  
  
  uint8_t data[5];
  
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;
   
  for (int i=0; i<40; ++i) {
	uint32_t lowCnt  = cnt[2*i];
    uint32_t highCnt = cnt[2*i+1];
	
	if ((lowCnt == 0) || (highCnt == 0))
	{
		error = ERROR_TIMEOUT;
		return;		
	}
		
	data[i/8] <<= 1;
	if (highCnt > lowCnt) 
		data[i/8] |= 1;	
  }
  
  if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
	error = ERROR_CHECKSUM;
	return;
  }
    
  // We're going to read 83 edges:
  // - First a FALLING, RISING, and FALLING edge for the start bit
  // - Then 40 bits: RISING and then a FALLING edge per bit
  // To keep our code simple, we accept any HIGH or LOW reading if it's max 85 usecs long

  word rawHumidity;
  word rawTemperature;
  
  rawHumidity = data[0] * 0xFF + data[1] ;
  rawTemperature = data[2] * 0xFF + data[3] ;
 
  // Store readings
  if ( model == DHT11 ) {
    humidity = rawHumidity >> 8;
    temperature = rawTemperature >> 8;
  }
  else {
    humidity = rawHumidity * 0.1;

    if ( rawTemperature & 0x8000 ) {
      rawTemperature = -(int16_t)(rawTemperature & 0x7FFF);
    }
    temperature = ((int16_t)rawTemperature) * 0.1;
  }

  error = ERROR_NONE;
}
