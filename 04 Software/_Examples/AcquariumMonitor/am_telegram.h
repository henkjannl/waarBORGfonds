#ifndef TELEGRAM_H
#define TELEGRAM_H

#include "am_data.h"

#include "esp_wifi.h" 
#include <WiFi.h>
#include <HTTPClient.h>

#include <UniversalTelegramBot.h>

#include <ESP32Ping.h> // Download from https://github.com/marian-craciunescu/ESP32Ping

#include <string>

// ======== DEFINES ================

// ======== GLOBAL VARIABLES ============= 
WiFiClientSecure secured_client;
UniversalTelegramBot bot("", secured_client); // Token given later when config is loaded

void taskTelegram(void * parameter );

void connectToWiFi() {
  
  /* =========================================================
   *  The code below is way more complex than I'd wish
   *  but somehow I could not make wifiMulti work together 
   *  with FreeRTOS
   *  ======================================================== */
   
  bool connected=false;

  for(auto accessPoint : config.AccessPoints) Serial.println(accessPoint.ssid);

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) 
      for(auto accessPoint : config.AccessPoints) 
        if( WiFi.SSID(i) == String(accessPoint.ssid) ) {
          Serial.printf("Connecting to %s\n", accessPoint.ssid);  
          // attempt to connect to Wifi network:
          WiFi.begin(accessPoint.ssid, accessPoint.password);
          for(int i=0; i<35; i++) {
            if (WiFi.status() != WL_CONNECTED)
              {
                Serial.print(".");
                vTaskDelay(500);
              }
            else {
              Serial.printf("\nWiFi connected to %s.\n", WiFi.SSID());
              Serial.println();
              connected = true;
              break;
            } // not yet connected
            if(connected) break;
        } // connection attempt 
        if(connected) break;
      } // ssid equal to accesPoint else
  } // networks found

  portENTER_CRITICAL(&dataAccessMux);
  data.connected = (WiFi.status() == WL_CONNECTED);
  portEXIT_CRITICAL(&dataAccessMux);  
};

void setupTelegram() {
  HTTPClient http;
  int httpCode;
  char buffer[100];
  int gmt;
  int dst;

  while(!data.connected) connectToWiFi();

  // Setup secure chat with Telegram
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  bot.updateToken(config.botToken); // Update bot token after loading config
  Serial.printf("Bot token: %s\n", config.botToken);

  Serial.println("Getting current daylight saving time");
  gmt=data.gmtOffset;
  dst=data.dstOffset;
  
  // Retrieve the timezone difference and the current daylight saving time difference from worldtimeapi.org
  http.begin(String("http://worldtimeapi.org/api/timezone/")+ data.timezone); 
  httpCode = http.GET();
  
  if(httpCode > 0) {    
    if(httpCode == HTTP_CODE_OK) {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, http.getString());
      dst = doc["dst_offset"]; // 3600
      gmt = doc["raw_offset"]; // 3600
    } // HTTP OK
  } // httpCode>0

  configTime(gmt, dst, "pool.ntp.org");

  // Check if time sync is successful
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println();

  // Set bot commands
  const String commands = F("["
                            "{\"command\":\"help\"  , \"description\":\"Overzichtje van de beschikbare commando's\"},"
                            "{\"command\":\"status\", \"description\":\"Actuele status van de druk\"},"
                            "{\"command\":\"report\", \"description\":\"Overzicht van de afgelopen 24 uur\"},"
                            "{\"command\":\"system\", \"description\":\"Actuele status van de software\"}" 
                            "]");
  
  bot.setMyCommands(commands);

  // Send headsup message
  bot.sendMessage(config.chatID, "\u270B De acquarium luchtdruksensor is gestart", "");

  // Add startup message to logfile
  portENTER_CRITICAL(&dataAccessMux);
    data.dstOffset = dst;
    data.gmtOffset = gmt;
    data.connected=true;
    data.timeSynched=true;
  
    // Log first pump event
    tTransition transition;
    time_t rawtime;
    struct tm * timeinfo;
  
    transition.secondsSinceStart=data.secondsSinceStart;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(transition.time,sizeof(transition.time),"%H:%M:%S",timeinfo);
    transition.state=seStart;
    data.report.push_back(transition);    
  portEXIT_CRITICAL(&dataAccessMux);
  
  xTaskCreatePinnedToCore(
    taskTelegram,            // The function containing the task
    "TaskTelegram",          // Name for mortals
    32768,                   // Stack size 
    NULL,                    // Parameters
    1,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on
}

String formatFloat(float val, int prec) {
  char buff[25];
  dtostrf(val, 2, prec, buff);
  return String(buff);
}

void startMessage(UniversalTelegramBot& bot, telegramMessage& msg) {
  String answer;
  answer="Welkom *"+msg.from_name+"*";
  bot.sendMessage(msg.chat_id, answer, "Markdown");
}

