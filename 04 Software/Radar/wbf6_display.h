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
#define CLR_RADAR_WIPER  0xF800   // FF, 00, 00 = red
#define CLR_LABELS       0xF800   // FF, 00, 00 = red
#define CLR_RED_RADAR    0xA000   // A0, 00, 00 = dark red
#define CLR_GREEN_RADAR  0x0400   // 00, 80, 00 = green
#define CLR_SMALL_TEXT   0x8410   // 80, 80, 80 = light grey

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
  
  float radius;          
  float angle;


  uint8_t counter;
  bool C[5];

   
  while(true) {

      // Run the state machines that manage which screen to display
      switch(mainState) {
        
        case msWelcomeScreen:
  
          if(updateScreen) 
          {
            updateScreen=false;

            // Clear screen
            tft.fillScreen(CLR_BACKGROUND);    
  
            tft.fillRoundRect(8, 32, 224, 80, 16, CLR_LABELS);
            tft.setTextDatum(MC_DATUM);
            
            tft.setTextColor(CLR_TEXT);
            tft.drawString("WELCOME",  120,  72-16, 4);
            tft.drawString("MARC!",    120,  72+16, 4);
            
            tft.setTextColor(CLR_SMALL_TEXT);
            tft.drawString("Press any key", 120, 172, 4);
            tft.drawString("to continue",   120, 204, 4);
  
          } // updateScreen 
          break; // case msWelcomeScreen

        
        case msConfirmChallenge:
  
          // Display the current challenge on screen and wait for input from keyboard
          if(updateScreen) 
          {
            updateScreen=false;

            // Clear screen
            tft.fillScreen(CLR_BACKGROUND);    
  
            tft.setTextColor(CLR_SMALL_TEXT);
            tft.setTextDatum(TL_DATUM); tft.drawString("NEXT",     4, 4, 4);
            tft.setTextDatum(TR_DATUM); tft.drawString("ACCEPT", 236, 4, 4);
  
            tft.fillCircle(120, 160, 80, CLR_LABELS);
  
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(CLR_SMALL_TEXT);
            tft.drawString("Challenge:", 120, 65, 4);
            tft.setTextColor(CLR_TEXT);
            tft.drawString(challenges[challengeID].title, 120, 160, 8);  
          }
          break; // case msConfirmChallenge

  
        case msRadar:

          // Copy most recent data from the radar
          portENTER_CRITICAL(&radarMux);
            for(int i=0; i<RADAR_ARRAY_SIZE; i++) bottleDistance[i]=radarMeasurements[i];
            antennaPos=sampleID;
          portEXIT_CRITICAL(&radarMux);
  
          // Clear screen
          tft.fillScreen(CLR_BACKGROUND);    
          tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 120, CLR_DARK);
          
          // Draw radar information as polygon
          radius = min(RADIUS_SCALE*bottleDistance[0],120.0);
          angle =  ANGLE_OFFSET+0*ANGLE_GAIN;
          currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
          currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
          
          for(int i=1;i<RADAR_ARRAY_SIZE;i++) 
            {
              prevPoint.x=currPoint.x;
              prevPoint.y=currPoint.y;
  
              radius = min(RADIUS_SCALE*bottleDistance[i],120.0);
              angle =  ANGLE_OFFSET+i*ANGLE_GAIN;
  
              currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
              currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
  
              tft.drawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y, currPoint.x, currPoint.y, CLR_SMALL_TEXT);
              tft.drawLine(prevPoint.x,     prevPoint.y,     currPoint.x, currPoint.y, CLR_TEXT);
  
            } // for i

            // Draw threshold circle
            radius = RADIUS_SCALE*BOTTLE_PRESENT_THRESHOLD;
            tft.drawCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, radius, CLR_RED_RADAR);
  
            // Draw radar wiper
            radius = 120;
            angle =  ANGLE_OFFSET+antennaPos*ANGLE_GAIN;
  
            currPoint.x=SCREEN_CENTER_X+radius*cos(angle);
            currPoint.y=SCREEN_CENTER_Y-radius*sin(angle);
            tft.drawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y, currPoint.x, currPoint.y, CLR_RADAR_WIPER);
   
            // Check if the answer is correct

            // Calculate the C coefficients
            counter=0; 
            for(int i=21; i<29; i++) if(bottleDistance[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
            C[0]=(counter>4);

            counter=0; 
            for(int i=25; i<33; i++) if(bottleDistance[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
            C[1]=(counter>4);

            counter=0; 
            for(int i=29; i<37; i++) if(bottleDistance[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
            C[2]=(counter>4);

            counter=0; 
            for(int i=33; i<41; i++) if(bottleDistance[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
            C[3]=(counter>4);
            
            counter=0; 
            for(int i=37; i<45; i++) if(bottleDistance[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
            C[4]=(counter>4);

            // Calculate possible answers
            if       (!C[0] && !C[1] &&  C[3] &&  C[4]) answer=anA;
            else if  ( C[0] && !C[2] && !C[4])          answer=anC;
            else if  ( C[0] &&  C[2] &&  C[4])          answer=anAandC;
            else if  ( C[1] &&  C[2] &&  C[3])          answer=anB;
            else                                        answer=anNoBottles;

            char buffer[10];
            buffer[0]='C';
            buffer[1]=C[0] ? 'X' : '.';
            buffer[2]=C[1] ? 'X' : '.';
            buffer[3]=C[2] ? 'X' : '.';
            buffer[4]=C[3] ? 'X' : '.';
            buffer[5]=C[4] ? 'X' : '.';
            buffer[6]=' ';
            buffer[7]='\0';
            debug=String(buffer);

  
            if(answer==challenges[challengeID].answer) 
            {
              // All the bottles were in the right places
              if(challengeID==NUM_CHALLENGES-1)
                mainState=msCompleted;
              else
                mainState=msAnswerCorrect;
  
              updateScreen=true;
            }  
            else
              vTaskDelay(1000); // Refresh radar image once per second

        break; // case msRadar 

  
        case msAnswerCorrect:
          if(updateScreen) 
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
          break; // case msAnswerCorrect

        case msCompleted:

          if(updateScreen) 
          {
            // Clear screen
            tft.fillScreen(CLR_BACKGROUND);    
            tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 120, CLR_GREEN_RADAR);
    
            tft.setTextColor(CLR_TEXT);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("FINISHED!", SCREEN_CENTER_X, SCREEN_CENTER_Y, 4);
    
            updateScreen=false;
            vTaskDelay(3000); // Delay next refresh of the screen for 3 seconds
          }
        break; // msCompleted
          
      } // switch mainState
      

  } // while true

}//taskDisplay

#endif
