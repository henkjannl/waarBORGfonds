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
#include "wbf5_display.h"

//====================================================================================
//                                    Setup and Loop
//====================================================================================
void setup()
{
  // Initialize debug channel
  Serial.begin(115200);
  delay(100);

  // Make the radar spin
  initRadar();

  // Start the display as independent process
  setupDisplay();
}

void loop() {
  static unsigned long debugTrigger;
  
  if(debugTrigger<millis()) {
    debugTrigger=millis()+500;
    
    Serial.printf("%d) ", stepper.currentPos());
    Serial.printf("*%d* ", radarDebug);
    for(int i=0; i<RADAR_ARRAY_SIZE; i++) Serial.printf("%3.0f ", radarMeasurements[i]);
    Serial.println("");
  }
}
