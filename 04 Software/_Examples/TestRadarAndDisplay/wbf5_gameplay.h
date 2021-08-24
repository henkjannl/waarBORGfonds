#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// ======== INCLUDES =====================
#include "wbf2_data.h"


// ======== DEFINES ======================
#define KEY_THRESHOLD  50

//
// ======== CONSTANTS ============= 
//


// ======== GLOBAL VARIABLES ============= 
void taskGameplay(void * parameter);

TimerHandle_t keyboardTimer;   // Polls capacitive keys @30Hz

bool btnNextTouched=false;
bool btnOkTouched=false;


static void keyboardTimerCallback( TimerHandle_t xTimer ) {

  static volatile uint32_t keyNextCounter  = 0;
  static volatile uint32_t keyOKCounter    = 0;
  
  if (touchRead(PIN_BUT_1)<KEY_THRESHOLD) keyNextCounter++;  else keyNextCounter =0; 
  if (touchRead(PIN_BUT_2)<KEY_THRESHOLD) keyOKCounter++;    else keyOKCounter   =0; 

  if(keyNextCounter==3) { btnNextTouched=true; }
  if(keyOKCounter  ==3) { btnOkTouched  =true; }

  keyBoardWatchDog++;

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
    taskGameplay,            // The function containing the task
    "taskGameplay",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    1,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on

  xTimerStart(keyboardTimer, 0 );
}

void taskGameplay(void *parameter) 
{
  while(true) 
  {
      // Run the sate machines that manage which screen to display
      if (mainState==msFirstChallenge) 
      {
        Serial.println("msFirstChallenge");
        // Initialize variables and go to the next state
        challengeID=0;
        radarSpinning=false;
        updateScreen=true;
        mainState=msConfirmChallenge;

      } // mainState==msFirstChallenge

      else if (mainState==msConfirmChallenge) 
      {
        if(btnNextTouched) {
          // Display the next challenge
          challengeID++;

          // Allow recycling of first challenge if we accidentally pressed the wrong button
          if(challengeID>CHALLENGE_MAX-1) challengeID=0;

          updateScreen=true;

          btnNextTouched=false;
        }

        if(btnOkTouched) {
          // Stop spinning if the last challenge is reached, otherwise start spinning
          if (challengeID<CHALLENGE_MAX-1)
          {
            radarSpinning=true; // Start the radar
            updateScreen=true;  // Update the screen
            mainState=msRadar;     // Go to the next state
          }
          else radarSpinning=false; // Stop scanning

          btnOkTouched=false;
        }

        } // mainState==msConfirmChallenge

      else if (mainState==msRadar)
      {

        // Go back to confirm challenge state if button is pressed in radar screen
        if(btnOkTouched || btnNextTouched) {
          mainState=msConfirmChallenge;
          updateScreen=true;  // Update the screen
          radarSpinning=false; // Stop the radar

          btnOkTouched=false;
          btnNextTouched=false;
        }

        // Checking if the answer is correct is done in the display module

      }

      else if (mainState==msAnswerCorrect)
      {
        if(challengeID<CHALLENGE_MAX-1) challengeID++;
        updateScreen=true;
        radarSpinning=false; // Stop the radar

        // Ignore buttons during this state
        btnOkTouched=false;
        btnNextTouched=false;

        mainState=msConfirmChallenge;
      }

    vTaskDelay(100);
  } // while
}

#endif
