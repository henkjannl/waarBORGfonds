#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// ======== INCLUDES =====================
#include <Arduino.h>
#include "wbf_data.h"

// ======== DEFINES ======================


//
// ======== CONSTANTS ============= 
//


// ======== GLOBAL VARIABLES ============= 
void taskGameplay(void * parameter);
bool checkAnswerIsOk();

TimerHandle_t keyboardTimer;   // Polls capacitive keys @30Hz

bool btnNextTouched=false;
bool btnOkTouched=false;


static void keyboardTimerCallback( TimerHandle_t xTimer ) {

  static volatile uint32_t keyNextCounter  = 0;
  static volatile uint32_t keyOKCounter    = 0;
  
  if (touchRead(PIN_BUT_1)<KEY_TRESHOLD) keyNextCounter++;  else keyNextCounter =0; 
  if (touchRead(PIN_BUT_2)<KEY_TRESHOLD) keyOKCounter++;    else keyOKCounter   =0; 

  if(keyNextCounter==3) { btnNextTouched=true; }
  if(keyOKCounter  ==3) { btnOkTouched  =true; }

}; // keyboardTimerCallback


void setupGameplay() 
{

  // Start a 30Hz timer that checks the capacitive keys
  keyboardTimer=xTimerCreate( "Keys", 
                pdMS_TO_TICKS(30), // Routine called at 30 Hz, so key response at 10 Hz
                pdTRUE,            // Auto reload, 
                0,                 // TimerID, unused
                keyboardTimerCallback); 

  Serial.printf("Create gameplay task\n");
  xTaskCreatePinnedToCore(
    taskGameplay,               // The function containing the task
    "taskGameplay",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    1,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on
}

void taskGameplay(void *parameter) 
{
   
  while(true) 
  {
      // Run the sate machines that manage which screen to display
      if (screen==scStartChallenge) 
      {

        // Initialize variables and go to the next state
        challengeID=0;
        radarSpinning=false;
        updateScreen=true;
        screen=scDisplayChallenge;

      } // screen==scStartChallenge

      else if (screen==scDisplayChallenge) 
      {
        if(btnNextTouched) {

          // Go to the next challenge
          challengeID++;
          updateScreen=true;

          // Allow recycling of first challenge if we accidentally pressed the wrong button
          if(challengeID>CHALLENGE_MAX-1) challengeID=0;

          btnNextTouched=false;
        }

        if(btnOkTouched) {
          // Stop spinning if the last challenge is reached, otherwise start spinning
          if (challengeID<CHALLENGE_MAX-1)
          {
            radarSpinning=true; // Start the radar
            updateScreen=true;  // Update the screen
            screen=scRadar;     // Go to the next state
          }

          btnOkTouched=false;
        }

        } // screen==scDisplayChallenge

      else if (screen==scRadar)
      {

        if(btnNextTouched) {

          btnNextTouched=false;
        }

        if(btnOkTouched) {

          btnOkTouched=false;
        }

      }
 
    vTaskDelay(100);
  } // while
}

#endif
