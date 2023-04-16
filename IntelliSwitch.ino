/**
IntelliSwitch
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include "WlanData.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <WiFiUdp.h>
#include <time.h>                   // time() ctime()

#define MY_NTP_SERVER "at.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

ESP8266WiFiMulti WiFiMulti;


void getTime(tm* p_tm);
void getAvState(void);
void getSunData(int* p_hour, int* p_min);
void convertStringToTime(const char* string, int* h, int* min);
bool isAfterSunset (tm* tm);                


/*******************************************************************/
/*******************************************************************/
void setup() {
  Serial.begin(2000000);
  // Serial.setDebugOutput(true);
  Serial.println();
  Serial.println();
  Serial.println();
  configTime(MY_TZ, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WLANNAME, WLANPASSWORD);
  delay(5000);  
}


/*******************************************************************/
/*******************************************************************/
void loop() {
  tm esptime, sunSet, mytime;
  static uint64_t counter = 0;
  getTime(&esptime);

  // check if time is valid
  if (esptime.tm_year < 2001) {
    if (esptime.tm_mday == sunSet.tm_mday) {
      getSunData(&sunSet.tm_hour,&sunSet.tm_min);
    }
  }


  // wait for WiFi connection
  wl_status_t wlstate;
  wlstate = WiFiMulti.run();

  if (( wlstate == WL_CONNECTED)) {    
    if (counter%60 == 0) {
      if (isAfterSunset(&sunSet)){
        Serial.println("Licht An");
      }
      else {
        Serial.println("Licht Aus");
      }
      getTime(&mytime);
      Serial.print(mytime.tm_hour);
      Serial.print(":");      
      Serial.println(mytime.tm_min);
      getAvState();
    }    
    counter++;

  //Serial.printf("state: ; %d\n",wlstate);
  delay(1000);
  }
}

/*******************************************************************/
/*******************************************************************/
bool isAfterSunset(tm* tm) {
  bool retval = false;
  bool sunsetIsValid = false;
  int sunsetHour = 0;
  int sunsetMin = 0;

  // get sunset date every day at 1:00
  if (sunsetIsValid == false) {
    if (tm->tm_hour >= 1) {
      getSunData(&sunsetHour, &sunsetMin);
      if ((sunsetHour > 16) && (sunsetHour < 23)) {
        sunsetIsValid = true;
      }
    }
  }


  if (tm->tm_hour >= sunsetHour) {
    retval = true;
  }
  return (false);
}


/*******************************************************************/
/*******************************************************************/
void getTime(tm* p_tm) {
time_t now;                         // this is the epoch

  time(&now);                       // read the current time
  localtime_r(&now, p_tm);           // update the structure tm with the current time
/*
  Serial.print(p_tm->tm_mday);         // day of month
  Serial.print(".");
  Serial.print(p_tm->tm_mon + 1);      // January = 0 (!)
  Serial.print(".");
  Serial.print(p_tm->tm_year + 1900);  // years since 1900
  Serial.print(" ");
  Serial.print(p_tm->tm_hour);         // hours since midnight  0-23
  Serial.print(":");
  Serial.print(p_tm->tm_min);          // minutes after the hour  0-59
  Serial.print(":");
  Serial.print(p_tm->tm_sec);          // seconds after the minute  0-61*
  Serial.print(" wday");
  Serial.print(p_tm->tm_wday);         // days since Sunday 0-6
  if (p_tm->tm_isdst == 1)             // Daylight Saving Time flag
    Serial.print(" DST");
  else
    Serial.print(" standard");
  Serial.println();
  */
}

/*******************************************************************/
/*******************************************************************/
void getAvState(void) {
  WiFiClient client;
  HTTPClient http;

  //Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://192.168.0.2//goform/formMainZone_MainZoneXml.xml")) {  // HTTP

    //Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        if ((payload.indexOf("<ZonePower><value>ON") != -1) && (payload.indexOf("<InputFuncSelect><value>SAT") != -1) ){
          Serial.println("Fernsehen Ein");            
        } else {
          Serial.println("Fernsehen Aus");            
        }
      }
    }
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } 
  else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}


/*******************************************************************/
/*******************************************************************/
void getSunData(int* p_hour, int* p_min) {
  WiFiClient client;
  HTTPClient http;
  DeserializationError jsonError;
  StaticJsonDocument<1000> doc;

  if (http.begin(client, "http://api.sunrise-sunset.org/json?lat=47.5596&lng=7.5886&date=today")) {  // HTTP
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        //Serial.println(payload);
        jsonError = deserializeJson(doc, payload);
        const char* sunsetToday = doc["results"]["sunset"];
        //Serial.println(jsonError.f_str());
        //serializeJsonPretty(doc, Serial);
        //Serial.println(sunsetToday);
        convertStringToTime(sunsetToday, p_hour, p_min);
      }
    } 
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  else {
    Serial.printf("[HTTP] Unable to connect\n");
  }
}


/*******************************************************************/
/*******************************************************************/
void convertStringToTime(const char* string, int* p_h, int* p_min) {
  //Serial.print("Sunset: ");
  //Serial.println(string);
  if (string[1] == ':') {
    if (string[8] == 'P') {
      *p_h = (string[0]-0x30+12);
    }
    else {
      *p_h = (string[0]-0x30);
    }
    *p_min = (string[2]-0x30)*10+(string[3]-0x30);
  }
  else {
    *p_h = (string[0]-0x30)*10;
    *p_h += (string[1]-0x30);
    if (strncmp(&string[9],"PM",2)) {
      *p_h+=12;
    }
    *p_min = (string[3]-0x30)*10+(string[4]-0x30);
  }
  Serial.printf("%d:%d\n",*p_h,*p_min);  
}