void helpMessage(UniversalTelegramBot& bot, telegramMessage &msg) {
  String answer;
  answer= "Gebruik:\n"
          "  */start* om de communicatie te starten\n"
          "  */status* om de huidige status op te vragen\n"
          "  */report* om een overzicht te krijgen van de compressor acties\n"
          "  */system* om te kijken naar het geheugengebruik\n";
  bot.sendMessage(msg.chat_id, answer, "Markdown");
}

void statusMessage(UniversalTelegramBot& bot, telegramMessage& msg) {
  String answer;      
  answer = "De compressor staat al *" + String( (int) data.lastPumpChange/60) + "* minuten *" + (data.pumpRunning ? "AAN" : "UIT") + "*\n";
  answer+= "Het drukverschil is *"+ formatFloat(data.pressureDifference/100, 1) +"* mbar\n";
  
  if(data.sensorMalfunction) answer+= "Mogelijk is er één van de sensoren kapot";
  
  bot.sendMessage(msg.chat_id, answer, "Markdown");
}

void systemMessage(UniversalTelegramBot& bot, telegramMessage& msg) {
  String answer;      
  float avg_time_ms;
  size_t freeheap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t minheap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  
  answer = "De applicatie draait nu *" + formatFloat((float)data.secondsSinceStart/3600,2) + "* uur\n";
  answer+= "De leidingdruk is *" + formatFloat(data.pumpPressure/100,1) + "* mbar\n";
  answer+= "De buitenluchtdruk is *" + formatFloat(data.ambientPressure/100, 1) + "* mbar\n";

  if(data.dataPoints.size()<1) 
    answer+= "Er is nog geen logdata gemeten\n";
  else {
    float mina=1e6, maxa=-1e6, minp=1e6, maxp=-1e6, mind=1e6, maxd=-1e6;
  
    for (auto &dataPoint: data.dataPoints)
      {
          if(mina>dataPoint.mina) mina=dataPoint.mina;
          if(maxa<dataPoint.maxa) maxa=dataPoint.maxa;
          if(minp>dataPoint.minp) minp=dataPoint.minp;
          if(maxp<dataPoint.maxp) maxp=dataPoint.maxp;
          if(mind>dataPoint.mind) mind=dataPoint.mind;
          if(maxd<dataPoint.maxd) maxd=dataPoint.maxd;
      } // for datapoint

    answer+= "Gegevens over de laatste *"+formatFloat( (float) (data.secondsSinceStart-data.dataPoints.front().secondsSinceStart)/3600, 1)+"* uur:\n";
    answer+= " - Buitenluchtdruk was tussen *" + formatFloat(mina/100,1) + "* en *"+formatFloat(maxa/100,1)+"* mbar\n";
    answer+= " - Leidingdruk was tussen *"     + formatFloat(minp/100,1) + "* en *"+formatFloat(maxp/100,1)+"* mbar\n";
    answer+= " - Het drukverschil was tussen *"+ formatFloat(mind/100,1) + "* en *"+formatFloat(maxd/100,1)+"* mbar\n";
  } // datapoints.size > 1

  answer+= "Chat ID *" + String(msg.chat_id) + "*\n";
  answer+= "Vrije heap *" + String(freeheap) + "* bytes\n";
  answer+= "Minimum heap *" + String(minheap) + "* bytes\n";
  answer+= "Vrije ruimte op stack voor de verschillende threads:\n";
  answer+= " - *measure* thread *"  + String(data.measureHighWaterMark)  + "* bytes\n";
  answer+= " - *display* thread *"  + String(data.displayHighWaterMark)  + "* bytes\n";
  answer+= " - *keyboard* thread *" + String(data.keyboardHighWaterMark) + "* bytes\n";
  answer+= " - *telegram* thread *" + String(data.telegramHighWaterMark) + "* bytes\n";

  if (Ping.ping(TELEGRAM_HOST)) 
    answer+="Gemiddelde ping tijd naar telegram.org *" + formatFloat(Ping.averageTime(),3) + "* ms\n";
  else
    answer+="Geen antwoord op ping naar telegram.org\n";

  if(data.sensorMalfunction) answer+= "Mogelijk is er één van de sensoren kapot";
        
  bot.sendMessage(msg.chat_id, answer, "Markdown");
}

void reportMessage(UniversalTelegramBot& bot, telegramMessage& msg) {
  String answer;      

  if(data.report.size()<1) 
    answer= "Er de laatste 24 uur niets gebeurd\n";
  else 
    answer= "Overzicht van de laatste 24 uur:\n";
    for (auto &transition: data.report) {
      answer+= String(transition.time);
      if      (transition.state==seStart            ) answer+= " aquarium alarm opgestart\n";
      else if (transition.state==seCompressorOn     ) answer+= " compressor aan\n";
      else if (transition.state==seCompressorOff    ) answer+= " compressor uit\n";
      else if (transition.state==seAlarm            ) answer+= " alarm dat de compressor uitstaat\n";
      else if (transition.state==seSensorMalfunction) answer+= " alarm dat er misschien een sensor stuk is\n";
    }

  bot.sendMessage(msg.chat_id, answer, "Markdown");
}


