#ifndef DISPLAY_H
#define DISPLAY_H

#include "am_data.h"

#include <SPI.h>
#include <TFT_eSPI.h>  

#include <time.h>       

using namespace std;


// ======== DEFINES ========
#define CLR_BACKGROUND   0x0000   // 00, 00, 00 = black
#define CLR_DARK         0x2945   // 28, 28, 28 = dark grey
#define CLR_TEXT         0xFFFF   // FF, FF, FF = white
#define CLR_LABELS       0x8410   // 80, 80, 80 = grey
#define CLR_PUMP_ON      0x0400   // 00, 80, 00 = green
#define CLR_PUMP_OFF     0xF800   // FF, 00, 00 = red

// ======== GLOBAL VARIABLES ============= 
TFT_eSPI tft = TFT_eSPI();  

void taskDisplay(void * parameter );
void drawBmp(TFT_eSPI& tft, const char *filename, int16_t x, int16_t y);

void setupDisplay() {

  // Initialize screen
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  
  xTaskCreatePinnedToCore(
    taskDisplay,            // The function containing the task
    "TaskDisplay",          // Name for mortals
    12000,                  // Stack size 
    NULL,                   // Parameters
    1,                      // Priority, 0=lowest, 3=highest
    NULL,                   // Task handle
    ARDUINO_RUNNING_CORE);  // Core to run the task on

}

void taskDisplay(void * parameter ){
  static bool prevAwake=true;
  static tScreen prevScreen=scSensorMalfunction; bool updateScreen;
  static char prevTime[12];                      bool updateTime;     
  static float prevPressure;                     bool updatePressure;
  char currTime[16];
  char buffer[40];
  time_t rawtime;
  struct tm * timeinfo;

  while(true) {

    // Check if time needs to be updated
    if(data.timeSynched) {
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(currTime,sizeof(currTime),"%H:%M",timeinfo);
    }
    else
      strncpy(currTime, "--:--", sizeof(currTime));
      
    updateTime=(strcmp(currTime,prevTime)!=0); // If the times are changed, redraw time

    // Check if pressure needs to be updated
    updatePressure=(abs(data.pressureDifference-prevPressure)>100.0); // If pressure is changed, redraw pressure
    
    // Check if the whole screen needs to be updated
    updateScreen=(data.screen!=prevScreen);          // If screen has changed, redraw screen
    if(data.awake and !prevAwake) updateScreen=true; // Also redraw if we are just waking up
    prevAwake=data.awake;
    
    if(updateScreen) { 
      // If the screen is redrawn, time and pressure must be too
      updateTime=true;
      updatePressure=true;
    }
    
    if (data.awake) {
      
      switch(data.screen) {
        
        case scMain:
          // Main screen
          
          // Redraw the whole screen
          if(updateScreen) {
            //tft.fillScreen(CLR_BACKGROUND);
            tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar

            // Box for time
            tft.fillRoundRect(44, 40, 152, 48, 8, CLR_DARK);

            // Box for pressure
            tft.fillRoundRect(4, 100, 152, 48, 8, CLR_DARK);

            // Box for time pump switched
            tft.fillRoundRect(4, 160, 152, 48, 8, CLR_DARK);

            tft.setTextColor(CLR_LABELS);
            tft.setTextDatum(TL_DATUM);
            tft.drawString("mbar", 165, 100, 4);
            tft.drawString("min", 165, 160, 4);
            
            prevScreen=data.screen;
          }
  
          if(updateTime) {
            // Draw the box with the time
            tft.fillRoundRect(44, 40, 152, 48, 8, CLR_DARK);
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MC_DATUM);
            tft.drawString(currTime, 120, 68, 6);

            // Update last pump switch time
            tft.fillRoundRect(4, 160, 152, 48, 8, CLR_DARK);
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MR_DATUM);
            tft.drawNumber( (int) data.lastPumpChange/60, 144, 188, 6);
  
            strncpy(prevTime,currTime,sizeof(prevTime));
          }
  
          if(updatePressure) {
            tft.fillRoundRect(4, 100, 152, 48, 8, CLR_DARK);
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MR_DATUM);
            tft.drawFloat(0.01*data.pressureDifference, 1, 144,128, 6);
            prevPressure=data.pressureDifference;
          }          
        break;
  
        case scGraph:
          if(updateScreen) {
            //tft.fillScreen(CLR_BACKGROUND);
            tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Graph", 120, 120, 4);

            prevScreen=data.screen;
          }
          break;
  
        case scMessage:
  
          if(updateScreen) {
            tft.setTextColor(CLR_TEXT, CLR_DARK);
            tft.setTextDatum(MC_DATUM);

            //tft.fillScreen(CLR_BACKGROUND);
            tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar
            tft.fillRoundRect(16, 40,208,72,36,CLR_DARK);
            tft.drawString("BEGIN-", 92, 63, 4);
            tft.drawString("SCHERM", 92, 90, 4);
            tft.fillCircle(188,  76, 27, TFT_BLACK);

            tft.fillRoundRect(16,132,208,72,36,CLR_DARK);
            tft.drawString("TEST", 92, 155, 4);
            tft.drawString("BERICHT", 92, 182, 4);
            tft.fillCircle(188, 168, 27, CLR_TEXT);

            prevScreen=data.screen;
          }
          break;
  
        case scMessageSent:
        
          if(updateScreen) {
            //tft.fillScreen(CLR_BACKGROUND);
            tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Testbericht",  120, 105, 4);
            tft.drawString("is verstuurd", 120, 135, 4);

            prevScreen=data.screen;
          }
          
          if(data.messageSentCountDown==0) {
            portENTER_CRITICAL(&dataAccessMux);
            data.screen=scMain;
            portEXIT_CRITICAL(&dataAccessMux);
          }
          
          break;

        case scSensorMalfunction:
        
          if(updateScreen) {
            //tft.fillScreen(CLR_BACKGROUND);
            tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Mogelijk is er een", 120, 105, 4);
            tft.drawString("druksensor kapot",   120, 135, 4);
            
            prevScreen=data.screen;
          }
                    
          break;

        } // Switch screen

        // Update pump running bar
        tft.fillRoundRect(8, 216, 224, 16, 5, data.pumpRunning ? CLR_PUMP_ON : CLR_PUMP_OFF);

        drawBmp(tft, data.connected         ? "/WifiConn.bmp" : "/WifiDis.bmp" , 180, 0);
        drawBmp(tft, data.timeSynched       ? "/TmSync.bmp"   : "/TmNSync.bmp" , 210, 0);
        drawBmp(tft, data.sensorMalfunction ? "/SnBroken.bmp" : "/SensorOK.bmp", 150, 0);

        
      data.displayHighWaterMark= uxTaskGetStackHighWaterMark( NULL );
      //Serial.printf("Display: %d bytes of stack unused\n", data.displayHighWaterMark);

      } // If data awake        
      else tft.fillScreen(TFT_BLACK); // Make the screen black when asleep
      
      
    vTaskDelay(100);

  } // while true

}

// Functions to display bitmaps, provided by Bodmer

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Bodmers BMP image rendering function
void drawBmp(TFT_eSPI& tft, const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SPIFFS
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}


#endif
