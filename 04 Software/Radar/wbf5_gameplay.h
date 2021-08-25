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

void determineBottleLocations() {

    uint8_t counter;
    bool C[5];

    // Calculate the C coefficients
    // these coefficients determine in which regions a response was measured
    portENTER_CRITICAL_ISR(&radarMux);

    counter=0; 
    for(int i=21; i<29; i++) if(radarMeasurements[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
    C[0]=(counter>4);

    counter=0; 
    for(int i=25; i<33; i++) if(radarMeasurements[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
    C[1]=(counter>4);

    counter=0; 
    for(int i=29; i<37; i++) if(radarMeasurements[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
    C[2]=(counter>4);

    counter=0; 
    for(int i=33; i<41; i++) if(radarMeasurements[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
    C[3]=(counter>4);
    
    counter=0; 
    for(int i=37; i<45; i++) if(radarMeasurements[i]<BOTTLE_PRESENT_THRESHOLD) counter++;
    C[4]=(counter>4);

    portEXIT_CRITICAL_ISR(&radarMux);

    // Based on the coefficients, the bottle locations can be determined
    if       (!C[0] && !C[1] &&  C[4] ) bottleLocations=anA;
    else if  ( C[0] && !C[3] && !C[4] ) bottleLocations=anC;
    else if  ( C[1] &&   C[2] && C[3] ) bottleLocations=anB;
    else                                bottleLocations=anNoBottles;
}


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
  uint16_t sustainedGoodAnswer = 0;
  
  while(true) 
  {
      // Run the state machines that manage which screen to display
      switch(mainState) {

        case msWelcomeScreen:
        
          // Splash screen with welcome message is displayed
          // press any key to go to the next screen
          
          if(btnNextTouched || btnOkTouched) {
            updateScreen=true;
            challengeID=0;
            btnNextTouched=false;
            btnOkTouched=false;
            mainState=msConfirmChallenge;
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
            sustainedGoodAnswer=0;
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
  
          // At each sweep of the radar, check if the bottles are in the right location
          if(radarSweepFinished) 
          {
            radarSweepFinished=false;
            determineBottleLocations();

            if(bottleLocations==challenges[challengeID].answer) 
              sustainedGoodAnswer++;
            else
              sustainedGoodAnswer=0;

            // The bottles need to be in the right location for at least two sweeps
            // to prevent moving forward while still moving bottles
            if(sustainedGoodAnswer>2)
              {
                // All the bottles were in the right places
                // go to the next challenge
                radarSpinning=false; // Stop the radar
                challengeID++;
                
                if(challengeID==NUM_CHALLENGES)
                  mainState=msCompleted;
                else
                  mainState=msAnswerCorrect;
    
                updateScreen=true;
              }  
          }
          
          break; // case msRadar
  
  
        case msAnswerCorrect:

          radarSpinning=false; // Stop the radar
  
          // Ignore buttons during this state
          btnOkTouched=false;
          btnNextTouched=false;
  
          mainState=msConfirmChallenge;

          break; // msAnswerCorrect

        case msCompleted:
          // nothing left to do

          // But allow the fols to start again
          if(btnNextTouched || btnOkTouched) {
            updateScreen=true;
            challengeID=0;
            btnNextTouched=false;
            btnOkTouched=false;
            mainState=msConfirmChallenge;
          }

          
        break; //msCompleted
          
        } // switch(mainState)
        
    vTaskDelay(100);
  } // while(true)
}

#endif
