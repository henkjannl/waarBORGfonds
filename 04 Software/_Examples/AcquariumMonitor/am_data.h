#ifndef DATA_H
#define DATA_H

#include "FS.h"
#include "SPIFFS.h"
#include <list>
#include <ArduinoJson.h>

using namespace std;


// ======== CONSTANTS ================
const char *CONFIG_FILE = "/config.jsn";

#define RECONNECT_INTERVAL 40   // Do a reconnect attempt every 40 seconds if wifi is lost

// ======== DATA TYPES ============= 
enum tScreen { scMain, scGraph, scMessage, scMessageSent, scSensorMalfunction };

enum tSwitchEvent { seStart, seCompressorOn, seCompressorOff, seAlarm, seSensorMalfunction };

enum tSendMessageRequest { mrIdle, mrRequestMessage, mrMessageSent };

struct tDataPoint {
  unsigned long secondsSinceStart;
  float mina, maxa;
  float minp, maxp;
  float mind, maxd;
};

struct tTransition {
  long secondsSinceStart;
  char time[16];  
  tSwitchEvent state;
};

class tData {
  public:

    // Check or update connection to the internet
    bool connected;
    bool timeSynched;
    
    // Manage which screen is displayed, and if we are asleep 
    tScreen screen;
    bool awake;
    int sleepCountdown;
    int reconnectCountdown;

    // Count down how long ago the test message was sent, so the screen can return to main
    int messageSentCountDown;

    // Messages sent by Telegram
    tSendMessageRequest sendSensorMalfunctionMessage;
    tSendMessageRequest sendAlarmMessage;
    tSendMessageRequest sendTestMessage;
    
    // Status monitoring
    float pumpPressure;
    float ambientPressure;
    float pressureDifference;
    bool  pumpRunning;
    bool  sensorMalfunction;
    int   lastPumpChange;

    // System info
    UBaseType_t measureHighWaterMark;  // Unused stack for the measurement thread
    UBaseType_t displayHighWaterMark;  // Unused stack for the display thread
    UBaseType_t keyboardHighWaterMark; // Unused stack for the keyboard thread
    UBaseType_t telegramHighWaterMark; // Unused stack for the telegram thread

    // Logdata
    int timeToNextSample;
    long secondsSinceStart;    // Good for 2^31 sec = 78 years. Used for samples, also before synched with timeserver
    long offsetWithTimeServer; // = time(timeserver) - secondsSinceStart 
                                        //   real time of each sample = secondsSinceStart + offsetWithTimeServer
                                        //   can be converted to local time using: struct tm * localtime (const time_t * timer);
    list<tDataPoint> dataPoints;  
    list<tTransition> report;

    // Synching with time server
    char timezone[64];
    long gmtOffset;
    int dstOffset;
        
    // Initialize data 
    tData() {
      secondsSinceStart=0;
      screen=scMain;
      awake=true;
      sleepCountdown=3600;
      strncpy(timezone, "Europe/Amsterdam", sizeof(timezone));
      gmtOffset = 3600;
      dstOffset = 3600;
      timeSynched=false;
      pumpRunning=false;
      lastPumpChange=0;
      timeToNextSample=200;
      reconnectCountdown=RECONNECT_INTERVAL;

      sensorMalfunction=false;

      // Messages sent by Telegram
      sendSensorMalfunctionMessage= mrIdle;
      sendAlarmMessage = mrIdle;
      sendTestMessage  = mrIdle;
    }
};    

struct tAccessPoint {
  char ssid[64];
  char password[64];
  char timezone[64];
};

class tConfig {
  public:
    list<tAccessPoint> AccessPoints;  
    char logfile[64];
    int pressureThreshold;
    int sampleInterval;
    int pumpOffTimeOut;
    char botToken[64];
    char chatID[64];

    tConfig() {
      pressureThreshold=150;
      sampleInterval=5*60;
      pumpOffTimeOut=65*60; // Raise an alarm if the pump is switched off for exactly 65 minutes
      strncpy(logfile,"pressure.log",sizeof(logfile));
    }
  
    void Load() {    
      StaticJsonDocument<1024> doc;
      File input = SPIFFS.open(CONFIG_FILE);
      DeserializationError error = deserializeJson(doc, input);
      
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      
      for (JsonObject elem : doc["AccessPoints"].as<JsonArray>()) {
        tAccessPoint AccessPoint;
        strlcpy(AccessPoint.ssid     ,elem["SSID"],    sizeof(AccessPoint.ssid    ));
        strlcpy(AccessPoint.password ,elem["password"],sizeof(AccessPoint.password));
        strlcpy(AccessPoint.timezone ,elem["timezone"],sizeof(AccessPoint.timezone));          
        AccessPoints.push_back(AccessPoint);    
      }

      strlcpy(logfile, doc["LogFile"], sizeof(logfile)); // "log001.csv"
      strlcpy(botToken, doc["BotToken"], sizeof(botToken)); // "xxxxxxxxxx:yyyyyyyyyyyy-zzzzzzzzzzzzzz_qqqqqqq"
      strlcpy(chatID, doc["ChatID"], sizeof(chatID)); // "-xxxxxxxxxx"
      pressureThreshold = doc["pressureThreshold"]; // 150
      sampleInterval = doc["sampleInterval"]; // 60
      pumpOffTimeOut = doc["pumpOffTimeOut"]; // 3900

    }

    void Save() {
      // To do: loop over access points
    }
};


// ======== GLOBAL VARIABLES ============= 
tConfig config; // Configuration data, to be stored as JSON file in SPIFFS
tData data;     // Data, lost if device is switched off

portMUX_TYPE dataAccessMux = portMUX_INITIALIZER_UNLOCKED;

#endif
