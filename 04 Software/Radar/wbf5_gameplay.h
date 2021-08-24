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
  btnNextTouched=false;
  btnOkTouched=false;
  updateScreen=true;
  mainState=msWelcomeScreen;
  radarSpinning=false;
  
  while(true) 
  {
      // Run the state machines that manage which screen to display
      switch(mainState) {

        case msWelcomeScreen:
        
          // Splash screen with welcome message is displayed
          // press any key to go to the next screen
          
          if(btnNextTouched || btnOkTouched) {
            mainState=msConfirmChallenge;
            updateScreen=true;
            challengeID=0;
            btnNextTouched=false;
            btnOkTouched=false;
          }
  
        break; // case msWelcomeScreen
  
        case msConfirmChallenge:
  
          if(btnNextTouched) {
            // Display the next challenge
            challengeID++;
  
            // Allow recycling of first challenge if we accidentally pressed the wrong button
            if(challengeID>NUM_CHALLENGES-1) challengeID=0;
  
            updateScreen=true;
            btnNextTouched=false;
          }
  
          if(btnOkTouched) {

            // Reset the radar screen to prevent the next answer to be correct straight away
            portENTER_CRITICAL_ISR(&radarMux);
            for(int i=0; i<RADAR_ARRAY_SIZE; i++) radarMeasurements[sampleID] = 3000.0;
            portEXIT_CRITICAL_ISR(&radarMux);
            
            radarSpinning=true; // Start the radar
            updateScreen=true;  // Update the screen
            mainState=msRadar;     // Go to the next state
            btnOkTouched=false;
          }
  
          break; // case msConfirmChallenge
  
        case msRadar:
  
          // Go back to confirm challenge state if button is pressed in radar screen
          if(btnOkTouched || btnNextTouched) {
            mainState=msConfirmChallenge;
            updateScreen=true;  // Update the screen
            radarSpinning=false; // Stop the radar
  
            btnOkTouched=false;
            btnNextTouched=false;
          }
  
          // Checking if the answer is correct is done in the display module
          
          break; // case msRadar
  
  
        case msAnswerCorrect:

          if(challengeID<NUM_CHALLENGES-1) challengeID++;
          updateScreen=true;
          radarSpinning=false; // Stop the radar
  
          // Ignore buttons during this state
          btnOkTouched=false;
          btnNextTouched=false;
  
          mainState=msConfirmChallenge;

          break; // msAnswerCorrect

        case msCompleted:
          // nothing left to do
        break; //msCompleted
          
        } // switch(mainState)
        
    vTaskDelay(100);
  } // while(true)
}

#endif