void alarmMessage(UniversalTelegramBot& bot) {
  String answer;        
  answer = "* \u274c ALARM! DE COMPRESSOR STAAT AL " + String(data.lastPumpChange/60) + " MINUTEN UIT! \u274c *";
  bot.sendMessage(config.chatID, answer, "Markdown");
  
  // Log alarm message
  tTransition transition;
  time_t rawtime;
  struct tm * timeinfo;

  transition.secondsSinceStart=data.secondsSinceStart;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(transition.time,sizeof(transition.time),"%H:%M:%S",timeinfo);
  transition.state=seAlarm;

  portENTER_CRITICAL(&dataAccessMux);
  data.report.push_back(transition);  
  portEXIT_CRITICAL(&dataAccessMux);
}

void sensorMalfunctionMessage(UniversalTelegramBot& bot) {
  String answer;        
  answer = "\u274c *Het lijkt erop dat één van de druksensoren kapot is:*\n";
  answer+= " - de pompdruk sensor geeft *" + formatFloat(data.pumpPressure/100,1) + "* mbar aan\n";
  answer+= " - de buitenluchtdruk sensor geeft *" + formatFloat(data.ambientPressure/100, 1) + "* mbar aan\n";
  
  bot.sendMessage(config.chatID, answer, "Markdown");

  // Log sensor malfunction message
  tTransition transition;
  time_t rawtime;
  struct tm * timeinfo;

  transition.secondsSinceStart=data.secondsSinceStart;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(transition.time,sizeof(transition.time),"%H:%M:%S",timeinfo);
  transition.state=seSensorMalfunction;

  portENTER_CRITICAL(&dataAccessMux);
  data.report.push_back(transition);  
  portEXIT_CRITICAL(&dataAccessMux);
}

void testMessage(UniversalTelegramBot& bot) {
  bot.sendMessage(config.chatID, "\u2714 Dit is een testbericht", "Markdown");
}

void taskTelegram(void * parameter )
{  
  while(true) {

    // Store connectivity result
    portENTER_CRITICAL(&dataAccessMux);
    data.connected=(WiFi.status()==WL_CONNECTED);
    portEXIT_CRITICAL(&dataAccessMux);

    if(!data.connected && data.reconnectCountdown==0) {
      connectToWiFi();
      //bot.sendMessage("46501331", "De WiFi verbinding was verbroken maar is weer hersteld", "Markdown");
    }
    
    if(data.connected) {
      
      portENTER_CRITICAL(&dataAccessMux);
      data.reconnectCountdown=RECONNECT_INTERVAL; // No need to reconnect
      portEXIT_CRITICAL(&dataAccessMux);
    
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages)
      {
        Serial.println("got response");
        for (int i = 0; i < numNewMessages; i++) {
          telegramMessage &msg = bot.messages[i];
          Serial.println("Received " + msg.text + " from " +msg.chat_id);
      
          if      ( (msg.text == "/help")   || (msg.text == "/help@AirPressureAlarmBot"  ) ) helpMessage  (bot, msg);
          else if ( (msg.text == "/start")  || (msg.text == "/start@AirPressureAlarmBot" ) ) startMessage (bot, msg);
          else if ( (msg.text == "/status") || (msg.text == "/status@AirPressureAlarmBot") ) statusMessage(bot, msg);
          else if ( (msg.text == "/system") || (msg.text == "/system@AirPressureAlarmBot") ) systemMessage(bot, msg);  
          else if ( (msg.text == "/report") || (msg.text == "/report@AirPressureAlarmBot") ) reportMessage(bot, msg);
        } // For i
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      if(data.sendAlarmMessage==mrRequestMessage) {
          alarmMessage(bot);
          portENTER_CRITICAL(&dataAccessMux);
          data.sendAlarmMessage=mrMessageSent;
          portEXIT_CRITICAL(&dataAccessMux);
      }

      if(data.sendTestMessage==mrRequestMessage) {
          testMessage(bot);
          portENTER_CRITICAL(&dataAccessMux);
          data.sendTestMessage=mrMessageSent;
          portEXIT_CRITICAL(&dataAccessMux);
      }

      if(data.sendSensorMalfunctionMessage==mrRequestMessage) {
          sensorMalfunctionMessage(bot);
          portENTER_CRITICAL(&dataAccessMux);
          data.sendSensorMalfunctionMessage=mrMessageSent;
          portEXIT_CRITICAL(&dataAccessMux);
        }

    } // if connected

    portENTER_CRITICAL(&dataAccessMux);
    data.telegramHighWaterMark= uxTaskGetStackHighWaterMark( NULL );
    portEXIT_CRITICAL(&dataAccessMux);
    //Serial.printf("Telegram: %d bytes of stack unused\n", data.telegramHighWaterMark);
    
    vTaskDelay(1000); // Check traffic every second plus the time it takes to check

  }  // while true
}

#endif
