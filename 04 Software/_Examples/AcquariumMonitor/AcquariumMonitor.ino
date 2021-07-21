#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#if CONFIG_FREERTOS_UNICORE
  #define ARDUINO_RUNNING_CORE 0
#else
  #define ARDUINO_RUNNING_CORE 1
#endif

#include "am_button.h"
#include "am_data.h"
#include "am_measure.h"
#include "am_display.h"
#include "am_keyboard.h"
#include "am_telegram.h"

using namespace std;


// ======== DEFINES ================
#define FORMAT_SPIFFS_IF_FAILED false


// ======== GLOBAL VARIABLES ============= 


// ======== INTERRUPT HANDLERS   ============= 

void setup() {
  Serial.begin(115200);  

  // Initialize SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      Serial.println("SPIFFS Mount Failed");
      return;
  }
  Serial.println("SPIFFS loaded");  

  // Load configuration data from SPIFFS
  config.Load();
  Serial.println("Config loaded");  

  // Plan when to record the first sample
  data.timeToNextSample=config.sampleInterval;

  // Setup all sub processes
  setupMeasure();
  setupDisplay();  
  setupKeyboard();
  setupTelegram();
}

void loop() {
  // All tasks delegated to separate threads
}
