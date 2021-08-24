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
#define CLR_RADAR_WIPER  0xA7E2   // A3, FF, 12 = red
#define CLR_LABELS       0x8410   // 80, 80, 80 = grey
#define CLR_RED_RADAR    0xA000   // A0, 00, 00 = dark red
#define CLR_GREEN_RADAR  0x0400   // 00, 80, 00 = green
#define CLR_SMALL_TEXT   0xFFE0   // FF, FF, 00 = yellow

#define SCREEN_CENTER_X 120
#define SCREEN_CENTER_Y 120
#define RADIUS_SCALE    120.0/450                       // 450 mm distance maps on 120 pixels
#define RADIUS_MAX      116                             // Distances clip after this value
#define ANGLE_OFFSET    0.610865238                     // First measurement maps on this angle
#define ANGLE_GAIN      -4.36332313/RADAR_ARRAY_SIZE    // All measurements span this angle



//
// ======== GLOBAL VARIABLES ============= 
//
TFT_eSPI tft = TFT_eSPI();  

void taskDisplay(void * parameter );

//
// ======== ROUTINES ===================== 
//
void setupDisplay() 
{
  // Initialize screen
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tMainState prevScreen=msRadar; 
  
  Serial.print("Create display task\n");
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
  float bottleDistance[RADAR_ARRAY_SIZE]; 
  int antennaPos;
  
  tPixel prevPoint;
  tPixel currPoint;
   
  while(true) {

      // Run the sate machines that manage which screen to display
        // msFirstChallenge passes quickly, no need to draw anything

      if (mainState==msConfirmChallenge) {

        // Display the current challenge on screen and wait for input from keyboard
        if(updateScreen) 
        {
          // Clear screen
          tft.fillScreen(CLR_BACKGROUND);    

          tft.fillRoundRect(8, 32, 224, 32, 8, CLR_LABELS);
          tft.setTextColor(CLR_TEXT);
          tft.setTextDatum(MC_DATUM);
          tft.drawString(challenges[challengeID].titleLine1, 120, 48, 4);

          tft.fillRoundRect(8, 72, 224, 32, 8, CLR_LABELS);
          tft.setTextColor(CLR_TEXT);
          tft.setTextDatum(MC_DATUM);
          tft.drawString(challenges[challengeID].titleLine2, 120, 88, 4);

          tft.setTextColor(CLR_SMALL_TEXT);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("press next to go to the next challenge", 120, 128, 2);
          tft.drawString("press OK to accept this challenge",      120, 168, 2);

          updateScreen=false;
        }

      } // mainState==msConfirmChallenge

      else if ( mainState==msRadar) 
      {

        // Copy most recent data from the radar
        portENTER_CRITICAL(&radarMux);
          for(int i=0; i<RADAR_ARRAY_SIZE; i++) bottleDistance[i]=radarMeasurements[i];
          antennaPos=stepper.currentPos();
        portEXIT_CRITICAL(&radarMux);

        // Clear screen
        tft.fillScreen(CLR_BACKGROUND);    
        tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 120, CLR_DARK);
        
        // Draw radar information as polygon
        float radius = RADIUS_SCALE*bottleDistance[0];
        float angle =  ANGLE_OFFSET+0*ANGLE_GAIN;

        currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
        currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
        
        for(int i=1;i<RADAR_ARRAY_SIZE;i++) 
          {
            prevPoint.x=currPoint.x;
            prevPoint.y=currPoint.y;

            radius = RADIUS_SCALE*bottleDistance[i];
            if(radius>120) radius=120;
            angle =  ANGLE_OFFSET+i*ANGLE_GAIN;

            currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
            currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);

            tft.drawLine(prevPoint.x, prevPoint.y, currPoint.x, currPoint.y, CLR_TEXT);

          } // for i

          // Draw radar wiper
          radius = 120;
          angle =  ANGLE_OFFSET+antennaPos*ANGLE_GAIN;

          currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
          currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
          tft.drawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y, currPoint.x, currPoint.y, CLR_RADAR_WIPER);


          // Check if the answer is correct
          // Perhaps we should do that in the game play module, but here, a copy of the radar data is available
          // and this function is conveniently called once a second
          bool answer=true;
          float filteredLocation=0;
          for(int i=0; i<NUM_BOTTLE_LOCATIONS; i++) {
            // To prevent outliers, use average of multiple measurements
            filteredLocation=(bottleDistance[bottleLocations[i]-1] + bottleDistance[bottleLocations[i]] + bottleDistance[bottleLocations[i]+1] )/3;

            //      there is a bottle                        !=    there should be a bottle     
            if ( (filteredLocation<BOTTLE_PRESENT_THRESHOLD) != challenges[challengeID].hasBottle[i] ) answer=false;
          }

          if(answer) 
          {
            // All the bottles were in the right places
            mainState=msAnswerCorrect;
            updateScreen=true;
          }  
          else
          {
            vTaskDelay(1000); // Refresh radar image once per second
          }


      } // mainState==msRadar

      else if ( mainState==msAnswerCorrect) 
      {
        // Clear screen
        tft.fillScreen(CLR_BACKGROUND);    
        tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 120, CLR_GREEN_RADAR);

        tft.setTextColor(CLR_TEXT);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("CONGRATULATIONS!", SCREEN_CENTER_X, SCREEN_CENTER_Y, 4);

        updateScreen=false;
        vTaskDelay(3000); // Delay next refresh of the screen for 3 seconds
      }

  } // while true

}//taskDisplay

#endif
