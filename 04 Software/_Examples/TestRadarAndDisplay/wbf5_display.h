#ifndef DISPLAY_H
#define DISPLAY_H

//
// ======== INCLUDES =====================
//
#include <SPI.h>
#include <TFT_eSPI.h>  
#include <time.h>     
#include <math.h>  
#include "wbf2_data.h"

//
// ======== DEFINES ======================
//
#define CLR_BACKGROUND   0x0000   // 00, 00, 00 = black
#define CLR_DARK         0x2945   // 28, 28, 28 = dark grey
#define CLR_TEXT         0xFFFF   // FF, FF, FF = white
//#define CLR_LABELS     0x8410   // 80, 80, 80 = grey
#define CLR_GREEN        0x0400   // 00, 80, 00 = green
#define CLR_RED          0xF800   // FF, 00, 00 = red

#define KEY_THRESHOLD  50

#define SCREEN_CENTER_X 120
#define SCREEN_CENTER_Y 120
#define RADIUS_SCALE 0.34/2*116.0/400                 // 0.34 mm equals 2*1µs, and 400 mm distance maps on 120 pixels
#define ANGLE_OFFSET  0.610865238                     // First measurement maps on this angle
#define ANGLE_GAIN   -4.36332313/RADAR_ARRAY_SIZE     // All measurements span this angle

//
// ======== GLOBAL VARIABLES ============= 
//
TFT_eSPI tft = TFT_eSPI();  

void taskDisplay(void * parameter );
void drawBmp(TFT_eSPI& tft, const char *filename, int16_t x, int16_t y);


//
// ======== ISR ==========================
//
void onDisplayCheckKeys() 
{
  static volatile int keyPlayCntr  = 0;
  static volatile int keyOkCntr = 0;

  if (touchRead(PIN_BUT_1)<KEY_THRESHOLD) keyPlayCntr++;  else keyPlayCntr =0; 
  if (touchRead(PIN_BUT_2)<KEY_THRESHOLD) keyOkCntr++; else keyOkCntr=0; 

  if(keyPlayCntr==3 || keyOkCntr==3)
  {
    //only enter critical when key is pressed
    portENTER_CRITICAL_ISR(&dataAccessMux);
    if(keyPlayCntr==3)data.btnPlayTouched=true;
    if(keyOkCntr==3)data.btnOkTouched=true;
    portEXIT_CRITICAL_ISR(&dataAccessMux); 
  }
}


//
// ======== ROUTINES ===================== 
//
void setupDisplay() 
{
  // Initialize screen
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  tScreen prevScreen=scRadar; 
  
  Serial.printf("Create display task\n");
  xTaskCreatePinnedToCore(
    taskDisplay,            // The function containing the task
    "TaskDisplay",          // Name for mortals
    12000,                  // Stack size 
    NULL,                   // Parameters
    1,                      // Priority, 0=lowest, 3=highest
    NULL,                   // Task handle
    ARDUINO_RUNNING_CORE);  // Core to run the task on
}

void taskDisplay(void * parameter )
{
  float screenData[RADAR_ARRAY_SIZE]; 
  int antennaPos;
  
  bool updateScreen;

  tPixel prevPoint;
  tPixel currPoint;
   
  while(true) {
    switch(data.screen) {
      case scRadar:
        // Radar screen
        
        // Copy most recent data from the radar
        portENTER_CRITICAL(&dataAccessMux);
          for(int i=0; i<RADAR_ARRAY_SIZE; i++) screenData[i]=radarMeasurements[i];
          antennaPos=stepper.currentPos();
        portEXIT_CRITICAL(&dataAccessMux);

        // Clear screen
        tft.fillScreen(CLR_BACKGROUND);    
        tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 120, CLR_DARK);
        
        // Draw radar information as polygon
        float radius = RADIUS_SCALE*screenData[0];
        float angle =  ANGLE_OFFSET+0*ANGLE_GAIN;

        currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
        currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
        
        for(int i=1;i<RADAR_ARRAY_SIZE;i++) {
          prevPoint.x=currPoint.x;
          prevPoint.y=currPoint.y;

          radius = RADIUS_SCALE*screenData[i];
          if(radius>120) radius=120;
          angle =  ANGLE_OFFSET+i*ANGLE_GAIN;

          currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
          currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);

          tft.drawLine(prevPoint.x, prevPoint.y, currPoint.x, currPoint.y, CLR_GREEN);

          } // for i
        Serial.printf("Screen refreshed %f %f %d %d\n", angle, radius, currPoint.x, currPoint.y);
        break;
        
    }// Switch screen

      
    //data.displayHighWaterMark= uxTaskGetStackHighWaterMark( NULL );
    //Serial.printf("Display: %d bytes of stack unused\n", data.displayHighWaterMark);
    vTaskDelay(1000); // Repeat once per second
  } // while true

}//taskDisplay




#endif
