//====================================================================================
//                                    Libraries
//====================================================================================
#if CONFIG_FREERTOS_UNICORE
  #define ARDUINO_RUNNING_CORE 0
#else
  #define ARDUINO_RUNNING_CORE 1
#endif

#include "wbf1_pins.h"
#include "wbf2_data.h"
#include "wbf3_stepper.h" 
#include "wbf4_radar.h"
#include "wbf5_gameplay.h"
#include "wbf6_display.h"

//====================================================================================
//                                    Setup and Loop
//====================================================================================
void setup()
{
  // Initialize debug channel
  Serial.begin(115200);
  delay(100);

  // Setup gameplay
  setupGameplay();

  // Make the radar spin
  initRadar();

  // Start the display as independent process
  setupDisplay();
}

void loop() {
  static unsigned long debugTrigger;
  
  if(debugTrigger<millis()) {
    debugTrigger=millis()+500;
    
    if     (mainState==msWelcomeScreen)    Serial.print("First   ");
    else if(mainState==msConfirmChallenge) Serial.print("Confirm ");
    else if(mainState==msRadar)            Serial.print("Radar ");
    else if(mainState==msAnswerCorrect)    Serial.print("Correct ");

    Serial.printf("ch %d ", challengeID);
    
    //Serial.printf("KB%d ", keyBoardWatchDog);
    //Serial.printf("B1%d ", touchRead(PIN_BUT_1));
    //Serial.printf("B2%d ", touchRead(PIN_BUT_2));

/*
    Serial.printf("%d) ", stepper.currentPos());
    Serial.printf("*%d* ", radarDebug);
*/

    if(radarSpinning)
    {
      Serial.printf("[%s] ",debug);
      if(bottleLocations==anNoBottles)   Serial.print("No bottles " );
      else if (bottleLocations==anA    ) Serial.print("Bottle A "   );
      else if (bottleLocations==anB    ) Serial.print("Bottle B "   );
      else if (bottleLocations==anC    ) Serial.print("Bottle C "   );
      
      for(int i=0; i<RADAR_ARRAY_SIZE; i++) Serial.printf("%d ", int(radarMeasurements[i]));
    }
    
    Serial.println("");

  }
}
